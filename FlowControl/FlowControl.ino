#include <AH_EasyDriver.h>

const byte wasserU = 6;
const byte wasserO = 7;

//AH_EasyDriver(int RES, int DIR, int STEP, int MS1, int MS2, int SLP, int ENABLE, int RST);
AH_EasyDriver stepper(200,3,4,0,0,0,5,0);
//AH_EasyDriver stepper(200,3,4);


void setup() {
  stepper.disableDriver();
  stepper.setMicrostepping(0);
  pinMode(wasserU, INPUT);
  pinMode(wasserO, INPUT);
}

void loop() {
  if((digitalRead(wasserU) == HIGH) && (digitalRead(wasserO) == HIGH)) { // Wassermelder unten ist Unten (AN) -> Drehe Motor x
    stepper.enableDriver();
    stepper.move(740,BACKWARD);
    stepper.disableDriver();
  }
  else if((digitalRead(wasserO) == LOW) && (digitalRead(wasserU) == LOW)) { // Wassermelder oben ist Oben (AUS) -> Drehe Motor x
    stepper.enableDriver();
    stepper.move(740,FORWARD);
    stepper.disableDriver();
  }
  delay(1000);
}
