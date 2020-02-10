// https://github.com/vasimv/esp-sensors/blob/e3e19e2c576c9e0a61142bc5fc8e9aa0836d0809/esp-sensors.ino

#include "soilMoistureWithDeepSleep.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

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

// MC description
char mcDescription[32];

// web host address 
char webHost[32];

// Full MQTT prefix (with chip serial ID)
char mqttPrefixNameS[32];
uint8_t mqttPrefixNameLength;
// MQTT Client name (with chip serial ID)
char mqttClientName[32];

// Configure mode flag (if 1 - create AP with web server to configure wifi and mqtt server)
uint8_t FlagConfigure = 0;

const char* host = "http://192.168.1.150";

//WIFI 
IPAddress localIP;
/*
IPAddress staticIP(192,168,1,71);  //static IP
IPAddress gateway(192,168,1,150);
IPAddress subnet(255,255,255,0);
IPAddress DHCPServer(192,168,1,150);

*/
bool isFirstTime = true;

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

char *IPAddress2String(IPAddress & ip) {
    static char str_IP[16];
    char * last = str_IP;
    for (uint8_t i = 0; i < 4; i++) {
        itoa(ip[i], last, 10);
        last = last + strlen(last);
        if (i == 3) *last = '\0'; else *last++ = '.';
    }
    return str_IP;
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
//  server.handleClient();
  server.on("/", []() {
    String content;
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
    content += ipStr;
    content += "<p>";
    content += "</p><form method='get' action='setting'><label>SSID:_________&nbsp;</label><input name='ssid' length=32 value='";
    content += wifiSsid;
    content += "'><BR><label>Password:______&nbsp;</label><input name='pass' length=16  value='";
    content += wifiPass;
    content += "'><BR><label>Description:____&nbsp;</label><input name='desc' length=32 value='";
    content += mcDescription;
    content += "'><BR><label>WEB Host Addr:&nbsp;</label><input name='host' length=32 value='";
    content += webHost;
    content += "'><BR><label>MQTT&nbsp;broker&nbsp;IP:&nbsp;</label><input name='mqttip' length=15 value='";
    content += mqttServer;
    content += "'><P><input type='submit'></form></html>";
    server.send(200, "text/html", content);
    startConfigure = millis();
  });
  server.on("/setting", []() {
    uint8_t statusCode;
    uint8_t offset = 1;
    String content;
    String qsid = server.arg("ssid");
    String qpass = server.arg("pass");
    String mcdesc = server.arg("desc");
    String whost = server.arg("host");  
    String mqttip = server.arg("mqttip");
    if ((qsid.length() > 0) && (qpass.length() > 0) && (whost.length() > 0) && (mqttip.length() > 0)) {
      printlnDebug("Writing to EEPROM");
      printlnDebug("writing eeprom ssid:");
      EEPROM.begin(512);
      // clear EEPROM
      for (int i = 0; i < 512; i++) 
        EEPROM.write(i, ' ');

      // Write signature
      EEPROM.write(0, 0xA5);
      strncpy(wifiSsid, qsid.c_str(), sizeof(wifiSsid) - 1);
      wifiSsid[sizeof(wifiSsid) - 1] = '\0';
      strncpy(wifiPass, qpass.c_str(), sizeof(wifiPass) - 1);
      wifiPass[sizeof(wifiPass) - 1] = '\0';
      strncpy(webHost, whost.c_str(), sizeof(webHost) - 1);
      webHost[sizeof(webHost) - 1] = '\0';
      strncpy(mqttServer, mqttip.c_str(), sizeof(mqttServer) - 1);
      mqttServer[sizeof(mqttServer) - 1] = '\0';
      strncpy(mcDescription, mcdesc.c_str(), sizeof(mcDescription) - 1);
      mcDescription[sizeof(mcDescription) - 1] = '\0';
      for (int i = 0; i < sizeof(wifiSsid); i++) {
        EEPROM.write(i + offset, wifiSsid[i]);
        printlnDebug("Wrote: ");
        printlnDebug(wifiSsid[i]);
      }
      offset += sizeof(wifiSsid);
      printlnDebug("writing eeprom pass:");
      for (int i = 0; i < sizeof(wifiPass); i++) {
        EEPROM.write(i + offset, wifiPass[i]);
        printDebug("Wrote: ");
        printlnDebug(wifiPass[i]);
      }
      offset += sizeof(wifiPass);
      printlnDebug("writing eeprom web host ip:");
      for (int i = 0; i < sizeof(webHost); i++) {
        EEPROM.write(i + offset, webHost[i]);
        printDebug("Wrote: ");
        printlnDebug(webHost[i]);
      }
      offset += sizeof(webHost);
      printlnDebug("writing eeprom mqtt broker ip:");
      for (int i = 0; i < sizeof(mqttServer); i++) {
        EEPROM.write(i + offset, mqttServer[i]);
        printDebug("Wrote: ");
        printlnDebug(mqttServer[i]);
      }
      offset += sizeof(mqttServer);
      printlnDebug("writing eeprom mc description:");
      for (int i = 0; i < sizeof(mcDescription); i++) {
        EEPROM.write(i + offset, mcDescription[i]);
        printDebug("Wrote: ");
        printlnDebug(mcDescription[i]);
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
      printlnDebug("Sending 404");
      server.send(statusCode, "application/json", content);
    }
  });
} // void createWebServer()


bool connectWiFi() {
  uint8_t iCount = 0;
// We start by connecting to a WiFi network
  printDebug("\nConnecting to ");
  printlnDebug(wifiSsid);

//  WiFi.mode(WIFI_AP_STA);
//  WiFi.config(staticIP, gateway, subnet, DHCPServer);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    iCount++;
    if ( iCount > WIFI_CONNECT_ATTEMPTS) 
      return false;
  }
  // Debugging - Output the IP Address of the ESP8266
  localIP = WiFi.localIP();
  printlnDebug("\nWiFi connected");
  printDebug("IP address: ");
  printlnDebug(localIP);
  return true;
}

bool connectMQTT() {
  uint8_t iCount = 0;
  mqttClient.setServer(mqttServer, 1883);
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    printlnDebug("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(mqttClientName, MQTT_USER_NAME, MQTT_PASSWORD)) {
      break;
    } 
    else {
      iCount++;
      if (iCount > MQTT_CONNECT_ATTEMPTS) {
        printDebug("failed, rc=");
        printlnDebug(mqttClient.state());
        return false;
      }
      // Wait 5 seconds before retrying
      delay(500);
    }
  }
  printlnDebug("connected");
  // Once connected, publish an announcement...
  if (isFirstTime){
    isFirstTime = false;        
    int lenTopic = strlen(MQTT_PNP_PREFIX) + strlen(mqttClientName) + 2;
    char topic[lenTopic];
    snprintf(topic, lenTopic, "%s/%s", MQTT_PNP_PREFIX, mqttClientName);
    StaticJsonDocument<256> doc;
    doc["name"] = mqttClientName;
    doc["ip"] = IPAddress2String(localIP);
    serializeJson(doc, Serial);
    char json_string[256];
    serializeJson(doc, json_string);
    printDebug("json_string: ");
    printlnDebug(json_string);
    mqttClient.publish(topic, json_string);
    printlnDebug(topic);
    printDebug(mqttClientName);
    printlnDebug(" switched_on");
  }
  return true;  
}


bool initMC() {
  HTTPClient http;
  printDebug("\n[Connecting to "); //  Connecting
  printlnDebug(webHost); 
  http.begin(webHost + String("/sendResponceToMC.php?name=MC1"));   
//    http.addHeader("Content-Type", "application/json");
    int httpCode = http.GET();
    //Check the returning code                                                                  
    if (httpCode == HTTP_CODE_OK) {

     // DynamicJsonDocument doc(1024);
      StaticJsonDocument<1024> doc;
      auto error = deserializeJson(doc, http.getString());
      if (error) {
         printDebug("deserializeJson() failed with code ");
         printlnDebug(error.c_str());
         return false;
      }
      int id = doc["id"]; 
      printlnDebug(id); 
      const char* name = doc["name"];
      printlnDebug(name);
      JsonObject root = doc.as<JsonObject>();
      for (JsonPair p : root) {
         const char* key = p.key().c_str();
         printlnDebug(key);
         JsonVariant value = p.value();
         if (value.is<JsonArray>()) {
            printlnDebug("value is array");
         }
      }

    }
    else {
      printlnDebug("error to connect to http server");
      return false;
    }
    http.end();   //Close connection
    return true;
}  // initMC()

void startWebServer() {
    FlagConfigure = 1;
//    WiFi.mode(WIFI_STA);
//    WiFi.disconnect();
    printDebug("Create AP to configure WIFI and MQTT parameters, SSID=");
    printlnDebug(mqttClientName);
    printlnDebug("ip address to connect setup server 192.168.4.1");
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
  generateMqttName();
  if (EEPROM.read(0) != 0xA5) {
    // No parameters in EEPROM, start configuration mode
    printlnDebug("No parameters in EEPROM, start configuration mode");
    startWebServer();
  }
  else {
    printlnDebug("EEPROM has been already configured");
    uint8_t offset = 1;
    for (int i = 0; i < sizeof(wifiSsid); i++)
      wifiSsid[i] = EEPROM.read(i + offset);
    offset += sizeof(wifiSsid);
    for (int i = 0; i < sizeof(wifiPass); i++)
      wifiPass[i] = EEPROM.read(i + offset);
    offset += sizeof(wifiPass);
    for (int i = 0; i < sizeof(webHost); i++)
      webHost[i] = EEPROM.read(i + offset);
    offset += sizeof(webHost);
    for (int i = 0; i < sizeof(mqttServer); i++)
      mqttServer[i] = EEPROM.read(i + offset);
    offset += sizeof(mqttServer);  
    for (int i = 0; i < sizeof(mcDescription); i++)
      mcDescription[i] = EEPROM.read(i + offset);
    printlnDebug("Found WIFI parameters in EEPROM: ");
    if (!connectWiFi() or !connectMQTT() or !initMC()) {
       printlnDebug("\nconnection error, start configuration mode");
       startWebServer();
    }
    
/*    printDebug(wifiSsid);
    printDebug(" ");
    printDebug(wifiPass);
    printDebug(" ");
    printlnDebug(mqttServer);*/
  }
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
  printlnDebug("LOW");
  digitalWrite(LED, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(3000);                      // Wait for two seconds (to demonstrate the active low LED)
  printlnDebug("HIGH");
  if (FlagConfigure)
    server.handleClient();
}
