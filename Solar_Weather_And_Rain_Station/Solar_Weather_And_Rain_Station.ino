#include <Wire.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

void ICACHE_RAM_ATTR handleInterrupt();   //!!! important for external interrupt for ESP8266

IPAddress staticIP(192,168,1,157);
IPAddress gateway(192,168,1,150);
IPAddress subnet(255,255,255,0);

const char* ssid = "OpenWrt";
const char* password = "charade23450";
const char* mqtt_server = "192.168.1.150";
const char* mqtt_user = "test";
const char* mqtt_pass = "duster07";
const char* mqtt_client = "ESP8266_Solar";
const char* outTopic_info = "/info/response";    // send info response
const char* outTopic_error = "/info/response/error";    // send error info response
const char* outTopic_alarm = "/watering";    // send StopAll/StartAll if it is(not) raining 
const char* inTopic = mqtt_client;        // 
String weather_message[] = {"temperature", "humility", "pressure", "weather"};

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

unsigned long lastMsg = 0;
char msg[50];
int value = 0;
boolean isFirstTime = true;

const byte interruptPin = 13;
const int rainPin = A0;
int thresholdValue = 500;   // value of treshhold to change isRain status
boolean isRain = false;
volatile boolean checkInterrupt = false;
 
unsigned long debounceTime = 1000;
unsigned long lastDebounce=0;
// set i2c addresses
#define BME280_ADDRESS 0x76
Adafruit_BME280 bme;

// init some values
float termoValue = 0;    //C
float pressureValue = 0; //mmHg
float humidityValue = 0; //%

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

//  WiFi.mode(WIFI_STA);
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* intopic, byte* payload, unsigned int length) {

  String strTopic = String(intopic);
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  Serial.print("Message arrived [" + strTopic + "] ");
  Serial.println(strPayload);

  int iPos = strTopic.lastIndexOf('/');
  if( iPos > 0){
    String destination = strTopic.substring(iPos + 1);
    int iNum = 0;
    for (int i = 0; i < 3; i++){
      if ( strPayload == weather_message[i] or strPayload == weather_message[3]) {
        int lenTopic = strlen(outTopic_info) + weather_message[i].length() + destination.length() + 3;
        char topic[lenTopic];
        snprintf(topic, lenTopic, "%s/%s/%s", outTopic_info, weather_message[i].c_str(), destination.c_str());
        float value;
        int lenMsg = 6;
        if (i == 0) 
          value = bme.readTemperature();
        else if (i == 1)  
          value = bme.readHumidity();
        else {
          value = bme.readPressure() / 100.0F;  
          lenMsg = 9;
        }  
      // push message back to broker
        snprintf (msg, lenMsg, "%f", value);
        MQTTclient.publish(topic, msg);
        iNum++; 
      }
    }
    if (!iNum) {
      // send push message about wrong request
      snprintf (msg, 50, "Wrong request %s", payload);
      MQTTclient.publish(outTopic_error, msg);     
    }
  }
  else {
    // send push message about wrong topic format
    snprintf (msg, 50, "Wrong topic format %s", intopic);
    MQTTclient.publish(outTopic_error, msg);
  }
}

void connectMQTT() {
  // Loop until we're reconnected
  while (!MQTTclient.connected()) {
    Serial.print("Attempting MQTT connection...");
// Attempt to connect
    if (MQTTclient.connect(mqtt_client, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      if (isFirstTime){
        int lenTopic = strlen(outTopic_alarm) + strlen(mqtt_client) + 2;
        char topic[lenTopic];
        snprintf(topic, lenTopic, "%s/%s", outTopic_alarm, mqtt_client);
        MQTTclient.publish(topic, "switched_on");
        isFirstTime = false;        
        Serial.print("First start MC ");
        Serial.print(topic);
        Serial.println(" switched_on");
      }
      // ... and resubscribe
      int lenTopic = strlen(inTopic) + 4;
      char topic[lenTopic];
      snprintf(topic, lenTopic, "/%s/#", inTopic);
      Serial.print("inTopic = ");
      Serial.println(topic);
      MQTTclient.subscribe(topic);

      MQTTclient.subscribe("/rain");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void handleInterrupt() {
  checkInterrupt= true;
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  MQTTclient.setServer(mqtt_server, 1883);
  MQTTclient.setCallback(callback);
  connectMQTT();
  
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(rainPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
  
  if (!bme.begin(BME280_ADDRESS))
  {
    Serial.println(F("\nCould not find a valid BME280 sensor, check wiring!"));
    MQTTclient.publish(outTopic_error, "Could not find a valid BME280 sensor");
  }
  Serial.println("started ... ");
}

void loop()
{
// MQTT client connect
  if (!MQTTclient.connected()) {
    Serial.println("reconnect MQTT");
    connectMQTT();
  }
  MQTTclient.loop();
// end MQTT client connect

// check if MQTT is still alive
  long now = millis();
  if (now - lastMsg > 900000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "%s. I am still alive #%ld", mqtt_client, value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    MQTTclient.publish("/outTopic", msg);
  }
// end check if MQTT is still alive

// check if rain is started/stopped
  if(checkInterrupt == true && ( (millis() - lastDebounce)  > debounceTime )){
    lastDebounce = millis();
    checkInterrupt = false;
    // read information from analog PIN
    int sensorValue = analogRead(rainPin);
//    if sensorValue < thresholdValue  it is Wet
//    if sensorValue >= thresholdValue  it is Dry
    if ((sensorValue < thresholdValue && !isRain) || (sensorValue >= thresholdValue && isRain)){
      isRain = !isRain;
 //     isRain ? MQTTclient.publish(outTopic, "StopAll") : MQTTclient.publish(outTopic, "StartAll");
      Serial.println("the change of rain status detected ");
    }
  }
  else
    checkInterrupt = false;
// end check if rain is started/stopped

// read sensor
/*  pressureValue = bme.readPressure() / 133.32239F;
  termoValue = bme.readTemperature();
  humidityValue = bme.readHumidity();

  // send to serial port, comment after debug
  Serial.println("---------------------------");
  Serial.print("Termo: ");
  Serial.println(termoValue);
  Serial.print("Humidity: ");
  Serial.println(humidityValue);
  Serial.print("Pressure: ");
  Serial.println(pressureValue);
  Serial.println("---------------------------");

  // check every 30 seconds
  delay(30000);*/
}
