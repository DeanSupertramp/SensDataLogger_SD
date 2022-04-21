/*
  SensDataLogger_SD.ino

  A simple data Logger based on Adafruit Feather 32u4 Adalogger Board.
  In this example, acquire current measurement time-referred and store into SD Card.

  The circuit:
    Adafruit Feather 32u4 Adalogger;
    INA168 Current sensor;
    RTC DS1302;
    2x Lipo Battery pack;
    2x 100kohm resistors
    1uF capacitor
    a load (for example, a motor)

  Created 18-02-2022
  By Andrea Alecce
  Modified 18-02-2022

  https://github.com/DeanSupertramp/SensDataLogger_SD

*/

#include <SPI.h>
#include <SD.h>
#include <DS1302.h>
#include <Wire.h>

File logfile;

// -------------------------------------- PINOUT --------------------------------------
DS1302 rtc(13, 11, 6);    // (RST, DAT, CLK)
#define cardSelect 4
#define VBATPIN_MICRO A9
#define VBATPIN A3
float vBat = 0;
float vBat_micro = 0;

// -------------------------------------- INA169 --------------------------------------
const int SENSOR_PIN = A0;            // Input pin for measuring Vout
const float RS = 0.1;                 // Shunt resistor value (in ohms)
const float RL = 10000;               // Load resistor value (in ohms)
const float VOLTAGE_REF = 3.3;        // Reference voltage for analog read

// Global Variables
float sensorValue_RAW, sensorValue;   // Variable to store value from analog read
uint8_t i = 0;
String m, dateTime;
int mLast = 0;

float current, currentSum, currentMean = 0.0;
float vBatSum, vBatMean = 0.0;
float vBatSum_micro, vBatMean_micro = 0.0;

String s, lastS;
int lastS_Int = 0;
int mInt;

void readINA_current() {
  sensorValue_RAW = analogRead(SENSOR_PIN);               // Read a value from the INA169 board
  sensorValue = (sensorValue_RAW * VOLTAGE_REF) / 1023;   // Remap the ADC value into a voltage number
  current = sensorValue * 1000 / (RS * RL);               // Is = (Vout x 1k) / (RS x RL)
  currentSum += current;
}

// -------------------------------------- VBat --------------------------------------
void Vbat() {
  vBat = analogRead(VBATPIN);
  vBat *= 2;      // we divided by 2, so multiply back
  vBat *= 3.3;    // Multiply by 3.3V, our reference voltage
  vBat /= 1024;   // convert to voltage
  vBatSum += vBat;
}

// -------------------------------------- VBat_micro --------------------------------------
void Vbat_micro() {
  vBat_micro = analogRead(VBATPIN_MICRO);
  vBat_micro *= 2;      // we divided by 2, so multiply back
  vBat_micro *= 3.3;    // Multiply by 3.3V, our reference voltage
  vBat_micro /= 1024;   // convert to voltage
  vBatSum_micro += vBat_micro;
}

void error(uint8_t errno) {
  while (1) {
    for (i = 0; i < errno; i++) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
    for (i = errno; i < 10; i++) {
      delay(200);
    }
  }
}

// Debug print
void serialPrint() {
  Serial.print(rtc.getDateStr()); Serial.print(", ");
  Serial.print(rtc.getTimeStr()); Serial.print(", ");
  Serial.print(vBatMean); Serial.print(", ");
  Serial.print(vBatMean_micro); Serial.print(", ");
  Serial.println(currentMean, 3);
  Serial.print("VBat_micro istantanea: "); Serial.println(vBat_micro);
  Serial.print("VBat istantanea: "); Serial.println(vBat);
  Serial.print("IBat istantanea: "); Serial.println(current, 3);
}

// -------------------------------------- SETUP --------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);

  if (!SD.begin(cardSelect)) {   // see if the card is present and can be initialized:
    Serial.println("Card init. failed!");
    error(2);
  }
  char filename[15];
  strcpy(filename, "/AREADS00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[7] = '0' + i / 10;
    filename[8] = '0' + i % 10;
    if (! SD.exists(filename)) {     // create if does not exist, do not open existing, write, sync after write
      break;
    }
  }
  logfile = SD.open(filename, FILE_WRITE);
  if ( ! logfile ) {
    Serial.print("Couldnt create ");
    Serial.println(filename);
    error(3);
  }
  Serial.print("Writing to ");
  Serial.println(filename);

  pinMode(13, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.println("Ready!");

  // -------------------------------------- SETUP RTC --------------------------------------
  delay (500);
  //  RTC run-mode and disable write protection
//  rtc.halt(false);
//  rtc.writeProtect(false);
//  //set Time in memory (use only the first time)
//  rtc.setDOW(FRIDAY);        // Set Day
//  rtc.setTime(23, 02, 30);        // Set hour, minutes and seconds 11:32:00 (24hr)
//  rtc.setDate(18, 02, 2022);    // set Date 7 July 2022

  // First Row
  logfile.print("Date"); logfile.print(",");
  logfile.print("Time"); logfile.print(",");
  logfile.print("VBat_micro"); logfile.print(",");
  logfile.print("VBat"); logfile.print(",");
  logfile.println("Current");

  // A first and fast average value
  for (int k = 0; k < 60; k++) {
    Vbat();
    Vbat_micro();
    readINA_current();
  }
  vBatMean = vBatSum / 60;
  vBatMean_micro = vBatSum_micro / 60;
  currentMean = currentSum / 60;
  vBatSum = 0;
  vBatSum_micro = 0;
  currentSum = 0;
}


// -------------------------------------- LOOP --------------------------------------
void loop() {
  // Serial.print(rtc.getDOWStr());    Serial.print(" ");     // Invia giorno della settimana
  // Invia data e ora
  dateTime = rtc.getTimeStr();
  m = dateTime.substring(3, 5);
  s = dateTime.substring(6, 8);

  // Sample every second
  if (s.toInt() - lastS_Int >= 1) {
    lastS_Int = s.toInt();
    Vbat();
    Vbat_micro();
    readINA_current();
    serialPrint(); // debug
  }

  // Write to SD every minutes
  mInt = m.toInt();
  if (mInt - mLast >= 1) {
    digitalWrite(8, HIGH);

    vBatMean = vBatSum / 60;
    vBatSum = 0;
    vBatMean_micro = vBatSum_micro / 60;
    vBatSum_micro = 0;
    currentMean = currentSum / 60;
    currentSum = 0;

    mLast = mInt;

    logfile.print(rtc.getDateStr()); logfile.print(",");
    logfile.print(rtc.getTimeStr()); logfile.print(",");
    logfile.print(vBatMean_micro); logfile.print(",");
    logfile.print(vBatMean); logfile.print(",");
    logfile.println(currentMean, 3);
    logfile.flush();
    digitalWrite(8, LOW);

    lastS_Int -= lastS_Int + 1;

  }

  if (mInt == 0 && mLast == 59) {
    mLast = -1;
  }
}
