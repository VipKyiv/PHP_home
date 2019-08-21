/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define LED_BUILTIN D4

IPAddress staticIP(192,168,1,158);
IPAddress gateway(192,168,1,150);
IPAddress subnet(255,255,255,0);

// Update these with values suitable for your network.
const char* ssid = "OpenWrt";
const char* password = "charade23450";
const char* mqtt_server = "192.168.1.150";
const char* mqtt_user = "test";
const char* mqtt_pass = "duster07";
//const char* mqtt_client = "ESP8266_1";
const char* mqtt_client = "name_2";
const char* outTopic = "/watering";
const char* inTopic = "/valve";
String dev_name[] = {"valve_1", "valve_2", "valve_3", "valve_4"};
//const char* dev_name[] = {"valve_1","valve_2","valve_3", "valve_4"};


WiFiClient espClient;
PubSubClient MQTTclient(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
byte isFirstTime = true;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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

void connectMQTT() {
  // Loop until we're reconnected
  while (!MQTTclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
//    String clientId = "ESP8266Client-";
//    clientId += String(random(0xffff), HEX);
    // Attempt to connect
//    if (client.connect(clientId.c_str())) {
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
      MQTTclient.subscribe(topic);
      MQTTclient.subscribe("/debug");
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

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  setup_wifi();
  MQTTclient.setServer(mqtt_server, 1883);
  MQTTclient.setCallback(callback);
}

void loop() {

  if (!MQTTclient.connected()) {
    Serial.println("reconnect");
    connectMQTT();
  }
  MQTTclient.loop();

  long now = millis();
  if (now - lastMsg > 900000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "%s. I am still alive #%ld", mqtt_client, value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    MQTTclient.publish("/outTopic", msg);
  }
}
