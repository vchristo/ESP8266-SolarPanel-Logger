/*
D8 = SDA
D9 = SCL
generic esp8266 module 
flash size 4M (!M SPIFFS)
reset metode nodemcu
esp8266 thing
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Wire.h>
#include <string.h>
//#include <ESP8266HTTPClient.h>
#define DBG_OUTPUT_PORT Serial
#define I2C_SLAVE_ADDRESS 9
#define REQ_BUF_SZ   60
#include "server.h"
String json;
String data;
String logJson;
char logF[]="/log.txt";
char File_dcOut[]="/dcoutCur.csv";
uint32_t index_log=0;
void updateDays(String line,uint16_t index);
void setup(void){
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  pinMode(13,OUTPUT);
  digitalWrite(13,HIGH);
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  SPIFFS.begin();
  
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }
  

  //WIFI INIT
  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
  }
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  MDNS.begin(host);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.println(".local/edit to see the file browser");
  
  
  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get all variables in one json call
  server.on("/all", HTTP_GET, [](){
    json = "{";
    json += "\"batChargCurrent\":"+String(mydata.batChargCurrent);
    json += ", \"batteryVoltage\":"+String(mydata.batteryVoltage);
    json += ", \"PVCurrent\":"+String(mydata.PVCurrent);
    json += ", \"PVVoltage\":"+String(mydata.PVVoltage);
    json += ", \"batteryCurrent\":"+String(mydata.batteryCurrent);
    json += ", \"dcOutCurrent\":"+String(mydata.dcOutCurrent);
    json += ", \"led1\":"+String(mydata.LED1);
    json += ", \"led2\":"+String(mydata.LED2);
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  //Serial.print("String Json ");
  //Serial.println(json);
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");
    /****** I2C *** Begin  D8 = SDA . D9 = SCL*/
  //Wire.begin(0,2);
  Wire.begin(5,4);
  Wire.setClock(100000);
//  Wire.begin(0,2);
  Wire.setClockStretchLimit(1500);
  Wire.onReceive(receiveEvent);
 /******* End of I2C ****/
  
 Serial.begin(115200);
 agora=millis();
 /*
  for(uint8_t i=0; i<25;i++){
    char FL[13]={"0"};
    String SFL;
    SFL="/logF"+String(i)+".csv";
    //string.toCharArray(buf, len) 
    SFL.toCharArray(FL, sizeof(SFL));
    createFile(FL);
    
  }
  */
 // deleteFile(File_dcOut);
  currentMinute =minute();
  currentHour=hour();

  //fileUpdateReady
 // deleteFile("logF16.csv");
 dateTime();
 shortYear= year() - 2000;
 ftpSrv.begin("vitor","vitor");
}




/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*****************              LOOP     ***************************************/
// ch
bool fileUpdateReady=false;
void loop(void){
shortYear= year() - 2000;
  server.handleClient();
   ftpSrv.handleFTP();
  sendData();
  
/*  
 medMinuteBuffBaVo=0; // Battery Voltage 5 minutes or more resolution for days
 medMinuteBuffPaAm=0; // panel Current
medMinuteBuffPaVo=0; // panel Voltage
 medMinuteBuffBaAm=0; // Battery Ampere
 medMinuteBuffDcOa=0; // Dc output Ampere
medMinuteBuffBChA=0; // Baterry Charge Amper

uint8_t updateCountMinutes=0;
#define RESMINUTE 5
*/
    
  

    if(updateCount >= RES ){
        buffBChA /=RES;
        buffDcOa /=RES;
        buffBaAm /=RES;
        buffPaVo /=RES;
        buffPaAm /=RES;
        buffBaVo /=RES;
        buffBChA = buffBChA *60/1024.00;
        buffDcOa = buffDcOa *120/1024.00;
        buffBaAm = buffBaAm *120/1024.00;
        buffPaVo = buffPaVo *48/1024.00;
        buffPaAm = buffPaAm *30/1024.00;
        buffBaVo = buffBaVo *15/1024.00;
        
        medMinuteBuffBaVo +=buffBaVo; 
        medMinuteBuffPaAm +=buffPaAm; 
        medMinuteBuffPaVo +=buffPaVo; 
        medMinuteBuffBaAm +=buffBaAm; 
        medMinuteBuffDcOa +=buffDcOa; 
        medMinuteBuffBChA +=buffBChA;

        
        if(updateCountMinutes == RESMINUTE ){
            medMinuteBuffBaVo /=6; 
            medMinuteBuffPaAm /=6; 
            medMinuteBuffPaVo /=6; 
            medMinuteBuffBaAm /=6; 
            medMinuteBuffDcOa /=6; 
            medMinuteBuffBChA /=6;
            medDayBuffBaVo +=medMinuteBuffBaVo; // Battery Voltage 5 minutes or more resolution for days
            medDayBuffPaAm +=medMinuteBuffPaAm; // panel Current
            medDayBuffPaVo +=medMinuteBuffPaVo; // panel Voltage
            medDayBuffBaAm +=medMinuteBuffBaAm; // Battery Ampere
            medDayBuffDcOa +=medMinuteBuffDcOa; // Dc output Ampere
            medDayBuffBChA +=medMinuteBuffBChA; // Baterry Charge Amper
            medDayBuffBaVo +=0; // Battery Voltage 5 minutes or more resolution for days
            medDayBuffPaAm +=0; // panel Current
            medDayBuffPaVo +=0; // panel Voltage
            medDayBuffBaAm +=0; // Battery Ampere
            medDayBuffDcOa +=0; // Dc output Ampere
            medDayBuffBChA +=0; // Baterry Charge Amper
            updateDays(medMinuteBuffBaVo,BaV);
            updateDays(medMinuteBuffPaAm,PaA);
            updateDays(medMinuteBuffPaVo,PaV);
            updateDays(medMinuteBuffBaAm,BaA);
            updateDays(medMinuteBuffDcOa,DcO);
            updateDays(medMinuteBuffBChA,BCh);
            medMinuteBuffBaVo=0; 
            medMinuteBuffPaAm=0; 
            medMinuteBuffPaVo=0; 
            medMinuteBuffBaAm=0; 
            medMinuteBuffDcOa=0; 
            medMinuteBuffBChA=0;
            updateCountMinutes=0;
            if(updateCountMounth == 40){
                medDayBuffBaVo /=40; // Battery Voltage 5 minutes or more resolution for days
                medDayBuffPaAm /=40; // panel Current
                medDayBuffPaVo /=40; // panel Voltage
                medDayBuffBaAm /=40; // Battery Ampere
                medDayBuffDcOa /=40; // Dc output Ampere
                medDayBuffBChA /=40; // Baterry Charge Amper 
                updateMonth(medDayBuffBaVo,BaV);
                updateMonth(medDayBuffPaAm,PaA);
                updateMonth(medDayBuffPaVo,PaV);
                updateMonth(medDayBuffBaAm,BaA);
                updateMonth(medDayBuffDcOa,DcO);
                updateMonth(medDayBuffBChA,BCh);
            
          
                updateCountMounth=0;
                medDayBuffBaVo +=0; // Battery Voltage 5 minutes or more resolution for days
                medDayBuffPaAm +=0; // panel Current
                medDayBuffPaVo +=0; // panel Voltage
                medDayBuffBaAm +=0; // Battery Ampere
                medDayBuffDcOa +=0; // Dc output Ampere
                medDayBuffBChA +=0; // Baterry Charge Amper
             
            }else updateCountMounth++;
            // updateCountMounth++; // se for igual a 40 faz update da media mensal
        }
        else {
            updateCountMinutes++;

        }
//        void updatLog(float data,char *filePart)
        updatLog(buffBChA,"BCh");
        updatLog(buffDcOa,"DcO");
        updatLog(buffBaAm,"BaA");
        updatLog(buffPaVo,"PaV");
        updatLog(buffPaAm,"PaA");
        updatLog(buffBaVo,"BaV");
#ifdef DEBUG        
        Serial.print(" Media dos 60s de Battery Charge Current = "); 
        Serial.println(buffBChA); 
        Serial.print(" Media dos 60s de Dc Out Current = "); 
        Serial.println(buffDcOa); 
        Serial.print(" Media dos 60s de Battery Current = "); 
        Serial.println(buffBaAm);  
        Serial.print(" Media dos 60s de Panel Voltage = "); 
        Serial.println(buffPaVo);       
        Serial.print(" Media dos 60s de Panel Current = "); 
        Serial.println(buffPaAm);        
        Serial.print(" Media dos 60s de Battery Voltage Output = "); 
        Serial.println(buffBaVo);   
#endif
        buffBChA =0;
        buffDcOa =0;
        buffBaAm =0;
        buffPaVo =0;
        buffPaAm =0;
        buffBaVo =0;
     
        updateCount=0;
    


   }
      
 // Serial.println(json);
// pjrc tensee
}

// sets every element of str to 0 (clears array)


