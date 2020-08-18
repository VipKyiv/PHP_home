#include <ESP8266WiFi.h>
#include <PubSubClient.h>

void ICACHE_RAM_ATTR handleInterrupt();   //!!! important for external interrupt for ESP8266

IPAddress staticIP(192,168,1,156);
IPAddress gateway(192,168,1,150);
IPAddress subnet(255,255,255,0);

const char* ssid = "OpenWrt";
const char* password = "charade23450";
const char* mqtt_server = "192.168.1.150";
const char* mqtt_user = "test";
const char* mqtt_pass = "duster07";
const char* mqtt_client = "ESP8266_2";
const char* outTopic = "/watering";    // ????????
const char* inTopic = "/rain";        // // ????????
String dev_name[] = {"rain_1", "rain_2"};

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

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet);

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

void callback(char* topic, byte* payload, unsigned int length) {

  String strTopic = String(topic);
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  Serial.print("Message arrived [" + strTopic + "] ");
  Serial.println(strPayload);

  // Switch on the LED if an 1 was received as first character
  int iPos = strTopic.lastIndexOf('/');
  if( iPos > 0){
    String devTopic = strTopic.substring(iPos + 1);
    for (int i = 0; i < 4; i++){
      if (dev_name[i] == devTopic){
        if ( strPayload == "Begin" or strPayload == "Start") {
          digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP8266)
      // push message back to broker
          MQTTclient.publish("/outTopic", "Successfully started");
        } 
        else if ( strPayload == "End" or strPayload == "Stop"){
          digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
        }
        break;
      }
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!MQTTclient.connected()) {
    Serial.print("Attempting MQTT connection...");
// Attempt to connect
    if (MQTTclient.connect(mqtt_client, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      if (isFirstTime){
        int lenTopic = strlen(outTopic) + strlen(mqtt_client) + 2;
        char topic[lenTopic];
        snprintf(topic, lenTopic, "%s/%s", outTopic, mqtt_client);
        MQTTclient.publish(topic, "switched_on");
        isFirstTime = false;        
        Serial.print("First start MC ");
        Serial.print(topic);
        Serial.println(" switched_on");
      }
      // ... and resubscribe
      int lenTopic = strlen(inTopic) + strlen(mqtt_client) + 4;
      char topic[lenTopic];
      snprintf(topic, lenTopic, "%s/%s/+", inTopic, mqtt_client);
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

void setup() {
 
  Serial.begin(115200);
  WiFi.softAPdisconnect(true);
  setup_wifi();
/*
//  MQTTclient.setServer(mqtt_server, 1883);
//  MQTTclient.setCallback(callback); */

  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(rainPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
  Serial.println("\nstarted ... ");
}


void loop() {
// MQTT client connect
 /* if (!MQTTclient.connected()) {
    Serial.println("reconnect");
    reconnect();
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
  }*/
// end check if MQTT is still alive

  int rainSensorValue = analogRead(rainPin);
  Serial.print("Rain ");
  Serial.println(rainSensorValue);
  delay(1000);

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
 
}
