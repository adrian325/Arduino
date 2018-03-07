#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include "DHT.h"
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)


DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x3F,16,2);

const int OT_IN_PIN = 17; //3 //Arduino UNO
const int OT_OUT_PIN = 16; //2 //Arduino UNO
const unsigned int bitPeriod = 1000; //1020 //microseconds, 1ms -10%+15%

//P MGS-TYPE  SPARE  DATA-ID    DATA-VALUE
//0    000    0000  00000000 00000000 00000000
unsigned long requests[] = {
  0x300, //0 get status
  0x90011400, //1 set CH temp //64C
  0x80190000, //25 Boiler water temperature
};


// ###### FUNCTIONS ########
void setIdleState() {
  digitalWrite(OT_OUT_PIN, HIGH);
}

void setActiveState() {
  digitalWrite(OT_OUT_PIN, LOW);
}

void activateBoiler() {
  setIdleState();
  delay(1000);
}

void sendBit(bool high) {
  if (high) setActiveState(); else setIdleState();
  delayMicroseconds(500);
  if (high) setIdleState(); else setActiveState();
  delayMicroseconds(500);
}

void sendFrame(unsigned long request) {
  sendBit(HIGH); //start bit
  for (int i = 31; i >= 0; i--) {
    sendBit(bitRead(request, i));
  }
  sendBit(HIGH); //stop bit
  setIdleState();
}

void printBinary(unsigned long val) {
  for (int i = 31; i >= 0; i--) {
    Serial.print(bitRead(val, i));
  }
}

unsigned long sendRequest(unsigned long request) {
  Serial.println();
  Serial.print("Request:  ");
  printBinary(request);
  Serial.print(" / ");
  Serial.print(request, HEX);
  Serial.println();
  sendFrame(request);

  if (!waitForResponse()) return 0;

  return readResponse();
}

bool waitForResponse() {
  unsigned long time_stamp = micros();
  while (digitalRead(OT_IN_PIN) != HIGH) { //start bit
    if (micros() - time_stamp >= 1000000) {
      Serial.println("Response timeout");
      return false;
    }
  }
  delayMicroseconds(bitPeriod * 1.25); //wait for first bit
  return true;
}

unsigned long readResponse() {
  unsigned long response = 0;
  for (int i = 0; i < 32; i++) {
    response = (response << 1) | digitalRead(OT_IN_PIN);
    delayMicroseconds(bitPeriod);
  }
  Serial.print("Response: ");
  printBinary(response);
  Serial.print(" / ");
  Serial.print(response, HEX);
  Serial.println();

  if ((response >> 16 & 0xFF) == 25) {
    Serial.print("t=");
    Serial.print(response >> 8 & 0xFF);
    Serial.println("");
  }
  return response;
}

unsigned int getTargergetTemp() {
  
}


// #### SETUP ####
void setup() {

  // DTH + LCD
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("T: ");
  lcd.setCursor(9, 0);
  lcd.print("H: ");
  lcd.setCursor(3, 1);
  lcd.print("Target:");
  

  // OPEN THERM
  pinMode(OT_IN_PIN, INPUT);
  pinMode(OT_OUT_PIN, OUTPUT);
  activateBoiler();
}


// #### LOOP ####
void loop() {

  float local_humidity = dht.readHumidity();
  float local_temperature = dht.readTemperature();
  int target_value = 0;


  for (int index = 0; index < (sizeof(requests) / sizeof(unsigned long)); index++) {

    target_value = analogRead(0);
    target_value = target_value/34;

    if (index==1 && (local_temperature < target_value)){
      sendRequest(0x90014000);
      lcd.setCursor(14, 1);
      lcd.print("CH");    
    } else {
      sendRequest(requests[index]);
      lcd.setCursor(14, 1);
      lcd.print("  ");
    }
    delay(500);
    
    lcd.setCursor(11, 0);
    lcd.print(local_humidity);
    lcd.setCursor(2, 0);
    lcd.print(local_temperature);
    if (target_value<10) {
      lcd.setCursor(11,1);
      lcd.print("0");
      lcd.setCursor(12,1);
      lcd.print(target_value);
    } else {
      lcd.setCursor(11,1);
      lcd.print(target_value);
    }
  } 
}
