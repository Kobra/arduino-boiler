//Store values in EEPROM
#include <EEPROM.h>
byte hiTempAddr = 0;

byte hiTemp = 65;
byte tempStep = 5;
float lastTemp;
int tempDir;
bool cool = false;

//LCD
#include <LiquidCrystal.h>
#include <LCDKeypad.h>
LCDKeypad lcd;
byte BLPin = 10;
String line1 = "";
String line2 = "";
byte len, s;

//Temperature Sensor
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 11
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress SensorAddr = { 0x28, 0xFF, 0x55, 0x8E, 0x03, 0x15, 0x02, 0x88 };

//RTC
#include <Wire.h>
#include <RTClib.h>
RTC_DS1307 rtc;
uint32_t lastTimestamp;

//Relay
byte relayPin = 12;

void checkTemp(float temp) {
  byte checkTemp = hiTemp;
  if(lastTemp > temp) tempDir=-1;
  if(lastTemp == temp) tempDir=0;
  if(lastTemp < temp) tempDir=1;
  lastTemp = temp;
  Serial.print("Current Temperature: ");
  Serial.println(temp);
  
  if(cool) checkTemp = hiTemp - tempStep;

  Serial.print("Current State: ");
  if(temp <= checkTemp) {
    relay(true);
    Serial.println("Heating");
  } else {
    relay(false);
    Serial.println("Cooling");
  }
  Serial.print("Temperature direction: ");
  if(tempDir == 1) Serial.println("Up");
  if(tempDir == 0) Serial.println("Same");
  if(tempDir == -1) Serial.println("Down");
  
  Serial.print("Max Temperature: ");
  Serial.print(hiTemp);
  Serial.print(" Cooling step: ");
  Serial.println(tempStep);
}

void backlight(byte level) {
  analogWrite(BLPin, level);
}

float getTemp() {
  if(sensors.isConnected(SensorAddr)) {
    sensors.requestTemperaturesByAddress(SensorAddr);
    float tempC = sensors.getTempC(SensorAddr);
    return tempC;
  } else {
    return false;
  }
}

void printLines() {
  len=line1.length(); for(s=0; s<16-len; s++) line1+=" ";
  len=line2.length(); for(s=0; s<16-len; s++) line2+=" ";
  lcd.setCursor(0,0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

void relay(bool state) {
  cool = !state;
  if(state) digitalWrite(relayPin, HIGH);
  else digitalWrite(relayPin, LOW);
}

void setup() {
  //#ifndef ESP8266
  //  while (!Serial); // for debug console Leonardo/Micro/Zero
  //#endif
  lcd.begin(16, 2);
  pinMode(BLPin, OUTPUT);
  backlight(12);
  sensors.setResolution(SensorAddr, 9);

  hiTemp = EEPROM.read(hiTempAddr);
  if(hiTemp < 20 || hiTemp > 100) {
    hiTemp = 65;
    EEPROM.write(hiTempAddr,hiTemp);
  }

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    //while (1);
  }
  //Set Date and Time to compile time
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  pinMode(relayPin, OUTPUT);
  relay(false);
}

void loop() {
  DateTime now = rtc.now();
  if(rtc.isrunning()) {
    if(now.unixtime() <= lastTimestamp) return;
    lastTimestamp = now.unixtime();
  } else {
    delay(1000);
    rtc.begin();
    lastTimestamp = 0;
  }
  line1 = ""; line2 = "";
  float temp = getTemp();
  if(temp) {
    checkTemp(temp);
    line1 += "Temp: ";
    line1 += int(temp);
  } else {
    relay(false);
    line1 += "Sensor error!";
  }
  line2 += "State: ";
  if(!cool) line2 += "Heating";
  else line2 += "Cooling";
  if(temp && rtc.isrunning()) {
    String clockSep;
    if(now.second() & 1) clockSep = ":"; else clockSep = " ";
    len=line1.length(); for(s=0; s<11-len; s++) line1+=" ";
    if(now.hour()<10) line1+="0"; line1+=now.hour();
    line1 += clockSep;
    if(now.minute()<10) line1+="0"; line1 += now.minute();
  }
  printLines();
}

