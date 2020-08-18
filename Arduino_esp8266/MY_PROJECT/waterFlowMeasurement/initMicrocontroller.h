// Main configuration and includes

#ifndef _INIT_MC_H
#define _INIT_MC_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// Set the EEPROM MASK 
#define EEPROM_MASK 0xA5

// MQTT prefix name (like "/myhome/ESPX-0003DCE2")
//extern char *mqttPrefixName;

// Call ESP.restart() after timeout in configure mode
#define RESTART_AFTER_CONFIGURE

// Set the number of WIFI connection attempts
#define WIFI_CONNECT_ATTEMPTS 20

// Set the number of MQTT connection attempts
#define MQTT_CONNECT_ATTEMPTS 5

// Simple thermostat control (conflicts with MOVEMENT_CONTROL and SONOFF_CONTROL!), thermostat.cpp
// #define THERMOSTAT_CONTROL

// Configuration stuff:
// Debug output to serial port
#define DEBUG

// How often to call refresh_* functions, milliseconds
#define REFRESH_TIME 200

// How often to call refresh_* functions with flagForce, milliseconds
#define FORCE_TIME 2000

// Disable internal led blinking at connecting
//#define DISABE_BLINK

// Configuration Wifi network password
#define WIFI_CONFIGURE_PASS "passtest"

// MQTT username and password
#define MQTT_USER_NAME "test"
#define MQTT_PASSWORD "duster07"
#define MQTT_PORT 1883

// MQTT Prefixes
#define MQTT_OUT_PREFIX "/fromMC"
#define MQTT_IN_PREFIX "/toMC"
#define MQTT_NAME "esp-"
#define MQTT_INFO_PREFIX "/info"
#define MQTT_DEVICE_PREFIX "/device"
#define MQTT_SENSOR_PREFIX "/sensor"

// OTA firmware upload password
#define OTA_UPLOAD_PASS "Bi38s2iw"

// Serial port pins (for dimmer or maxbotix)
//#define rxPin 16
//#define txPin 4
//#define RS485_DE_PIN 15


// MQTT Topics names
#define TOPIC_REBOOT "REBOOT"
#define TOPIC_RECONNECT "RECONNECT"

// weather.cpp
//#define TOPIC_DHT "DHT"
//#define TOPIC_TEMPERATURE "TEMPERATURE"
//#define TOPIC_HUMIDITY "HUMIDITY"

// MQTT Client name (with chip serial ID)
extern char mqttClientName[];
extern char outTopic[];
extern char inTopic[];
extern char infoTopic[];  

// sensors and devices max number
#define MAX_SENSORS 4
#define MAX_DEVICES 4

// Configure web server
extern ESP8266WebServer server;

extern WiFiClient espClient;
extern PubSubClient mqttClient;


char *IPAddress2String(IPAddress &);
void generateMqttName();
void startWebServer();
bool connectWiFi();
bool connectMQTT();
bool initMC();
void startWebServer();
void readParamFromEEPROM();
int sendMqttMessages(char* ,char* , char* );
int sendMqttMessages(char* ,int , char* );

template < typename T >
void printlnDebug ( T param) {
#ifdef DEBUG      
      Serial.println(param);
#endif   
}

extern int sensNumber;
extern int sensID[];
extern int sensInterval[];

extern int devNumber;
extern int devID[];
extern int devDuration[];

extern char* pgName;

template < typename T >
void printDebug ( T param) {
#ifdef DEBUG      
      Serial.print(param);
#endif   
}

#endif
