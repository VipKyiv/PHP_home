void ICACHE_RAM_ATTR handleBobberInterrupt();   //!!! important for external interrupt for ESP8266

const byte pinBobberSensor = 5;  // D1

volatile boolean checkBobberInterrupt = false;
int prevBobberStatus = -1;

// to avoid accidental operation - add interval for debounce
uint32_t debounceTime = 3000;
uint32_t lastBobberDebounce = 0;

void handleBobberInterrupt() {
  checkBobberInterrupt= true;
}

void setup() {

  Serial.begin(115200);
  Serial.println("\nstarted ... ");
  pinMode(pinBobberSensor, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pinBobberSensor), handleBobberInterrupt, CHANGE); // CHANGE or FALLING or RISING  
}

void loop(){
  // if bobber status is changed
  if(checkBobberInterrupt == true && ( (millis() - lastBobberDebounce)  > debounceTime )){
    lastBobberDebounce = millis();
    checkBobberInterrupt = false;
    int currBobberStatus = digitalRead(pinBobberSensor);
    if (currBobberStatus != prevBobberStatus){   // to avoid double messages
      prevBobberStatus = currBobberStatus;
      if(!currBobberStatus)       // if there is zero value on a pin
        Serial.println("low water levev - send a message"); 
      else
        Serial.println("Stop the pump"); 
    }
  }
}
