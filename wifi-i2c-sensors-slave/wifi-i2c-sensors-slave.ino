// mkr1000

#include <Wire.h>
#define REQ_BUF_SZ 50
#define LED_0 7
#define LED_1 6
#define AREF 5              // LM4040 4.096
#define BATERY_VOLTAGE A0    
#define PV_CURRENT A1
#define PV_VOLTAGE A2
#define BATERY_CHARGER_CURRENT A3
#define BATERY_CURRENT A6
#define DC_OUT_CURRENT A7
#define BUZZER 2
#define FAN_CONTROL_PWM 3

byte requestBatCurrenteCharge=1;
byte requestBatteryVoltage=2;
byte requestPVCurrent =3;
byte requestPVVoltage=4;
byte requestBatteryCurrente =5;
byte requestDcOutCurrente=6;
byte requestLed1 = 7;
byte requestLed2 =8;
unsigned long agora;
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
boolean LED_state[2] = {1}; // stores the states of the LEDs
/*
struct SEND_DATA_STRUCTURE {
  uint16_t  acCur;
  uint16_t  Power;
  uint16_t  vBat;
  uint16_t  aPanel;
  uint16_t  tempe;
  uint16_t  ac_volts;
};

*/

struct SEND_DATA_STRUCTURE 
{
    uint16_t  batChargCurrent;
    uint16_t  batteryVoltage;
    uint16_t  PVCurrent;
    uint16_t  PVVoltage;            
    uint16_t  batteryCurrent;
    uint16_t  dcOutCurrent;
    byte  LED1;
    byte  LED2;
};
SEND_DATA_STRUCTURE mydata;
//give a name to the group of data


//define slave i2c address
#define I2C_SLAVE_ADDRESS 9

/*********************************************************************************/
/*                          On Wire requeste, Do it!                                     */
/*********************************************************************************/

void manda() {
  //Wire.beginTransmission();
  char c = mydata.batChargCurrent;
  Wire.write(c);
  c = (mydata.batChargCurrent >> 8);
  Wire.write(c);
  /************************/

  c = mydata.batteryVoltage ;
  Wire.write(c);
  c = (mydata.batteryVoltage  >> 8);
  Wire.write(c);

  /************************/

  c = mydata.PVCurrent;
  Wire.write(c);
  c = (mydata.PVCurrent >> 8);
  Wire.write(c);

  /************************/

  c = mydata.PVVoltage;
  Wire.write(c);
  c = (mydata.PVVoltage >> 8);
  Wire.write(c);

  /************************/
  c = mydata.batteryCurrent;
  Wire.write(c);
  c = (mydata.batteryCurrent >> 8);
  Wire.write(c);


  c = mydata.dcOutCurrent;
  Wire.write(c);
  c = (mydata.dcOutCurrent >> 8);
  Wire.write(c);

 c= mydata.LED1;
 Wire.write(c);
 
 c= mydata.LED2;
 Wire.write(c);


  Serial.print("mydata.PVVoltage =");
  Serial.print(mydata.PVVoltage);
  Serial.print(" batteryVoltage =");
  Serial.println(mydata.batteryVoltage);
  
  digitalWrite(13, !digitalRead(13));
 // delay(100);
 //   digitalWrite(13, !digitalRead(13));
  //Serial.println("Pediu dados");
}
void setup() {
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  // LEDs
  pinMode(LED_0, OUTPUT);
  pinMode(LED_1, OUTPUT);
  agora = millis();
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(manda);
  Wire.onReceive(receiveEvent);
  Wire.setClock(100000);
  pinMode(13, OUTPUT);
    digitalWrite(13, !digitalRead(13));
 // delay(35);
 //   digitalWrite(13, !digitalRead(13));

  randomSeed(analogRead(0));
}
/********************************************************************/
/*          on server sends data                                    */
/*          LED_State[] reresents the sectors 0 / 1                 */
/********************************************************************/

void receiveEvent(int howMany) {

  uint8_t c = Wire.read();
  Serial.print("Led 0 =");
  Serial.print(c);
  digitalWrite(LED_0,c);
  LED_state[0]=c;
  /*
  mydata.LED1=c;
  bool tmp;
  if (c == 0)tmp = false;
  else tmp = true;
  if (tmp != LED_state[0]) {
    LED_state[0] = tmp;
    digitalWrite(LED_0, !digitalRead(LED_0));
  }
  //digitalWrite(7,c);
*/
  c = Wire.read();
  
  //mydata.LED2=c;
  Serial.print("  Led 1 =");
  Serial.println(c);
  digitalWrite(LED_1,c);
  LED_state[1]=c;
  /*
  if (c == 0)tmp = false;
  else tmp = true;

  if (tmp != LED_state[1]) {
    LED_state[1] = tmp;
    digitalWrite(LED_1, !digitalRead(LED_1));
  }
  // digitalWrite(6,c);
  */
}
void loop() {

  mydata.batChargCurrent = analogRead(BATERY_VOLTAGE);
  delay(10);
  mydata.batteryVoltage =  analogRead(BATERY_VOLTAGE);
  delay(10);
  mydata.PVCurrent = analogRead(PV_CURRENT);
  delay(10);
  mydata.PVVoltage = analogRead(PV_VOLTAGE);
  delay(10);
  mydata.batteryCurrent = analogRead(BATERY_CURRENT);
   delay(10);
  mydata.dcOutCurrent = analogRead(DC_OUT_CURRENT);
  delay(10);



if (digitalRead(LED_0))mydata.LED1 = 1;
  else mydata.LED1 = 0;
  
if (digitalRead(LED_1))mydata.LED2 = 1;
  else mydata.LED2 = 0;

 //  Serial.print("LedState 0 =");
 //  Serial.print(mydata.LED1);

  //Serial.print(" LedState 1 =");
  //Serial.println(mydata.LED2);


  
  ButtonDebounce();
}

void ButtonDebounce(void) // switch noise suppressor
{
  static byte buttonState[2]     = {LOW, LOW};   // the current reading from the input pin
  static byte lastButtonState[2] = {LOW, LOW};   // the previous reading from the input pin

  // the following variables are long's because the time, measured in miliseconds,
  // will quickly become a bigger number than can be stored in an int.
  static long lastDebounceTime[2] = {0};  // the last time the output pin was toggled
  long debounceDelay = 50;         // the debounce time; increase if the output flickers

  byte reading[2];

  reading[0] = digitalRead(4);
  reading[1] = digitalRead(5);

  for (int i = 0; i < 2; i++) {
    if (reading[i] != lastButtonState[i]) {
      // reset the debouncing timer
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      // whatever the reading is at, it's been there for longer
      // than the debounce delay, so take it as the actual current state:

      // if the button state has changed:
      if (reading[i] != buttonState[i]) {
        buttonState[i] = reading[i];

        // only toggle the LED if the new button state is HIGH
        if (buttonState[i] == HIGH) {
          LED_state[i] = !LED_state[i];
        }
      }
    }
  } // end for() loop

  /********************************/
  /* set the SECTORS relay        */
  /********************************/


  //digitalWrite(LED_0, !LED_state[0]);
  //digitalWrite(LED_1, !LED_state[1]);

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState[0] = reading[0];
  lastButtonState[1] = reading[1];
}
