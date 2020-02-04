// Main configuration and includes

#ifndef _SOIL_SENSOR_H
#define _SOIL_SENSOR_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>

// MQTT prefix name (like "/myhome/ESPX-0003DCE2")
//extern char *mqttPrefixName;

// Call ESP.restart() after timeout in configure mode
#define RESTART_AFTER_CONFIGURE

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
#define MQTT_USER_NAME "testuser"
#define MQTT_PASSWORD "testtest"
#define MQTT_PORT 1883

// MQTT Prefixes
#define MQTT_PREFIX "/myhome/ESPX-"
#define MQTT_NAME "espx-"
#define MQTT_PNP_PREFIX "/myhome/PNP"

// OTA firmware upload password
#define OTA_UPLOAD_PASS "Bi38s2iw"

// Serial port pins (for dimmer or maxbotix)
//#define rxPin 16
//#define txPin 4
//#define RS485_DE_PIN 15

#define LED 2

// MQTT Topics names
#define TOPIC_REBOOT "REBOOT"
#define TOPIC_RECONNECT "RECONNECT"

// weather.cpp
//#define TOPIC_DHT "DHT"
//#define TOPIC_TEMPERATURE "TEMPERATURE"
//#define TOPIC_HUMIDITY "HUMIDITY"

extern WiFiClient wifi;
extern PubSubClient myClient;

#endif
