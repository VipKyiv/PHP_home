#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const int LED = 2;
const char* host = "http://192.168.1.150";
//WIFI 

IPAddress staticIP(192,168,1,71);  //static IP
IPAddress gateway(192,168,1,150);
IPAddress subnet(255,255,255,0);
IPAddress DHCPServer(192,168,1,150);
const char* ssid = "OpenWrt";
const char* wifi_password = "charade23450";

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
//      Serial.println(doc["country"]); 

//      Serial.println(doc["office"][0]);
//      Serial.println(doc["office"][1]);
    }
    else {
      Serial.println("error to connect to http server");
    }
    http.end();   //Close connection

}

void setup() {
  Serial.begin(115200);
  Serial.println("\nhello");
  connectWiFi();
  initMC();
  
  pinMode(LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
                                    // but actually the LED is on; this is because 
                                    // it is active low on the ESP-01)
  delay(3000);                      // Wait for a second
  Serial.println("LOW");
  digitalWrite(LED, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(3000);                      // Wait for two seconds (to demonstrate the active low LED)
  Serial.println("HIGH");
}
