#include <SPI.h>
#include <SD.h>
#include <DS1302.h>
#include <Wire.h>

// Creazione oggetto RTC
DS1302 rtc(13, 11, 6);    // (RST, DAT, CLK)

// Set the pins used
#define cardSelect 4

File logfile;

#define VBATPIN A9
float measuredvbat = 0;

// -------------------------------------- INA169 --------------------------------------
const int SENSOR_PIN = A0;            // Input pin for measuring Vout
const float RS = 0.1;                 // Shunt resistor value (in ohms)
const float RL = 10000;               // Load resistor value (in ohms)
const float VOLTAGE_REF = 3.3;        // Reference voltage for analog read

// Global Variables
float sensorValue_RAW, sensorValue;   // Variable to store value from analog read
float current;                        // Calculated current value

uint8_t i, j = 0;
String minuti;
String minNow;
int minLast = 0;

float currentSum = 0.0;
float currentMean = 0.0;

void readINA_current() {
  sensorValue_RAW = analogRead(SENSOR_PIN);               // Read a value from the INA169 board
  sensorValue = (sensorValue_RAW * VOLTAGE_REF) / 1023;   // Remap the ADC value into a voltage number
  current = sensorValue * 1000 / (RS * RL);               // Is = (Vout x 1k) / (RS x RL)
  currentSum += current;
  if (j >= 60) {
    currentMean = currentSum / j;
    currentSum = 0;
    j = 0;
  }
}

float measuredvbatSum = 0.0;
float measuredvbatMean = 0.0;



void Vbat() {
  j++;
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;      // we divided by 2, so multiply back
  measuredvbat *= 3.3;    // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024;   // convert to voltage
  measuredvbatSum += measuredvbat;
  if (j >= 60) {
    measuredvbatMean = measuredvbatSum / j;
    measuredvbatSum = 0;
    j = 0;
  }
}

// blink out an error code
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
  Serial.println("\r\nAnalog logger test");
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
  rtc.setDOW(WEDNESDAY);        // Imposta il giorno della settimana a SUNDAY
  rtc.setTime(13, 0, 0);        // Imposta l'ora come 11:32:00 (Formato 24hr)
  rtc.setDate(16, 02, 2022);    // Imposta la data come 7 novembre 2020

  logfile.print("Date"); logfile.print(","); logfile.print("Time"); logfile.print(","); logfile.print("VBat"); logfile.print(","); logfile.println("Current");
}


void loop() {
  Vbat();
  readINA_current();
  // Serial.print(rtc.getDOWStr());    Serial.print(" ");     // Invia giorno della settimana
  // Invia data e ora
  Serial.print(rtc.getDateStr()); Serial.print(", "); Serial.print(rtc.getTimeStr()); Serial.print(", "); Serial.print(measuredvbatMean); Serial.print(", "); Serial.println(currentMean, 3);

  Serial.print("ORA: "); Serial.print(rtc.getTimeStr()[0]); Serial.println(rtc.getTimeStr()[1]);
  Serial.print("MIN: "); Serial.print(rtc.getTimeStr()[3]); Serial.println(rtc.getTimeStr()[4]);
  Serial.print("SEC: "); Serial.print(rtc.getTimeStr()[6]); Serial.println(rtc.getTimeStr()[7]);

  minuti = rtc.getTimeStr();
  minNow = minuti.substring(3, 5);

  Serial.print("CONVERTITI= "); Serial.println(minNow);
  int num = minNow.toInt();
  if (num - minLast >= 1) {
    digitalWrite(8, HIGH);
    minLast = num;
    logfile.print(rtc.getDateStr()); logfile.print(","); logfile.print(rtc.getTimeStr()); logfile.print(","); logfile.print(measuredvbatMean); logfile.print(","); logfile.println(currentMean, 3);
    logfile.flush();
    digitalWrite(8, LOW);
  }
}
