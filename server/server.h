#include <ESP8266WebServer.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <ESP8266FtpServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#define DBG_OUTPUT_PORT Serial
#define I2C_SLAVE_ADDRESS 9
#define REQ_BUF_SZ   60
/****  Pins Defination   *****/
#define AREF 5              // LM4040 4.096
#define BATERY_VOLTAGE A0    
#define PV_CURRENT A1
#define PV_VOLTAGE A2
#define BATERY_CHARGER_CURRENT A3
#define BATERY_CURRENT A6
#define DC_OUT_CURRENT A7
#define BUZZER 2
#define FAN_CONTROL_PWM 3
#define RES 12
#define RESMINUTE 5
#define UPDATEHOUR 22
#define UPDATEMINUTE 43
#define UPDATESECOUND 1
#define DEBUG 1
String buf;
unsigned long agora;
unsigned long tmp=0;
uint8_t currentDay=0;
uint8_t currentHour=0;
uint8_t currentMinute=0;
uint8_t updateCountMounth =0;
/*****   Time sErver *********/
String Time={"00:00:00"};
String Date={"00-00-000"};
char BCh[]="A";
char DcO[]="B";
char BaA[]="C";
char PaV[]="D";
char PaA[]="E";
char BaV[]="F";

FtpServer ftpSrv;

uint8_t monthTable[]={31,  28, 31, 30, 31, 30,  31, 31, 30,31 ,30, 31};
// 31,  28, 31, 30, 31, 30,  31, 31, 30,31 ,30, 31
// jam,fev,mar,abr,mai,jun,jul,ago,set,out,nov,dez
// 2020, 2024, 2028, 
// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

const int timeZone = 0;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)


unsigned int localPort = 8888;  // local port to listen for UDP packets


/*********************/
WiFiClient client;
byte requestBatCurrenteCharge=1;
byte requestBatteryVoltage=2;
byte requestPVCurrent =3;
byte requestPVVoltage=4;
byte requestBatteryCurrente =5;
byte requestDcOutCurrente=6;
byte requestLed1 = 7;
byte requestLed2 =8;
String logDcOutCurrent;
String tmpLog;
float bccVal=0;

struct io_int_payload{
    uint16_t  batChargCurrent;
    uint16_t  batteryVoltage;
    uint16_t  PVCurrent;
    uint16_t  PVVoltage;            
    uint16_t  batteryCurrent;
    uint16_t  dcOutCurrent;
    byte      LED1;
    byte      LED2;
};

io_int_payload mydata;
unsigned long updateInterval=5000;
uint8_t compressionRate=12;
uint8_t updateCount=0;
uint8_t updateCountMinutes=0;
float buffBaVo=0; // Battery Voltage
float buffPaAm=0; // panel Current
float buffPaVo=0; // panel Voltage
float buffBaAm=0; // Battery Ampere
float buffDcOa=0; // Dc output Ampere
float buffBChA=0; // Baterry Charge Amper


float medMinuteBuffBaVo=0; // Battery Voltage 5 minutes or more resolution for days
float medMinuteBuffPaAm=0; // panel Current
float medMinuteBuffPaVo=0; // panel Voltage
float medMinuteBuffBaAm=0; // Battery Ampere
float medMinuteBuffDcOa=0; // Dc output Ampere
float medMinuteBuffBChA=0; // Baterry Charge Amper


float medDayBuffBaVo=0; // Battery Voltage 5 minutes or more resolution for days
float medDayBuffPaAm=0; // panel Current
float medDayBuffPaVo=0; // panel Voltage
float medDayBuffBaAm=0; // Battery Ampere
float medDayBuffDcOa=0; // Dc output Ampere
float medDayBuffBChA=0; // Baterry Charge Amper

uint16_t shortYear=0;




char req_index = 0;  // index into HTTP_req buffer
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
uint8_t LED_state[2] = {0}; // stores the states of the LEDs

time_t prevDisplay = 0; // when the digital clock was displayed
const char* ssid = "Vodafone-12E3BA";
const char* password = "4FB985898B";
const char* host = "esp8266fs";
String formatBytes(size_t bytes);
String getContentType(String filename);
bool handleFileRead(String path);
void handleFileUpload();
void handleFileDelete();
void handleFileCreate();
void handleFileList();
char StrContains(char *str, char *sfind);
void sentToSlave();
void createFile(char* fileName);
void deleteFile(char * fileName);
void writeFile(String msg, char *fileName);
void readFile(char *fileName);
void openFS(void);
void closeFS(void);
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
void receiveEvent(int howMany);
void sendData();
void dateTime();
void removLog();
void createEmptyLog();
void updatLog(float data,char *filePart);
void updateDays(float data,char *filePart); 
void updateMonth(float data,char *filePart);
uint8_t dayInMonth(uint8_t month, uint8_t year);

uint8_t adjustDayDel();
uint8_t adjustDayDel(){
uint8_t tmp=0;
    if(month() ==1)tmp=12;
    else tmp=month()-1; 
    return dayInMonth(tmp,year());
}



uint8_t dayInMonth(uint8_t month, uint8_t year){
       if((month==1) &&(year%4==0))
        return 29; 
       else return monthTable[month];
}
void updateMonth(float data,char *filePart)
{        /******** create string hour + val Battery Current  *****/
// /022265519A.
     char FileName[14];
        String tmp="";
        
        if(month() <10)tmp +="0" + String(month());
        else tmp += String(month());
        tmp +=String(shortYear);        
        
        String strLogDcOutCurrent="";
        String StringFileName="";
        StringFileName="/";
        StringFileName += tmp + filePart;
        StringFileName += ".csv";
        StringFileName.toCharArray(FileName, 13);
        strLogDcOutCurrent=Date;
        strLogDcOutCurrent +="-";
        strLogDcOutCurrent +=Time;
        strLogDcOutCurrent += ",";
        strLogDcOutCurrent += String(data);
        if(!SPIFFS.exists(FileName)){
           createFile(FileName);
           writeFile(strLogDcOutCurrent,FileName);
        }else writeFile(strLogDcOutCurrent,FileName);
        Serial.print("File Name Mensal Up to date = ");
        Serial.println(FileName);        

}
void updateDays(float data,char *filePart)
{        /******** create string hour + val Battery Current  *****/
// /022265519A.

     char FileName[14];
        String tmp="";
        if(day()<10)tmp="0" + String(day());
        else  tmp = String(day());
        
        if(month() <10)tmp +="0" + String(month());
        else tmp += String(month());
        tmp +=String(shortYear);        
        
        String strLogDcOutCurrent="";
        String StringFileName="";
        StringFileName="/";
        StringFileName += tmp + filePart;
        StringFileName += ".csv";
        StringFileName.toCharArray(FileName, 13);
        strLogDcOutCurrent=Date;
        strLogDcOutCurrent +="-";
        strLogDcOutCurrent +=Time;
        strLogDcOutCurrent += ",";
        strLogDcOutCurrent += String(data);
        if(!SPIFFS.exists(FileName)){
           createFile(FileName);
           writeFile(strLogDcOutCurrent,FileName);
        }else writeFile(strLogDcOutCurrent,FileName);
        Serial.print("File Name Diario Up to date = ");
        Serial.println(FileName);        
/*
#ifdef DEBUG        
            Serial.print(" Media dos 5 minutos de Battery Charge Current = "); 
            Serial.println(medMinuteBuffBaVo); 
            Serial.print(" Media dos 5 minutos de Dc Out Current = "); 
            Serial.println(medMinuteBuffDcOa); 
            Serial.print(" Media dos 5 minutos de Battery Current = "); 
            Serial.println( medMinuteBuffBaAm);  
            Serial.print(" Media dos 5 minutos de Panel Voltage = "); 
            Serial.println( medMinuteBuffPaVo);       
            Serial.print(" Media dos 5 minutos de Panel Current = "); 
            Serial.println( medMinuteBuffPaAm);        
            Serial.print(" Media dos 5 minutos de Battery Voltage Output = "); 
            Serial.println( medMinuteBuffBChA );   
#endif
*/
}

void updatLog(float data,char *filePart){        /******** create string hour + val Battery Current  *****/
        char FileName[14];
        char fileToDelet[14];
        String tmp="";
        String tmp1="";        
        uint8_t dayDel=0;
        if(day() < 4)dayDel=adjustDayDel();    // adjust day to delet return the day tobe deleted
        else dayDel=day() -3;
        if(dayDel<10)tmp1="0" + String(dayDel);
        else  tmp1 = String(dayDel);       
        if(day()<10)tmp="0" + String(day());
        else  tmp = String(day());
        if(hour() <10){
                tmp +="0" + String(hour());
                tmp1 +="0" + String(hour());
            }
        else {
                tmp += String(hour());
                tmp1 += String(hour());
            }
        String val="";
        String strLogDcOutCurrent="";
        String fileToDel="/";
        String StringFileName="";
        
        StringFileName="/";
        StringFileName += tmp + filePart;
        fileToDel += tmp1 + filePart;
        fileToDel += ".csv"; 
        StringFileName += ".csv";
        StringFileName.toCharArray(FileName, 13);
        fileToDel.toCharArray(fileToDelet,13);
        
        strLogDcOutCurrent=Time;
        strLogDcOutCurrent += ",";
        strLogDcOutCurrent += String(data);
        if(SPIFFS.exists(fileToDelet)){
            deleteFile(fileToDelet);
        }else{
            Serial.print("The File ");
            Serial.print(fileToDelet);
            Serial.println(" Does not Exists ");            
        }
        if(!SPIFFS.exists(FileName)){
           createFile(FileName);
           writeFile(strLogDcOutCurrent,FileName);
        }else writeFile(strLogDcOutCurrent,FileName);
        Serial.print("File Name = ");
        Serial.println(FileName);        
        
     }

   
void createEmptyLog()
{
 String tmp="";
 String tmp1="";
 char fileName[13];
 if(day() < 10)tmp="/0";
 tmp +="/" + String(day());
 if(month() < 10)tmp1="0";
 tmp1 += String(month()); 

 String StringfileName=tmp + tmp1 + String(shortYear);
 /********** Create file log named /ddmmyyA for BCh **********/
 String fileName_c = StringfileName + BCh;
 fileName_c.toCharArray(fileName, 13);
 createFile(fileName);
  /********** Create file log named /ddmmyyB for DcO **********/
 fileName_c ="";
 fileName_c = StringfileName + DcO;
 fileName_c.toCharArray(fileName, 13);
 createFile(fileName);

  /********** Create file log named /ddmmyyC for BaA **********/
 fileName_c ="";
 fileName_c = StringfileName + BaA;
 fileName_c.toCharArray(fileName, 13);
 createFile(fileName);

  /********** Create file log named /ddmmyyD for PaV **********/
 fileName_c ="";
 fileName_c = StringfileName + PaV;
 fileName_c.toCharArray(fileName, 13);
 createFile(fileName);

  /********** Create file log named /ddmmyyE for PaA **********/
 fileName_c ="";
 fileName_c = StringfileName + PaA;
 fileName_c.toCharArray(fileName, 13);
 createFile(fileName);
  /********** Create file log named /ddmmyyF for BaV **********/
 fileName_c ="";
 fileName_c = StringfileName + BaV;
 fileName_c.toCharArray(fileName, 13);
 createFile(fileName);

 
    /*
        updatLog(buffBChA,"BCh");
        updatLog(buffDcOa,"DcO");
        updatLog(buffBaAm,"BaA");
        updatLog(buffPaVo,"PaV");
        updatLog(buffPaAm,"PaA");
        updatLog(buffBaVo,"BaV");
    
    */
 
}

void removLog(){
  for(uint8_t i=0; i<25;i++){
    char FL[13]={"0"};
    String SFL;
    SFL="/logF"+String(i)+".csv";
    //string.toCharArray(buf, len) 
    SFL.toCharArray(FL, sizeof(SFL));
    deleteFile(FL);
    
  }
}

/************************************************************************************/
void sendData(){
  unsigned long h=millis();
  if((h - agora) >= updateInterval){  // update time
    
       if (timeStatus() != timeNotSet) {
 //   if (now() != prevDisplay) { //update the display only if time has changed
 //     prevDisplay = now();
      dateTime();
      

  Serial.println(Time);
//  Serial.println(Date);      
    
  }
    Wire.requestFrom(9,14); // request 8 bytes from slave
    while (Wire.available()) { 
      char c = Wire.read(); 
      delay(10);
      mydata.batChargCurrent = Wire.read();
      mydata.batChargCurrent = (mydata.batChargCurrent << 8) | c;
      delay(10);
      /**********************/
      c = Wire.read(); // receive a byte as character
      delay(10);
      mydata.batteryVoltage = Wire.read();
      mydata.batteryVoltage = (mydata.batteryVoltage << 8) | c;
      delay(10);
      /**********************/
      c = Wire.read(); // receive a byte as character
      delay(10);
      mydata.PVCurrent = Wire.read();
      mydata.PVCurrent = (mydata.PVCurrent << 8) | c;
      delay(10);
      /**********************/
      c = Wire.read(); // receive a byte as character
      delay(10);
      mydata.PVVoltage = Wire.read();
      mydata.PVVoltage = (mydata.PVVoltage << 8) | c;
     // Serial.print("Temperatura ");
     // Serial.println(50*mydata.tempe/1024.0);
      delay(10);
      /**********************/
      
      c = Wire.read(); // receive a byte as character
      delay(10);
      mydata.batteryCurrent = Wire.read();
      mydata.batteryCurrent = (mydata.batteryCurrent << 8) | c;
     // Serial.print("Temperatura ");
     // Serial.println(50*mydata.tempe/1024.0);
      delay(10);
      
       /**********************/
      c = Wire.read(); // receive a byte as character
      delay(10);
      mydata.dcOutCurrent = Wire.read();
      mydata.dcOutCurrent = (mydata.dcOutCurrent << 8) | c;
      delay(10);
       /**********************/
      mydata.LED1 = Wire.read(); // receive a byte as character
 //     if(c==0)mydata.LED1=0;
 //     else mydata.LED1=1;
     
      /**********************/
      delay(10);
      mydata.LED2 = Wire.read(); // receive a byte as character
      //if(c==0)mydata.LED2=0;
      //else mydata.LED2=1;
    //  Serial.print("LED_state[0] ");
    //  Serial.println(mydata.LED1,DEC);
    //  Serial.print("LED_state[1] ");
    //  Serial.println(mydata.LED2,DEC);
    

    
    }
  agora=millis();
   buffBChA +=mydata.batChargCurrent;
   buffDcOa +=mydata.dcOutCurrent;
   buffBaAm +=mydata.batteryCurrent;
   buffPaVo +=mydata.PVVoltage;
   buffPaAm +=mydata.PVCurrent;
   buffBaVo +=mydata.batteryVoltage;
   
   updateCount++;
  }

}
/************************************************************************************/
void receiveEvent(int howMany)
{
  while(1 < Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
 // uint16_ x;
  mydata.batChargCurrent =  Wire.read();    // receive byte as an integer
 
  Serial.println(mydata.batChargCurrent);         // print the integer
  Serial.println("********************************************");
}
WiFiUDP Udp;

/************************************************************************************/

/*-------- NTP code ----------*/
void dateTime() 
{

  // digital clock display of the time
  currentMinute=minute();
  currentHour=hour();
  currentDay=day();
  Time=String(hour());
  if(minute()<10){
    Time +=":0"+String(minute());  
  }else Time +=":" + String(minute());

  if(second()<10){
    Time +=":0"+String(second());  
  }else Time +=":" + String(second());

  
  if(day() <10){
      Date+="-0" + String(day());
    }else Date += "-" +String(day());
    Date=String(day());
    if(month() <10){
      Date+="-0" + String(month());
    }else Date += "-" + String(month());

    Date +="-"+ String(year());


}
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
/************************************************************************************/
time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}
/************************************************************************************/
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

/************************************************************************************/
void closeFS(void){
  SPIFFS.end();
}
 /************************************************************************************/
void openFS(void){
  //Abre o sistema de arquivos
  if(!SPIFFS.begin()){
    Serial.println("Erro ao abrir o sistema de arquivos");
  } else {
    Serial.println("Sistema de arquivos aberto com sucesso!");
  }
}
 
/************************************************************************************/
void readFile(char *fileName) {
  //Faz a leitura do arquivo
  File rFile = SPIFFS.open(fileName,"r");
  Serial.println("Reading file...");
  while(rFile.available()) {
    String line = rFile.readStringUntil('\n');
    buf += line;
    buf += "<br />";
  }
  rFile.close();
}


void writeFile(String msg, char *fileName) {
 
  //Abre o arquivo para adição (append)
  //Inclue sempre a escrita na ultima linha do arquivo
  File rFile = SPIFFS.open(fileName,"a+");
 
  if(!rFile){
    Serial.println("Erro ao abrir arquivo!");
  } else {
    rFile.println(msg);
    //Serial.println(msg);
  }
  rFile.close();
}
 
void deleteFile(char * fileName) {
  //Remove o arquivo
  if(SPIFFS.remove(fileName)){
    Serial.println("Erro ao remover arquivo!");
  } else {
    Serial.println("Arquivo removido com sucesso!");
  }
}

void createFile(char* fileName){
  File wFile;
 
  //Cria o arquivo se ele não existir
  if(SPIFFS.exists(fileName)){
    Serial.println("Arquivo ja existe!");
  } else {
    Serial.println("Criando o arquivo...");
    wFile = SPIFFS.open(fileName,"w+");
 
    //Verifica a criação do arquivo
    if(!wFile){
      Serial.println("Erro ao criar arquivo!");
    } else {
      Serial.println("Arquivo criado com sucesso!");
    }
  }
  wFile.close();
}

void requestData(){
  /*
    uint16_t  batChargCurrent;
    uint16_t  batteryVoltage;
    uint16_t  PVCurrent;
    uint16_t  PVVoltage;            
    uint16_t  batteryCurrent;
    uint16_t  dcOutCurrent;
    byte      LED1;
    byte      LED2;
*/

  
}
ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;


//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
char buffer[1024];


bool handleFileRead(String path){ 
  
  strcpy(buffer,path.c_str()); //         check from web site if Led status is changend 
  if (StrContains(buffer, "LED1=1")) { // if changed, send it to I2C slave, as arduino sensores 
      LED_state[0]=1;
      sentToSlave();
      Serial.println("LED1=1");
       
  }
    if (StrContains(buffer, "LED1=0")) {
      LED_state[0]=0;
      sentToSlave();
      Serial.println("LED1=0");
       
  }
    if (StrContains(buffer, "LED2=1")) {
      LED_state[1]=1;
      sentToSlave();
      Serial.println("LED2=1");
       
  }
    if (StrContains(buffer, "LED2=0")) {
       LED_state[1]=0;
       sentToSlave();
       Serial.println("LED2=0");
      
  }
  
 // DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
    
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}

char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}
/*************************************************************************/
void sentToSlave(){
  Serial.print(" Led State Led0 =");
  Serial.print(LED_state[0]);
  Serial.print(" Led State Led1 =");
  Serial.println(LED_state[1]);
  //mydata.LED1=LED_state[0];
  //mydata.LED2=LED_state[1];
  Wire.beginTransmission(9); // transmit to device #8
  Wire.write(LED_state[0]);    
  Wire.write(LED_state[1]);    
  Wire.endTransmission();    // stop transmitting
}


