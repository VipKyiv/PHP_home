// https://github.com/vasimv/esp-sensors/blob/e3e19e2c576c9e0a61142bc5fc8e9aa0836d0809/esp-sensors.ino

#include "initMicrocontroller.h"

// Wifi and MQTT connection parameters
char wifiSsid[32];
char wifiPass[16];
char mqttServer[16];

// MC description
char mcDescription[32];

// web host address 
char webHost[32];

// get string
char getHttpString[100];

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Configure web server
ESP8266WebServer server(80);

// When we started configure mode
unsigned long startConfigure = 0;
// When we started wifi fconnecting
unsigned long startConnecting = 0;

// Full MQTT prefix (with chip serial ID)
char mqttPrefixNameS[32];

// MQTT Client name (with chip serial ID)
char mqttClientName[32];
char outTopic[32];
char inTopic[32];
char infoTopic[32];

const char* host = "http://192.168.1.150";

//WIFI 
IPAddress localIP;

bool isFirstTime = true;

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

  sprintf(mqttClientName, "%s%08X", MQTT_NAME, ESP.getChipId());
  sprintf(outTopic, "%s/%s", MQTT_OUT_PREFIX, mqttClientName);
  sprintf(infoTopic, "%s/mcStarted/%s/", MQTT_INFO_PREFIX, mqttClientName);
  sprintf(inTopic, "%s/%s/#", MQTT_IN_PREFIX, mqttClientName);
//  printDebug("Auto-generated MQTT prefix: ");
 // printlnDebug(mqttPrefixName);
  printDebug("Auto-generated MQTT name: ");
  printlnDebug(mqttClientName);
} // void generateMQTTName()

// Create and start web server in configure mode
void startWebServer() {
  printDebug("Create AP to configure WIFI and MQTT parameters, SSID=");
  printlnDebug(mqttClientName);
  printlnDebug("ip address to connect setup server 192.168.4.1");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(mqttClientName, WIFI_CONFIGURE_PASS);
  server.begin();
  server.on("/", []() {
    printlnDebug("192.168.4.1  server on /");
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
    printlnDebug("192.168.4.1  server on /setting");
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
      for (uint16_t i = 0; i < 512; i++) 
        EEPROM.write(i, ' ');

      // Write signature
      EEPROM.write(0, EEPROM_MASK);
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
      for (uint8_t i = 0; i < sizeof(wifiSsid); i++) {
        EEPROM.write(i + offset, wifiSsid[i]);
        printlnDebug("Wrote: ");
        printlnDebug(wifiSsid[i]);
      }
      offset += sizeof(wifiSsid);
      printlnDebug("writing eeprom pass:");
      for (uint8_t i = 0; i < sizeof(wifiPass); i++) {
        EEPROM.write(i + offset, wifiPass[i]);
        printDebug("Wrote: ");
        printlnDebug(wifiPass[i]);
      }
      offset += sizeof(wifiPass);
      printlnDebug("writing eeprom web host ip:");
      for (uint8_t i = 0; i < sizeof(webHost); i++) {
        EEPROM.write(i + offset, webHost[i]);
        printDebug("Wrote: ");
        printlnDebug(webHost[i]);
      }
      offset += sizeof(webHost);
      printlnDebug("writing eeprom mqtt broker ip:");
      for (uint8_t i = 0; i < sizeof(mqttServer); i++) {
        EEPROM.write(i + offset, mqttServer[i]);
        printDebug("Wrote: ");
        printlnDebug(mqttServer[i]);
      }
      offset += sizeof(mqttServer);
      printlnDebug("writing eeprom mc description:");
      for (uint8_t i = 0; i < sizeof(mcDescription); i++) {
        EEPROM.write(i + offset, mcDescription[i]);
        printDebug("Wrote: ");
        printlnDebug(mcDescription[i]);
      }
      EEPROM.commit();
      EEPROM.end();
      content = "{\"Success\":\"saved to eeprom... restarting to boot into new wifi\"}";
      statusCode = 200;
      server.send(statusCode, "application/json", content);
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
  printlnDebug("192.168.4.1 end createWebServer");
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
    StaticJsonDocument<256> doc;
    doc["name"] = mqttClientName;
    doc["ip"] = IPAddress2String(localIP);
    doc["dsc"] = mcDescription;
    doc["pgName"] = pgName;
    char json_string[256];
    serializeJson(doc, json_string);
    printDebug("\njson_string: ");
    printlnDebug(json_string);
 /*   
    int lenInitTopic = strlen(outTopic) + 10;
    char initTopic[lenInitTopic];
    sprintf(initTopic, "%s/mcInit", outTopic);
    mqttClient.publish(initTopic, json_string);
*/
    sendMqttMessages(outTopic, (char *)"mcInit", json_string);
    printDebug("mqttClientName: "); 
    printDebug(mqttClientName);
    printlnDebug(" initialized");
  }
  printDebug("Subscribe to topic : ");
  printlnDebug(inTopic);
  mqttClient.subscribe(inTopic);
  return true;  
}


bool initMC() {
  HTTPClient http;
  printDebug("\nConnecting to web host "); //  Connecting
  printlnDebug(webHost); 
  snprintf(getHttpString, 100, "%S/sendResponceToMC.php?name=%S", webHost,mqttClientName);
  printlnDebug(getHttpString);
  http.begin(getHttpString);   
//  http.begin(webHost + String("/sendResponceToMC.php?name=") + mqttClientName + String("& description=") + mcDescription);   
//    http.addHeader("Content-Type", "application/json");
    int httpCode = http.GET();
    printDebug("\nhttpCode =  "); //  
    printlnDebug(httpCode); 
    //Check the returning code                                                                  
    if (httpCode == HTTP_CODE_OK) {
      String result1FromGet = http.getString(); 
      printDebug("\nConnecting to web host "); //  Connecting
      printlnDebug(result1FromGet); 
     // DynamicJsonDocument doc(1024);
      StaticJsonDocument<1024> doc;
      auto error = deserializeJson(doc, result1FromGet);
      if (error or doc["error"]) {
         char errMsg[128];
         if (error)
            sprintf(errMsg, "!!!ERROR <%s> deserializeJson() failed with code %s", mqttClientName, error.c_str());
         else {
            const char* err = doc["error"];
            sprintf(errMsg, "!!!ERROR <%s>  %s", mqttClientName, err);
         }
         mqttClient.publish("/error", errMsg);
         printlnDebug(errMsg);
         return false;
      }

      JsonObject root = doc["sensors"].as<JsonObject>();
      for (JsonPair p : root) {
         const char* key = p.key().c_str();
         printlnDebug(key);
         JsonVariant value = p.value();
         if (value.is<JsonArray>()) {
            sensID[sensNumber] = value[0];            
            sensInterval[sensNumber] = value[1]; //.as<int>;            
            printDebug("ID = ");
            printlnDebug(sensID[sensNumber]);
            printDebug("Interval = ");
            printlnDebug(sensInterval[sensNumber]);
            sensNumber++;
         }  
         else if (value.is<int>())
           printlnDebug(value.as<int>()); 
         else  
           printlnDebug("jakas hren"); 
      }
      root = doc["devices"].as<JsonObject>();
      for (JsonPair p : root) {
         const char* key = p.key().c_str();
         printlnDebug(key);
         JsonVariant value = p.value();
         if (value.is<JsonArray>()) {
            devID[devNumber] = value[0];            
            devDuration[devNumber] = value[1]; //
            printDebug("ID = ");
            printlnDebug(devID[devNumber]);
            printDebug("Duration = ");
            printlnDebug(devDuration[devNumber]);
            devNumber++;
         }  
      }

    }
    else {
      // send mqtt push message about error
      char errMsg[128];
      sprintf(errMsg, "!!!ERROR <%s>  error to connect to http server", mqttClientName);
      mqttClient.publish("/error", errMsg);
      printlnDebug("error to connect to http server");
      return false;
    }
    http.end();   //Close connection
    return true;
}  // initMC()

void readParamFromEEPROM(){
    uint8_t offset = 1;
    for (uint8_t i = 0; i < sizeof(wifiSsid); i++)
      wifiSsid[i] = EEPROM.read(i + offset);
    offset += sizeof(wifiSsid);
    for (uint8_t i = 0; i < sizeof(wifiPass); i++)
      wifiPass[i] = EEPROM.read(i + offset);
    offset += sizeof(wifiPass);
    for (uint8_t i = 0; i < sizeof(webHost); i++)
      webHost[i] = EEPROM.read(i + offset);
    offset += sizeof(webHost);
    for (uint8_t i = 0; i < sizeof(mqttServer); i++)
      mqttServer[i] = EEPROM.read(i + offset);
    offset += sizeof(mqttServer);  
    for (uint8_t i = 0; i < sizeof(mcDescription); i++)
      mcDescription[i] = EEPROM.read(i + offset);

}

int sendMqttMessages(char* mainTopic, char* subTopic, char* msg ) {
  int lenTopic = strlen(mainTopic) + strlen(subTopic) + 3;
  char topic[lenTopic];
  snprintf(topic, lenTopic, "%s/%s", mainTopic, subTopic);

  printDebug("Send MQTT message ");
  printDebug(msg);
  printDebug("  topic = ");
  printlnDebug(topic);

  return mqttClient.publish(topic, msg);
}

int sendMqttMessages(char* mainTopic, int subTopic, char* msg ) {
  int lenTopic = strlen(mainTopic) + 8;
  char topic[lenTopic];
  snprintf(topic, lenTopic, "%s/%d", mainTopic, subTopic);

  printDebug("Send MQTT message ");
  printDebug(msg);
  printDebug("  topic = ");
  printlnDebug(topic);

  return mqttClient.publish(topic, msg);
}
