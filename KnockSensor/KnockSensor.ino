const int knockSensor = A0;
byte x[1000];

void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    for(int i = 0; i < 1000; i++) {
      x[i] = analogRead(knockSensor);
      delayMicroseconds(100);
    }
    for(int i = 0; i < 1000; i++) {
      Serial.println(map(x[i],0,1023,-512,512)+344);
    }
  }
}
