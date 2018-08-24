#include <Arduino.h>
#include <AH_EasyDriver.h>
#include <OneWire.h>

const byte temp1Pin = 8; // Temperaturfühler Daten Pin
const byte temp2Pin = 9; // Temperaturfühler Daten Pin
const byte oberU = 6; // Oberwasser Unterer Melder
const byte oberO = 7; // Oberwasser Oberer Melder
const byte unterU = 10; // Unterwasser Unterer Melder
const byte unterO = 11; // Unterwasser Oberer Melder
const byte schranke = 2; // Lichtschrankendatenpin
const byte endschalter = 3; // Endschalter Interrupt Pin
const byte RES = 200; // Resolution vom Stepper Motor
const byte DIR = 12; // Direction Pin am Stepper Driver
const byte STEP = 4; // Step am Stepper Driver
const byte ENABLE = 5; // Enable Pin am Stepper Driver
const int dauer = 60000; // Eine Minute lang zaehlen

unsigned long zeit = 0; // millis Zeit tmp var
unsigned int count = 0; // gezaehlte Unterbrechungen

boolean state = false; // Visuelle Hilfe bei RPM Messung (interne LED Pin 13)
boolean end = false; // Variable true wenn Endschalter betätigt ist

OneWire temp1(temp1Pin); // Temperaturfühler 1 initialisierung
OneWire temp2(temp2Pin); // Temperaturfühler 2 initialisierung

AH_EasyDriver stepper(RES,DIR,STEP,0,0,0,ENABLE,0); // RES,DIR,STEP,MS1,MS2,SL,ENABLE,RST

int incomingByte = 0; // for incoming serial data

void setup() {
  stepper.disableDriver();
  pinMode(oberU, INPUT_PULLUP);
  pinMode(oberO, INPUT_PULLUP);
  pinMode(unterU, INPUT_PULLUP);
  pinMode(unterO, INPUT_PULLUP);
  pinMode(schranke, INPUT);
  pinMode(temp1Pin, INPUT);
  pinMode(temp2Pin, INPUT);
  pinMode(13, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(endschalter), ende, FALLING);
  Serial.begin(115200);
}

void loop() {
  receiveSerial();
}

void receiveSerial() {
  if (Serial.available() > 0)  {
    incomingByte = Serial.read();
    switch (incomingByte) {
      case 116: //t Temperatur
        Serial.print("temp1: " + String(readTemperatureAtDevice(temp1)));
        Serial.println(" | temp2: " + String(readTemperatureAtDevice(temp2)));
        break;
      case 114: //r RPM
        rpm();
        break;
      case 118: //v Vibration
        Serial.println("Vibration");
        delay(100);
        break;
      case 111: //o Oberwasser
        Serial.println(oberwasser(false));
        break;
      case 117: //u Unterwasser
        Serial.println(unterwasser());
        break;
      case 43: //+ Motor plus
        Serial.println(oberwasserRegulierung(2));
        break;
      case 45: //- Motor minus
        Serial.println(oberwasserRegulierung(1));
        break;
      case 109: //m OberwasserRegulierung mit Motordrehung
        Serial.println(oberwasser(true));
        break;
      default: // Unbekannt
        Serial.println(String(incomingByte) + " is not a known command");
        break;
    }
  }
}

float readTemperatureAtDevice(OneWire temp_) {
   //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !temp_.search(addr)) {
      //no more sensors on chain, reset search
      temp_.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  temp_.reset();
  temp_.select(addr);
  temp_.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = temp_.reset();
  temp_.select(addr);
  temp_.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = temp_.read();
  }

  temp_.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;
}

String oberwasser(boolean motor) {
  stepper.enableDriver();
  if((digitalRead(oberU) == LOW) && (digitalRead(oberO) == LOW)) { // Wassermelder unten ist Unten (AN) -> Drehe Motor
    if (motor) Serial.println(oberwasserRegulierung(2));
    return "Oberwasser niedrig";
  }
  else if((digitalRead(oberO) == HIGH) && (digitalRead(oberU) == HIGH)) { // Wassermelder oben ist Oben (AUS) -> Drehe Motor
    if (motor) Serial.println(oberwasserRegulierung(1));
    return "Oberwasser hoch";
  }
  else if((digitalRead(oberU) == HIGH) && (digitalRead(oberO) == LOW)) { // Wasserstand
    if (motor) Serial.println(oberwasserRegulierung(0));
    return "Oberwasser normal";
  }
  else return "Oberwasser unmöglich";
}

String oberwasserRegulierung(int x) {
  if (digitalRead(endschalter) == LOW) return "Endschalter betätigt, Fahre nicht";
  else {
    stepper.enableDriver();
    switch(x) {
      case 2:
        stepper.move(740,BACKWARD); // Der eine Motor ist über 180° gefahren / 800 Schritte
        return "Fahre halbe Umdrehung zurück";
        break;
      case 1:
        stepper.move(740,FORWARD); // Der eine Motor ist über 180° gefahren / 800 Schritte
        return "Fahre halbe Umdrehung vor";
        break;
      case 0:
        return "Fahre nicht";
        break;
      default:
        return "Fahre nicht";
        break;
    }
  }
  stepper.disableDriver();
}

String unterwasser() {
  int level = 0;
  if(digitalRead(unterO) == LOW) level++;
  if(digitalRead(unterU) == LOW) level++;
  else {
    if(digitalRead(unterO) == LOW) level = -1;
  }
  return "Unterwasser Level " + String(level) + " / 2";
}

void rpm() {
  Serial.println("Beginne RPM Messung");
  count = 0;
  zeit = millis();
  attachInterrupt(digitalPinToInterrupt(schranke), umdrehung, RISING);
  while (millis() - zeit < dauer) {
    digitalWrite(13, state);
    yield();
  }
  Serial.println(String(count) + " RPM");
  detachInterrupt(digitalPinToInterrupt(schranke));
}

void umdrehung() {
  count++;
  state = !state;
}

void ende() {
  stepper.disableDriver();
}
