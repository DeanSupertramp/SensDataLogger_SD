#include <SPI.h>
#include <SD.h>
#include <DS1302.h>
#include <Wire.h>

// Creazione oggetto RTC
DS1302 rtc(13, 11, 6); // (RST, DAT, CLK)

// Set the pins used
#define cardSelect 4

File logfile;

#define VBATPIN A9
float measuredvbat = 0;

// -------------------------------------- INA169 --------------------------------------
// Constants
const int SENSOR_PIN = A0;  // Input pin for measuring Vout
const float RS = 0.1;          // Shunt resistor value (in ohms)
const float RL = 10000;          // Load resistor value (in ohms)
const float VOLTAGE_REF = 3.3;  // Reference voltage for analog read

// Global Variables
float sensorValue_RAW, sensorValue;   // Variable to store value from analog read
float current;       // Calculated current value

void readINA_current() {
  // Read a value from the INA169 board
  sensorValue_RAW = analogRead(SENSOR_PIN);

  // Remap the ADC value into a voltage number
  sensorValue = (sensorValue_RAW * VOLTAGE_REF) / 1023;

  // Is = (Vout x 1k) / (RS x RL)
  current = sensorValue *1000/ (RS * RL);
}

void Vbat() {
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
}

// blink out an error code
void error(uint8_t errno) {
  while (1) {
    uint8_t i;
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
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println("\r\nAnalog logger test");
  pinMode(13, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    Serial.println("Card init. failed!");
    error(2);
  }
  char filename[15];
  strcpy(filename, "/ANALOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[7] = '0' + i / 10;
    filename[8] = '0' + i % 10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
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

  // Imposta RTC in run-mode e disabilita' la protezione da scrittura
  rtc.halt(false);
  rtc.writeProtect(false);

  // Imposta la comunicazione seriale
  Serial.begin(9600);

  // LImpostazione valori nella memori del DS1302
  rtc.setDOW(WEDNESDAY);        // Imposta il giorno della settimana a SUNDAY
  rtc.setTime(13, 0, 0);     // Imposta l'ora come 11:32:00 (Formato 24hr)
  rtc.setDate(16, 02, 2022);   // Imposta la data come 7 novembre 2020

  logfile.print("Date"); logfile.print(","); logfile.print("Time"); logfile.print(","); logfile.print("VBat"); logfile.print(","); logfile.println("Current");
}

uint8_t i = 0;
char minuti[2];
char minuti2[2];

void loop() {
  digitalWrite(8, HIGH);
  Vbat();
  readINA_current();
  // Invia giorno della settimana
  // Serial.print(rtc.getDOWStr());    Serial.print(" ");
  // Invia data e ora
  Serial.print(rtc.getDateStr()); Serial.print(", "); Serial.print(rtc.getTimeStr()); Serial.print(", "); Serial.print(measuredvbat); Serial.print(", "); Serial.println(current, 3);

  Serial.print("ORA: "); Serial.print(rtc.getTimeStr()[0]); Serial.println(rtc.getTimeStr()[1]);
  Serial.print("MIN: "); Serial.print(rtc.getTimeStr()[3]); Serial.println(rtc.getTimeStr()[4]);
  Serial.print("SEC: "); Serial.print(rtc.getTimeStr()[6]); Serial.println(rtc.getTimeStr()[7]);
  
//  minuti[0]=rtc.getTimeStr()[6];
//  minuti[1]=rtc.getTimeStr()[7];
//  memcpy(minuti2, &minuti[3], 2*sizeof(*minuti));
//
//  Serial.print("CONVERTITI= "); Serial.println(minuti2);
  
  logfile.print(rtc.getDateStr()); logfile.print(","); logfile.print(rtc.getTimeStr()); logfile.print(","); logfile.print(measuredvbat); logfile.print(","); logfile.println(current, 3);
  logfile.flush();
  digitalWrite(8, LOW);

  delay(1000);
}
