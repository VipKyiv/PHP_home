// https://github.com/vasimv/esp-sensors/blob/e3e19e2c576c9e0a61142bc5fc8e9aa0836d0809/esp-sensors.ino

#include "soilMoistureWithDeepSleep.h"

// Configure web server
ESP8266WebServer server(80);

// When we started configure mode
unsigned long startConfigure = 0;
// When we started wifi fconnecting
unsigned long startConnecting = 0;

// Wifi and MQTT connection parameters
char wifiSsid[32];
char wifiPass[16];
char mqttServer[16];

// Full MQTT prefix (with chip serial ID)
char mqttPrefixNameS[32];
uint8_t mqttPrefixNameLength;
// MQTT Client name (with chip serial ID)
char mqttClientName[32];

// Configure mode flag (if 1 - create AP with web server to configure wifi and mqtt server)
uint8_t FlagConfigure;

//WIFI 
const char* host = "http://192.168.1.150";

IPAddress staticIP(192,168,1,71);  //static IP
IPAddress gateway(192,168,1,150);
IPAddress subnet(255,255,255,0);
IPAddress DHCPServer(192,168,1,150);
const char* ssid = "OpenWrt";
const char* wifi_password = "charade23450";


template < typename T >
void printlnDebug ( T param) {
#ifdef DEBUG      
      Serial.println(param);
#endif   
}

template < typename T >
void printDebug ( T param) {
#ifdef DEBUG      
      Serial.print(param);
#endif   
}

// Use chip ID to generate unique MQTT prefix
void generateMqttName() {

  sprintf(mqttPrefixNameS, "%s%08X/", MQTT_PREFIX, ESP.getChipId());
//  mqttPrefixName = mqttPrefixNameS;
  mqttPrefixNameLength = strlen(mqttPrefixNameS);
  sprintf(mqttClientName, "%s%08X", MQTT_NAME, ESP.getChipId());

  printDebug("Auto-generated MQTT prefix: ");
 // printlnDebug(mqttPrefixName);
  printlnDebug(mqttPrefixNameS);
  printDebug("Auto-generated MQTT name: ");
  printlnDebug(mqttClientName);
} // void generateMQTTName()

// Create web server in configure mode
void createWebServer() {
  server.begin();
  server.on("/", []() {
    String content;
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
    content += ipStr;
    content += "<p>";
    content += "</p><form method='get' action='setting'><label>SSID:&nbsp;</label><input name='ssid' length=32 value='";
    content += wifiSsid;
    content += "'><BR><label>Password:&nbsp;</label><input name='pass' length=16>";
    content += "<BR><label>MQTT&nbsp;broker&nbsp;IP:&nbsp;</label><input name='mqttip' length=15><P><input type='submit'></form>";
    content += "</html>";
    server.send(200, "text/html", content);
    startConfigure = millis();
  });
  server.on("/setting", []() {
    int statusCode;
    String content;
    String qsid = server.arg("ssid");
    String qpass = server.arg("pass");
    String mqttip = server.arg("mqttip");
    if ((qsid.length() > 0) && (qpass.length() > 0) && (mqttip.length() > 0)) {
      printlnDebug("Writing to EEPROM");
      printlnDebug("writing eeprom ssid:");
      EEPROM.begin(512);
      // Write signature
      EEPROM.write(0, 0xA5);
      strncpy(wifiSsid, qsid.c_str(), sizeof(wifiSsid) - 1);
      wifiSsid[sizeof(wifiSsid) - 1] = '\0';
      strncpy(wifiPass, qpass.c_str(), sizeof(wifiPass) - 1);
      wifiPass[sizeof(wifiPass) - 1] = '\0';
      strncpy(mqttServer, mqttip.c_str(), sizeof(mqttServer) - 1);
      mqttServer[sizeof(mqttServer) - 1] = '\0';
      for (int i = 0; i < sizeof(wifiSsid); i++) {
        EEPROM.write(i + 1, wifiSsid[i]);
        printlnDebug("Wrote: ");
        printlnDebug(wifiSsid[i]);
      }
      printlnDebug("writing eeprom pass:");
      for (int i = 0; i < sizeof(wifiPass); i++) {
        EEPROM.write(i + sizeof(wifiSsid) + 1, wifiPass[i]);
        printDebug("Wrote: ");
        printlnDebug(wifiPass[i]);
      }
      printlnDebug("writing eeprom mqtt broker ip:");
      for (int i = 0; i < sizeof(mqttServer); i++) {
        EEPROM.write(i + sizeof(wifiSsid) + sizeof(wifiPass) + 1, mqttServer[i]);
        printDebug("Wrote: ");
        printlnDebug(mqttServer[i]);
      }
      EEPROM.commit();
      EEPROM.end();
      content = "{\"Success\":\"saved to eeprom... restarting to boot into new wifi\"}";
      statusCode = 200;
      server.send(statusCode, "application/json", content);
      FlagConfigure = 0;
      delay(1000);
      WiFi.disconnect();
      ESP.restart();
    } 
    else {
      content = "{\"Error\":\"404 not found\"}";
      statusCode = 404;
      Serial.println("Sending 404");
      server.send(statusCode, "application/json", content);
    }
  });
} // void createWebServer()


void connectWiFi() {

//  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

//  WiFi.mode(WIFI_AP_STA);
  WiFi.config(staticIP, gateway, subnet, DHCPServer);
  WiFi.begin(ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Debugging - Output the IP Address of the ESP8266
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

void initMC() {
  HTTPClient http;
  Serial.printf("\n[Connecting to %s ... \n", host); //  Connecting
  http.begin(host + String("/sendResponceToMC.php?name=MC1"));   
//    http.addHeader("Content-Type", "application/json");
    int httpCode = http.GET();
    //Check the returning code                                                                  
    if (httpCode == HTTP_CODE_OK) {
 
      // TODO: Parsing
     // DynamicJsonDocument doc(1024);
      StaticJsonDocument<1024> doc;
      auto error = deserializeJson(doc, http.getString());
      if (error) {
         Serial.print(F("deserializeJson() failed with code "));
         Serial.println(error.c_str());
         return;
      }
      int id = doc["id"]; 
      Serial.println(id); 
      const char* name = doc["name"];
      Serial.println(name);
      JsonObject root = doc.as<JsonObject>();
      for (JsonPair p : root) {
         const char* key = p.key().c_str();
         Serial.println(key);
         JsonVariant value = p.value();
         if (value.is<JsonArray>()) {
            Serial.println("value is array");
         }
       //     ...
      }

    }
    else {
      printlnDebug("error to connect to http server");
    }
    http.end();   //Close connection

}

void startWebServer() {
    FlagConfigure = 1;
//    WiFi.mode(WIFI_STA);
//    WiFi.disconnect();
    printDebug("Create AP to configure wifi parameters, SSID=");
    printlnDebug(mqttClientName);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(mqttClientName, WIFI_CONFIGURE_PASS);
    createWebServer();
}

void setup() {
  Serial.begin(115200);
  printlnDebug("\nhello");

//  connectWiFi();
//  initMC();
  EEPROM.begin(512);
  printDebug("Signature in EEPROM: ");
  printlnDebug(EEPROM.read(0));
// Check if we have stored configuration data
  if (EEPROM.read(0) != 0xA5) {
    // No parameters in EEPROM, start configuration mode
    printlnDebug("No parameters in EEPROM, start configuration mode");
  }
  else {
    printlnDebug("EEPROM has been already configured");
//    signatureConf = true;
    for (int i = 0; i < sizeof(wifiSsid); i++)
      wifiSsid[i] = EEPROM.read(i + 1);
    for (int i = 0; i < sizeof(wifiPass); i++)
      wifiPass[i] = EEPROM.read(i + sizeof(wifiSsid) + 1);
    for (int i = 0; i < sizeof(mqttServer); i++)
      mqttServer[i] = EEPROM.read(i + sizeof(wifiSsid) + sizeof(wifiPass) + 1);
    printlnDebug("Found WIFI parameters in EEPROM: ");
    printDebug(wifiSsid);
    printDebug(" ");
    printDebug(wifiPass);
    printDebug(" ");
    printlnDebug(mqttServer);
  }
  generateMqttName();
  startWebServer();
    //signatureConf = false;
    //setupWifi(true);
  
  pinMode(LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
                                    // but actually the LED is on; this is because 
                                    // it is active low on the ESP-01)
  delay(3000);                      // Wait for a second
//  printlnDebug("LOW");
  digitalWrite(LED, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(3000);                      // Wait for two seconds (to demonstrate the active low LED)
//  printlnDebug("HIGH");
  server.handleClient();
}
