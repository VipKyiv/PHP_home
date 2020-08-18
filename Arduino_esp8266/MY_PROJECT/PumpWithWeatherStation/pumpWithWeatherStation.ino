#include "initMicrocontroller.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_ADS1015.h>

// void ICACHE_RAM_ATTR handleInterrupt();   //!!! important for external interrupt for ESP8266

// Configure mode flag (if True - create AP with web server to configure wifi and mqtt server)
bool flagConfigure = false;

char* pgName = __FILE__;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(14);    // hard code temperature sensor PIN
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensorTemperature(&oneWire);

Adafruit_ADS1115 ads;  // Use this for the 16-bit version 
 
int sensNumber = 0;
int sensID[MAX_SENSORS];
int sensInterval[MAX_SENSORS] = {0};
char sensPortType[MAX_SENSORS];
int sensPortNumber[MAX_SENSORS];

uint64_t prevMillis[MAX_SENSORS] = {0};

int devNumber = 0;
int devID[MAX_DEVICES];
int devDuration[MAX_DEVICES] = {0};
char devPortType[MAX_SENSORS];
int devPortNumber[MAX_SENSORS];
int devPin[MAX_DEVICES] = {12};           
uint64_t devFinishTime[MAX_DEVICES] = {0};

uint64_t restartInterval = 60000;
uint64_t lastRestartCheck = 0;

uint64_t webRestartInterval = 5*60000;
uint64_t webLastRestartCheck = 0;

char msg[50];

void setupPins() {
// Initialize the Relays pin as an output
  for (uint8_t i = 0; i < devNumber; i++) {
    pinMode(devPin[i], OUTPUT);     
    digitalWrite(devPin[i], LOW);
    
  }
  pinMode(2, OUTPUT);     
  digitalWrite(2, HIGH);

//Set pinBobber, pulling pin to logical «1» via  pull-down resistor
//  pinMode(sensorPin, INPUT);
//  digitalWrite(sensorPin, HIGH);
  
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
          digitalWrite(devPin[i], HIGH);
          digitalWrite(2, LOW);
          if (devDuration[i] > 0 )
              devFinishTime[i] = (uint64_t)(millis() + devDuration[i] * 1000);
          sendMqttMessages((char *)MQTT_DEVICE_PREFIX, devID[i],"SwitchON");
        } 
        else if ( strPayload == "Finish" or strPayload == "Stop"){
          digitalWrite(devPin[i], LOW);
          digitalWrite(2, HIGH);
          devFinishTime[i] = 0;
          sendMqttMessages((char *)MQTT_DEVICE_PREFIX, devID[i],"SwitchOFF");
        }
        break;
      }
    }
  }
  
} // callback

char* readSensorInfo(const char type, const int port)
{
  char returnMessage[10];
  if (type == 'A') 
    snprintf (returnMessage, 10, "%d", analogRead(port));
  else if (type == 'E') {
    int16_t tempADC = ads.readADC_SingleEnded(port);  
    tempADC = tempADC > 0 ? tempADC : 0;
    float percent = (tempADC * 100.00) / 27000.00;
    snprintf(returnMessage, 10, "%6.2f", percent);
  }
  else if (type == 'T') {
    sensorTemperature.requestTemperatures(); 
    //float temperatureC = sensorTemperature.getTempCByIndex(0);
    snprintf(returnMessage, 10, "%6.2f", sensorTemperature.getTempCByIndex(0));   
  }
  else
    snprintf(returnMessage, 10, "%s", digitalRead(port)); 
//    return digitalRead(port); 
  return returnMessage;   
}


void setup()
{
  Serial.begin(115200);
  generateMqttName();
  printlnDebug("\nhello  ");
  printDebug("Out topic ");
  printlnDebug(outTopic);
  checkEEPROM();
  if (!flagConfigure) {   // normal mode (not a local web server)  
    setupPins();
    mqttClient.setCallback(callback);

    sensorTemperature.begin();   // Start the DS18B20 sensor
    //   ads  
    ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
//  ads.setGain(GAIN_TWOTHIRDS);
    ads.begin();
// send start MQTT message
    char startMsg[32];
    sprintf(startMsg, "<%s> STARTS", mqttClientName);
    sendMqttMessages(outTopic, (char *)"mcStart", startMsg);
  }

}
 
void loop()
{
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
  // sensors processing    
     for (uint8_t i = 0; i < sensNumber; i++) {  // check interval {
      if ( sensInterval[i] > 0) {
        uint64_t intervalMillis = sensInterval[i] * 60 * 1000;
        if ((uint64_t)(currentMillis - prevMillis[i]) >= intervalMillis ) {
          prevMillis[i] = currentMillis;
          //sprintf(msg, "%d", readSensorInfo(sensPortType[i], sensPortNumber[i]));
          sendMqttMessages((char *)MQTT_SENSOR_PREFIX, sensID[i],readSensorInfo(sensPortType[i], sensPortNumber[i]));
        }
      }
      
    }
  
  // devices processing    
    for (uint8_t i = 0; i < devNumber; i++) {  // check duration
      if ( devFinishTime[i] and devFinishTime[i] <= currentMillis) {
          digitalWrite(devPin[i], LOW);
          devFinishTime[i] = 0;
          sendMqttMessages((char *)MQTT_DEVICE_PREFIX, devID[i],"SwitchOFF");
      }
    }
      
  } 
 
}
 
