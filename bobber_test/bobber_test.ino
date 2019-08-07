void ICACHE_RAM_ATTR handleBobberInterrupt();   //!!! important for external interrupt for ESP8266
void ICACHE_RAM_ATTR handleRainInterrupt();   //!!! important for external interrupt for ESP8266

const byte pinBobberSensor = D1;
const byte pinRainSensor = D2;
const int analogPin = A0;
int thresholdValue = 500;   // value of treshhold to change isRain status
boolean isRain = false;
volatile boolean checkBobberInterrupt = false;
volatile boolean checkRainInterrupt = false;
uint16_t analogPinValue = 0;
int prevBobberStatus = -1;

unsigned long debounceTime = 1000;
unsigned long lastBobberDebounce = 0;
unsigned long lastRainDebounce = 0;

void handleBobberInterrupt() {
  checkBobberInterrupt= true;
}

void handleRainInterrupt() {
  checkRainInterrupt= true;
}

void setup() {

  Serial.begin(115200);
  pinMode(pinBobberSensor, INPUT_PULLUP);
  pinMode(pinRainSensor, INPUT_PULLUP);
  pinMode(analogPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(pinBobberSensor), handleBobberInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinRainSensor), handleRainInterrupt, CHANGE);
  Serial.println("");
  Serial.println("started ... ");
}

void loop(){
/*  if(!digitalRead(pinSensor)){       // if there is zero value on a pin
    Serial.println("CEHCOP TOHET");  // print message
  }
  delay(200);*/
  // check if rain is started/stopped
  if(checkBobberInterrupt == true && ( (millis() - lastBobberDebounce)  > debounceTime )){
    lastBobberDebounce = millis();
    checkBobberInterrupt = false;
 //     isRain ? MQTTclient.publish(outTopic, "StopAll") : MQTTclient.publish(outTopic, "StartAll");
    Serial.println("the change of pump status detected ");
    int currBobberStatus = digitalRead(pinBobberSensor);
    if (currBobberStatus != prevBobberStatus){
      prevBobberStatus = currBobberStatus;
      if(!currBobberStatus)       // if there is zero value on a pin
        Serial.println("Start the pump"); 
      else
        Serial.println("Stop the pump"); 
//      Serial.println("the bobber status was really detected ");
    }
  }
/*  else
    checkInterrupt = false; */

   if(checkRainInterrupt == true && ( (millis() - lastRainDebounce)  > debounceTime )){
    if(!digitalRead(pinRainSensor))       // if there is non zero value on a rain pin
      Serial.println("DIGITAL -- It is raining"); 
    else
      Serial.println("DIGITAL -- It is stop raining"); 
    lastRainDebounce = millis();
    checkRainInterrupt = false;
    // read information from analog PIN
    analogPinValue = analogRead(analogPin);
    Serial.print("Analog pin = "); 
    Serial.println(analogPinValue); 
    if (analogPinValue < thresholdValue)
      Serial.println("it is Wet");
    else 
      Serial.println("it is Dry");
//    if sensorValue >= thresholdValue  it is Dry
/*    if ((sensorValue < thresholdValue && !isRain) || (sensorValue >= thresholdValue && isRain)){
      isRain = !isRain;*/
 //     isRain ? MQTTclient.publish(outTopic, "StopAll") : MQTTclient.publish(outTopic, "StartAll");
      Serial.println("the change of rain status detected ");
    }
}
