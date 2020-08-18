#include "initMicrocontroller.h"

void ICACHE_RAM_ATTR handleBobberInterrupt();   //!!! important for external interrupt for ESP8266

// Configure mode flag (if True - create AP with web server to configure wifi and mqtt server)
bool flagConfigure = false;

boolean isBottom = false;  
volatile boolean checkBobberInterrupt = false;
int prevBobberStatus = -1;
 
uint64_t debounceTime = 3000;
uint64_t lastBobberDebounce = 0;

//const int pinLED = 2;

const byte pinBobber = 12;   // 

char* pgName = __FILE__;

int sensNumber = 0;
int sensID[MAX_SENSORS];
int sensInterval[MAX_SENSORS] = {0};

int devNumber = 0;
int devID[MAX_DEVICES];
int devDuration[MAX_DEVICES] = {0};
int devPin[MAX_DEVICES] = {5};
uint64_t devFinishTime[MAX_DEVICES] = {0};

uint64_t restartInterval = 60000;
uint64_t lastRestartCheck = 0;

uint64_t webRestartInterval = 5*60000;
uint64_t webLastRestartCheck = 0;

char msg[50];

void handleBobberInterrupt() {
  checkBobberInterrupt = true;
}  // handleBobberInterrupt

void setupPins() {
// Initialize the Relays pin as an output
  for (uint8_t i = 0; i < devNumber; i++) {
    pinMode(devPin[i], OUTPUT);     
    digitalWrite(devPin[i], HIGH);
    
  }

//Set pinBobber, pulling pin to logical «1» via  pull-down resistor
  pinMode(pinBobber, INPUT_PULLUP);         
  
}    // setupPins

void checkEEPROM() {
     EEPROM.begin(512);
     printDebug("Signature in EEPROM: ");
     printlnDebug(EEPROM.read(0));
// Check if we have stored configuration data
     if (EEPROM.read(0) != EEPROM_MASK) {
    // No parameters in EEPROM, start configuration mode
       flagConfigure = true;
       printlnDebug("No parameters in EEPROM, start configuration mode");
       startWebServer();
     }
     else {
       readParamFromEEPROM();
       if (!connectWiFi() or !connectMQTT() or !initMC()) {
          flagConfigure = true;
          printlnDebug("\nconnection error, start configuration mode");
          startWebServer();
       }
     }
}  // checkEEPROM


void callback(char* topic, byte* payload, unsigned int length) {

  String strTopic = String(topic);
  printlnDebug("Message arrived [" + strTopic + "] ");
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  printlnDebug(strPayload);
  if ( strPayload == "STOP" )  // send message to delete cron task
    printlnDebug("S.T.O.P");


  // Switch on the LED if an 1 was received as first character
  int iPos = strTopic.lastIndexOf('/');
  if( iPos > 0){
    String devTopic = strTopic.substring(iPos + 1);
    for (uint8_t i = 0; i < devNumber; i++){
      if (devID[i] == devTopic.toInt()){
        if ( strPayload == "Begin" or strPayload == "Start") {
          digitalWrite(devPin[i], LOW);
          if (devDuration[i] > 0 )
              devFinishTime[i] = (uint64_t)(millis() + devDuration[i] * 1000);
          sendMqttMessages((char *)MQTT_DEVICE_PREFIX, devID[i],"SwitchON");
        } 
        else if ( strPayload == "Finish" or strPayload == "Stop"){
          digitalWrite(devPin[i], HIGH);
          devFinishTime[i] = 0;
          sendMqttMessages((char *)MQTT_DEVICE_PREFIX, devID[i],"SwitchOFF");
        }
        break;
      }
    }
  }
  
} // callback

void setup() {
  Serial.begin(115200);
  generateMqttName();
  printlnDebug("\nhello  ");
  printDebug("Out topic ");
  printlnDebug(outTopic);
  checkEEPROM();
  if (!flagConfigure) {   // normal mode (not a local web server)  
    setupPins();
    mqttClient.setCallback(callback);
// set interrupt and handle function for bobber 
    attachInterrupt(digitalPinToInterrupt(pinBobber), handleBobberInterrupt, CHANGE); // CHANGE or FALLING or RISING  
// send start MQTT message
    char startMsg[32];
    sprintf(startMsg, "<%s> STARTS", mqttClientName);
    sendMqttMessages(outTopic, (char *)"mcStart", startMsg);
    if (digitalRead(pinBobber) == LOW)
      sendMqttMessages((char *)MQTT_SENSOR_PREFIX, sensID[0],"Low");
  }

}

void loop() {
  if (flagConfigure) {   // when local web server is started
    server.handleClient();
    if ( webLastRestartCheck == 0)
        webLastRestartCheck = millis();
    else if ((uint64_t)(millis() - webLastRestartCheck) > webRestartInterval ) 
         ESP.restart();    // restart after webRestartInterval
  }
  else {
    if (!mqttClient.connected()) {
      if ( lastRestartCheck == 0)
        lastRestartCheck = millis();
      else if ((uint64_t)(millis() - lastRestartCheck) > restartInterval ) 
         ESP.restart();    // restart if no connect more than Interval
         
      if (WiFi.status() != WL_CONNECTED) {
        printlnDebug("#####  trouble with WIFI connection ######");
      }
      else 
      printlnDebug("##### WIFI connected ######");
      printlnDebug("reconnect MQTT");
      connectMQTT();
    }
    else 
      lastRestartCheck = 0;
    
    mqttClient.loop();
    uint64_t currentMillis = millis(); // grab current time
  // if bobber status is changed
    if(checkBobberInterrupt == true && ( (currentMillis - lastBobberDebounce)  > debounceTime )){
      lastBobberDebounce = currentMillis;
      checkBobberInterrupt = false;
      int currBobberStatus = digitalRead(pinBobber);
      if (currBobberStatus != prevBobberStatus){   // to avoid double messages
        prevBobberStatus = currBobberStatus;
        if(!currBobberStatus) {      // if there is non zero value on a pin
          printlnDebug("low water level - send a message"); 
//          digitalWrite(5, LOW);
          sendMqttMessages((char *)MQTT_SENSOR_PREFIX, sensID[0],"Low");
        }
        else {
          printlnDebug("Stop the pump"); 
//          digitalWrite(5, HIGH);
          sendMqttMessages((char *)MQTT_SENSOR_PREFIX, sensID[0],"High");
        }
      }
    }
    for (uint8_t i = 0; i < devNumber; i++) {  // check duration
      if ( devFinishTime[i] and devFinishTime[i] <= currentMillis) {
          digitalWrite(devPin[i], HIGH);
          devFinishTime[i] = 0;
          sendMqttMessages((char *)MQTT_DEVICE_PREFIX, devID[i],"SwitchOFF");
      }
    }
      
  } 
}
