void ICACHE_RAM_ATTR handleInterrupt();   //!!! important for external interrupt for ESP8266

const byte interruptPin = 2;
volatile byte interruptCounter = 0;
int numberOfInterrupts = 0;

void setup() {
 
  Serial.begin(115200);
  pinMode(interruptPin, INPUT_PULLUP);
  Serial.println("\nS.T.A.R.T. ");
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
 
}
 
void handleInterrupt() {
  interruptCounter++;
}
 
void loop() {
 
  if(interruptCounter>0){
 
      interruptCounter--;
      numberOfInterrupts++;
 
      Serial.print("An interrupt has occurred. Total: ");
      Serial.println(numberOfInterrupts);
  }
 
}
