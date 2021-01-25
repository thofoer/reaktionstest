

TaskHandle_t blinkerTask;

const int  barPins[] = { 12, 14, 27, 26, 25, 33, 32, 13, 17, 16 };

void setup() {
   Serial.begin(115200); 

   for (int i=0; i<10; i++) {
      pinMode(barPins[i], OUTPUT);
   }
  
   xTaskCreatePinnedToCore(
      blinker, 
      "blinker",
      10000,  
      NULL,  
      0,  
      &blinkerTask,
      0);     
}

void loop() {

  for (int i=0; i<10; i+=2) {
      digitalWrite(barPins[i], HIGH);
  }   
  delay(500);
  
  for (int i=0; i<10; i+=2) {
      digitalWrite(barPins[i], LOW);
  }
  delay(500);
}


void blinker( void * parameter) {
  for(;;) {
    for (int i=1; i<10; i+=2) {
      digitalWrite(barPins[i], HIGH);
    }
   
    delay(331);
    for (int i=1; i<10; i+=2) {
      digitalWrite(barPins[i], LOW);
    }
    delay(331);   
  }
}
