#include "initMicrocontroller.h"
// based on https://www.electroniclinic.com/water-flow-sensor-arduino-water-flow-rate-volume-measurement/?fbclid=IwAR0wn_NZsOsbTOwuHiukUTtvOfdfdDp1IyTKtCU2iWZv1BScYFU62iRorg8#Water_Flow_Sensor_Calculation

void ICACHE_RAM_ATTR pulseCounter();

// Configure mode flag (if True - create AP with web server to configure wifi and mqtt server)
bool flagConfigure = false;

char* pgName = __FILE__;

int sensorPin = 5; // waterMeasurement sensor
int sensorInterrupt = digitalPinToInterrupt(sensorPin); 
 
/*The hall-effect flow sensor outputs pulses per second per litre/minute of flow.*/
//float calibrationFactor = 90; //You can change according to your datasheet
const float calibrationFactor = 7.5; //You can change according to your datasheet
 
volatile byte pulseCount =0;  
 
float flowRate = 0.0;
uint64_t flowMilliLitres =0;
uint64_t totalMilliLitres = 0;
uint64_t oldTime = 0;
uint64_t prevMillisWaterFlow = 0;

int flowLitres =0;
int totalLitres = 0;


int sensNumber = 0;
int sensID[MAX_SENSORS];
int sensInterval[MAX_SENSORS] = {0};
uint64_t intervalMillisWaterFlow;

int devNumber = 0;
int devID[MAX_DEVICES];
int devDuration[MAX_DEVICES] = {0};
int devPin[MAX_DEVICES] = {12,13,14};
uint64_t devFinishTime[MAX_DEVICES] = {0};

uint64_t restartInterval = 60000;
uint64_t lastRestartCheck = 0;

uint64_t webRestartInterval = 5*60000;
uint64_t webLastRestartCheck = 0;

char msg[50];

//Insterrupt Service Routine
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

void setupPins() {
// Initialize the Relays pin as an output
  for (uint8_t i = 0; i < devNumber; i++) {
    pinMode(devPin[i], OUTPUT);     
    digitalWrite(devPin[i], LOW);
    
  }
  pinMode(2, OUTPUT);     
  digitalWrite(2, HIGH);

//Set pinBobber, pulling pin to logical «1» via  pull-down resistor
  pinMode(sensorPin, INPUT);
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
//The Hall-effect sensor is connected to pin 2 which uses interrupt 0. Configured to trigger on a FALLING state change (transition from HIGH
//(state to LOW state)
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING); //you can use Rising or Falling
    intervalMillisWaterFlow = sensInterval[0] * 60000;
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

////////////////////////////////////////    
    if((millis() - oldTime) > 1000)    // Only process counters once per second
    { 
    // Disable the interrupt while calculating flow rate and sending the value to the host
      detachInterrupt(sensorInterrupt);
 
    // Because this loop may not complete in exactly 1 second intervals we calculate the number of milliseconds that have passed since the last execution and use that to scale the output.
    // We also apply the calibrationFactor to scale the output based on the number of pulses per second per units of measure (litres/minute in this case) coming from the sensor.
      flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
 
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
      oldTime = millis();
 
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
      flowMilliLitres = (flowRate / 60) * 1000;
      flowLitres = (flowRate / 60);
 
    // Add the millilitres passed in this second to the cumulative total
      totalMilliLitres += flowMilliLitres;
      totalLitres += flowLitres;
 
    // Print the flow rate for this second in litres / minute
    printDebug("Flow rate: ");
    Serial.print((unsigned long)flowMilliLitres, DEC);  // Print the integer part of the variable
    printDebug("mL/Second");
    printDebug("\t");           
 
    // Print the cumulative total of litres flowed since starting
      printDebug("Output Liquid Quantity: ");        
      printDebug((unsigned long)totalMilliLitres);
      printlnDebug("mL"); 
// Reset the pulse counter so we can start incrementing again
      pulseCount = 0;
 
    // Enable the interrupt again now that we've finished sending output
      attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
    }
    
////////////////////////////////////////    
    
    uint64_t currentMillis = millis(); // grab current time
    if ((uint64_t)(currentMillis - prevMillisWaterFlow) >= intervalMillisWaterFlow ) {
      prevMillisWaterFlow = currentMillis;
      if ( totalMilliLitres > 0) {
        sprintf(msg, "%d", totalMilliLitres);
        sendMqttMessages((char *)MQTT_SENSOR_PREFIX, sensID[0],msg);
        totalMilliLitres = 0;
      }
    }

  // device processing    
    for (uint8_t i = 0; i < devNumber; i++) {  // check duration
      if ( devFinishTime[i] and devFinishTime[i] <= currentMillis) {
          digitalWrite(devPin[i], LOW);
          devFinishTime[i] = 0;
          sendMqttMessages((char *)MQTT_DEVICE_PREFIX, devID[i],"SwitchOFF");
      }
    }
      
  } 
 
}
 
