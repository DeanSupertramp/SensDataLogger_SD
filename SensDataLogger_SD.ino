#include <SPI.h>
#include <SD.h>
#include <DS1302.h>
#include <Wire.h>

// Creazione oggetto RTC
DS1302 rtc(13, 11, 6);    // (RST, DAT, CLK)

// Set the pins used
#define cardSelect 4

File logfile;

#define VBATPIN_MICRO A9
#define VBATPIN A3
float measuredvbat = 0;
float measuredvbat_micro = 0;


// -------------------------------------- INA169 --------------------------------------
const int SENSOR_PIN = A0;            // Input pin for measuring Vout
const float RS = 0.1;                 // Shunt resistor value (in ohms)
const float RL = 10000;               // Load resistor value (in ohms)
const float VOLTAGE_REF = 3.3;        // Reference voltage for analog read

// Global Variables
float sensorValue_RAW, sensorValue;   // Variable to store value from analog read

uint8_t i, j = 0;
String minuti, minNow;
int minLast = 0;

float current, currentSum, currentMean = 0.0;
float measuredvbatSum, measuredvbatMean = 0.0;
float measuredvbatSum_micro, measuredvbatMean_micro = 0.0;


void readINA_current() {
  sensorValue_RAW = analogRead(SENSOR_PIN);               // Read a value from the INA169 board
  sensorValue = (sensorValue_RAW * VOLTAGE_REF) / 1023;   // Remap the ADC value into a voltage number
  current = sensorValue * 1000 / (RS * RL);               // Is = (Vout x 1k) / (RS x RL)
  currentSum += current;
}

// -------------------------------------- VBat --------------------------------------
void Vbat() {
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;      // we divided by 2, so multiply back
  measuredvbat *= 3.3;    // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024;   // convert to voltage
  measuredvbatSum += measuredvbat;
}

void Vbat_micro() {
  measuredvbat_micro = analogRead(VBATPIN_MICRO);
  measuredvbat_micro *= 2;      // we divided by 2, so multiply back
  measuredvbat_micro *= 3.3;    // Multiply by 3.3V, our reference voltage
  measuredvbat_micro /= 1024;   // convert to voltage
  measuredvbatSum_micro += measuredvbat_micro;
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

// -------------------------------------- SETUP --------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);

  if (!SD.begin(cardSelect)) {   // see if the card is present and can be initialized:
    Serial.println("Card init. failed!");
    error(2);
  }
  char filename[15];
  strcpy(filename, "/ANALOG00.TXT");
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

  // Imposta RTC in run-mode e disabilita la protezione da scrittura
  rtc.halt(false);
  rtc.writeProtect(false);
  // Impostazione valori nella memoria del DS1302
  rtc.setDOW(FRIDAY);        // Imposta il giorno della settimana
  rtc.setTime(17, 32, 0);        // Imposta l'ora come 11:32:00 (Formato 24hr)
  rtc.setDate(18, 02, 2022);    // Imposta la data come 7 novembre 2020

  logfile.print("Date"); logfile.print(","); logfile.print("Time"); logfile.print(","); logfile.print("VBat_micro"); logfile.print(","); logfile.print("VBat"); logfile.print(","); logfile.println("Current");

  for (int k = 0; k < 60; k++) {
    Vbat();
    Vbat_micro();
    readINA_current();
  }
  measuredvbatMean = measuredvbatSum / 60;
  measuredvbatMean_micro = measuredvbatSum_micro / 60;
  currentMean = currentSum / 60;
  measuredvbatSum = 0;
  measuredvbatSum_micro = 0;
  currentSum = 0;
}

String secondi, lastSecondi;
int lastSecondi_Int = 0;
int num;

void serialPrint() {
  Serial.print(rtc.getDateStr()); Serial.print(", "); Serial.print(rtc.getTimeStr()); Serial.print(", "); Serial.print(measuredvbatMean); Serial.print(", "); Serial.println(currentMean, 3);
  Serial.print("VBat_micro istantanea: "); Serial.println(measuredvbat_micro);
  Serial.print("VBat istantanea: "); Serial.println(measuredvbat);
  Serial.print("IBat istantanea: "); Serial.println(current, 3);
}

void loop() {
  // Serial.print(rtc.getDOWStr());    Serial.print(" ");     // Invia giorno della settimana
  // Invia data e ora
  minuti = rtc.getTimeStr();
  minNow = minuti.substring(3, 5);
  secondi = minuti.substring(6, 8);

  if (secondi.toInt() - lastSecondi_Int >= 1) {
    lastSecondi_Int = secondi.toInt();
    Vbat();
    Vbat_micro();
    readINA_current();
    serialPrint(); // debug
  }

  num = minNow.toInt();
  if (num - minLast >= 1) {
    digitalWrite(8, HIGH);

    measuredvbatMean = measuredvbatSum / 59;
    measuredvbatSum = 0;
    measuredvbatMean_micro = measuredvbatSum_micro / 59;
    measuredvbatSum_micro = 0;
    currentMean = currentSum / 59;
    currentSum = 0;

    minLast = num;
    logfile.print(rtc.getDateStr()); logfile.print(","); logfile.print(rtc.getTimeStr()); logfile.print(","); logfile.print(measuredvbatMean_micro); logfile.print(","); logfile.print(measuredvbatMean); logfile.print(","); logfile.println(currentMean, 3);
    logfile.flush();
    digitalWrite(8, LOW);

    lastSecondi_Int -= lastSecondi_Int;
  }
}
