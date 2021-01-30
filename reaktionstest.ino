
#define BUTTONPIN 23
#define DEBOUNCETIME 20


#define DATA_PIN     22  // DS Pin of 74HC595(Pin14)
#define LATCH_PIN    1   // ST_CP Pin of 74HC595(Pin12)
#define CLOCK_PIN    3   // CH_CP Pin of 74HC595(Pin11)


#define STATUS_WAIT  0
#define STATUS_COUNTDOWN  1
#define STATUS_ACTION  2

#define VALUE_HIHO  -1
#define VALUE_FOUL  -2
#define VALUE_OFF  0

#define BAR_LED_COUNT 10

#define MIN_WAIT 3000
#define MAX_WAIT 7000

#define DELAY_MILLIS 1
#define DELAY_US 4

volatile int numberOfButtonInterrupts = 0;
volatile bool lastState;
volatile uint32_t debounceTimeout = 0;


TaskHandle_t displayTask;
TaskHandle_t buttonReadTask;
TaskHandle_t countDownTask = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

const int barPins[] = { 16, 17, 13, 12, 14, 27, 26, 25, 33, 32 };
const int digitPin[] = {21, 19, 18, 5};        // Define 7-segment display common pin

//                    0     1     2     3     4     5     6     7     8     9     A     b     C    d      E     F     H     U     L
unsigned char num[]={0xfc, 0x60, 0xdb, 0xf3, 0x67, 0xb7, 0xbf, 0xe0, 0xfe, 0xf7, 0xef, 0x3f, 0x9c, 0x7b, 0x9e, 0x8e, 0x6e, 0x7c, 0x1c};


volatile int countdownValue = 0;
volatile int value = 0;
volatile int state = STATUS_WAIT;
volatile uint32_t startTime = 0;

void setup() {
  // Serial.begin(115200); 

   for (int i=0; i<10; i++) {
      pinMode(barPins[i], OUTPUT);
   }
       
   for(int i=0;i<4;i++){       
       pinMode(digitPin[i],OUTPUT);
       digitalWrite(digitPin[i],HIGH);
   }
    
   pinMode(DATA_PIN, OUTPUT);
   pinMode(LATCH_PIN, OUTPUT);
   pinMode(CLOCK_PIN, OUTPUT);
      
   xTaskCreatePinnedToCore(display, "display", 10000, NULL, 0, &displayTask, 0);     
   xTaskCreatePinnedToCore(buttonRead, "button", 10000, NULL, 0, &buttonReadTask, 0);     
}

void loop() {
  value=VALUE_HIHO;
  vTaskDelay(1000000 / portTICK_PERIOD_MS);     
}



void buttonPressed() {
  // Serial.printf("Klick\n");
   if (state == STATUS_WAIT) {
       startCountDown();
   }
   else if (state == STATUS_COUNTDOWN) {
//     Serial.printf("Foul!\n");
       value = VALUE_FOUL;
       state = STATUS_WAIT;
       for (int i=0; i<10; i++) {
            switchAll(HIGH);
            vTaskDelay(60 / portTICK_PERIOD_MS);     
            switchAll(LOW);
            vTaskDelay(60 / portTICK_PERIOD_MS);     
       }
   }
   else if (state == STATUS_ACTION) {
       value = xTaskGetTickCount() - startTime;       
       state = STATUS_WAIT;
    //   Serial.printf("Zeit= %d!\n", value);
   }
}

void startCountDown() {  
    xTaskCreatePinnedToCore(doCountDownTask, "display", 10000, NULL, 0, &countDownTask, 0);          
}

void doCountDownTask( void * parameter) {
     Serial.printf("Start countdown\n");
     state = STATUS_COUNTDOWN;
     
     switchAll(LOW);
      
     for(int i=0; i<BAR_LED_COUNT && (state == STATUS_COUNTDOWN); i++) {
        vTaskDelay(300 / portTICK_PERIOD_MS);     
        digitalWrite(barPins[i], HIGH);         
      }
      if (state == STATUS_COUNTDOWN) {
          unsigned int waitCount = randomWaitMillis()/10;
          for(int i=0; i<waitCount && (state == STATUS_COUNTDOWN); i++) {          
              vTaskDelay(10 / portTICK_PERIOD_MS);     
          }
      }
      switchAll(LOW);
      if (state == STATUS_COUNTDOWN) {
          state = STATUS_ACTION;
          startTime = xTaskGetTickCount();
      }
      else {
          state = STATUS_WAIT;
      }
    //  Serial.printf("Exit countdown\n");
      vTaskDelete(countDownTask);
}


unsigned int randomWaitMillis() {
   return MIN_WAIT + (rand() % (MAX_WAIT - MIN_WAIT)); 
}

void switchAll(uint8_t value) {
  
    for(int i=0; i<10; i++) {
        digitalWrite(barPins[i], value);         
    }
}

void display( void * parameter ) {
  while(1) {
     int showValue = value;             
          //Serial.printf("showValue= %d!\n", value);
     if (showValue>=0) {
       showDigit(0,3, num[showValue%10000/1000]);    
       showDigit(1,0, num[showValue%1000/100]);        
       showDigit(2,1, num[showValue%100/10]);    
       showDigit(3,2, num[showValue%10]);    
     }
     else if (showValue==-1) {
       showDigit(0,3, num[16]);
       showDigit(1,0, num[1]);
       showDigit(2,1, num[16]);
       showDigit(3,2, num[0]);
     }
     else if (showValue==-2) {
       showDigit(0,3, num[15]);
       showDigit(1,0, num[0]);
       showDigit(2,1, num[17]);
       showDigit(3,2, num[18]);
     }
     else if (showValue==-3) {
       showDigit(0,3, 255);
       showDigit(1,0, 255);
       showDigit(2,1, 255);
       showDigit(3,2, 255);
     }    
     
  }
}


void IRAM_ATTR handleButtonInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  numberOfButtonInterrupts++;
  lastState = digitalRead(BUTTONPIN);
  debounceTimeout = xTaskGetTickCount(); //version of millis() that works from interrupt
  portEXIT_CRITICAL_ISR(&mux);
}

void buttonRead( void * parameter) {
  
  pinMode(BUTTONPIN, INPUT);
  pinMode(BUTTONPIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTONPIN), handleButtonInterrupt, FALLING);

  uint32_t saveDebounceTimeout;
  bool saveLastState;
  int save;

  // Enter RTOS Task Loop
  while (1) {

    portENTER_CRITICAL_ISR(&mux);
    save  = numberOfButtonInterrupts;
    saveDebounceTimeout = debounceTimeout;
    saveLastState  = lastState;
    portEXIT_CRITICAL_ISR(&mux); 
    
    if ( (millis() - saveDebounceTimeout) > DEBOUNCETIME) {

      bool currentState = digitalRead(BUTTONPIN);

      if ((save != 0) && (currentState == saveLastState)) { 

        buttonPressed();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        portENTER_CRITICAL_ISR(&mux); 
        numberOfButtonInterrupts = 0; 
        portEXIT_CRITICAL_ISR(&mux);      
      }

      vTaskDelay(1 / portTICK_PERIOD_MS);
    }
  }
}

void writeValue(int value) {
   
    for(int i=0; i<8; i++) {
      digitalWrite(CLOCK_PIN, LOW);
      ets_delay_us(DELAY_US);
      digitalWrite(DATA_PIN, (0x01 & (value>>i)) ? HIGH : LOW ); 
      ets_delay_us(DELAY_US);
      digitalWrite(CLOCK_PIN, HIGH); 
      ets_delay_us(DELAY_US);
    }
}

void displayValue(int value) {
    
    digitalWrite(LATCH_PIN, LOW);
    
    writeValue(value);    
    
    digitalWrite(LATCH_PIN, HIGH);  
    
}

void showDigit(int on, int off, int value){    
    digitalWrite(digitPin[off], HIGH);    
    displayValue(value);
    digitalWrite(digitPin[on], LOW);    
    vTaskDelay(DELAY_MILLIS / portTICK_PERIOD_MS); 
}
    
void allDigitsOff(){    
    digitalWrite(digitPin[0], HIGH);
    digitalWrite(digitPin[1], HIGH);
    digitalWrite(digitPin[2], HIGH);
    digitalWrite(digitPin[3], HIGH);
}
