#include <ESP8266WiFi.h>
// Настройки сети
const char* ssid = "OpenWrt";
const char* password = "charade23450";

WiFiServer server(80);               // порт сервера 80 

//Начальные данные ESP чипа
unsigned long ESPChipId = ESP.getChipId();                                 // ID чипа ESP8266
unsigned long ESPFlashChipId = ESP.getFlashChipId();                            // ID флэш-памяти чипа
const char* ESPSdkVersion = ESP.getSdkVersion();                           // Версия SDK 
byte ESPCpuFreqMHz = ESP.getCpuFreqMHz();                                  // Частота процессора
unsigned long ESPFlashChipMHz = ESP.getFlashChipSpeed()/1000000;           // Частота flash-памяти чипа (в Гц).
int ESPFlashChipSize = ESP.getFlashChipSize()/1048576;                     // Размер flash-памяти чипа (в байтах), каким его видит SDK
int ESPFlashChipRealSize = ESP.getFlashChipRealSize()/1048576;             // Настоящий размер flash-памяти (в байтах), основанный на ID flash-памяти чипа.
int ESPFreeHeap = ESP.getFreeHeap();                                       // Размер свободной памяти.
unsigned long ESPFreeSketchSpace = ESP.getFreeSketchSpace();               // Свободное место для загрузки скетча 
unsigned long ESPSketchSize = ESP.getSketchSize();                         // Размер скетча, в данный момент загруженного на ESP8266

void setup() {
//Конект к точке доступа
WiFi.mode(WIFI_STA);        // Тип работы  WIFI_STA что значит клиент
WiFi.begin(ssid, password); // Подключаемся по установленным параметрам
server.begin();             // Стартуем сервер на ESP8266 
Serial.begin(115200);       // Открываем работу с COM (Serial) портом
} 

void loop() {
  PodprogrammWebHtmlESPPage();
  //Вывод данных в порт для отладки
Serial.println("Start-------------------------------");
Serial.print("ID ESP8266: "); 
Serial.println(ESPChipId); 
Serial.print("ID flash memory: "); 
Serial.println(ESPFlashChipId); 
Serial.print("Version SDK: "); 
Serial.println(ESPSdkVersion);
Serial.println("------------------------------------"); 
Serial.print("MHz CPU: "); 
Serial.println(ESPCpuFreqMHz); 
Serial.print("MHz memory: "); 
Serial.println(ESPFlashChipMHz); 
Serial.println("------------------------------------"); 
Serial.print("Flash memory SDK (Mb): "); 
Serial.println(ESPFlashChipSize); 
Serial.print("Flash memory ID chip (Mb): "); 
Serial.println(ESPFlashChipRealSize); 
Serial.println("------------------------------------"); 
Serial.print("Free memory: "); 
Serial.println(ESPFreeHeap);

Serial.print("Free memory sketch: "); 
Serial.println(ESPFreeSketchSpace);

Serial.print("Size sketch: "); 
Serial.println(ESPSketchSize);
Serial.println("End---------------------------------");
Serial.println("------------------------------------");

delay (60000);
}

// Подпрограмма WEB,Html страницы.
void PodprogrammWebHtmlESPPage()
{ 
// проверяем, подключен ли клиент:
WiFiClient client = server.available();
if (!client) {return;}
// ждем, когда клиент отправит какие-нибудь данные:
while(!client.available()){}
String request = client.readStringUntil('\r');// считываем первую строчку запроса
client.flush();

// Формируем страницу Html в ответ на запрос
client.println("HTTP/1.1 200 OK");
client.println("Content-Type: text/html"); // "Тип контента: text/html 
client.println(""); // не забываем это, иначе не будет грузится страница
client.println("<!DOCTYPE HTML>");
client.println("<html>");
client.println("<head>");
client.println("<meta charset='utf-8'>");
client.println("<title>Веб сервер модуля ESP8266</title>");
client.println("</head>");
client.println("<body>");
client.println("<h1>Данные чипа ESP 8266</h1>");
client.println("<table border=\"1\" width=\"50%\" cellpadding=\"5\">");
client.println("<tr>");
client.println("<th>Данные чипа - ID ESP8266</th>");
client.println("<th>");
client.print(ESPChipId);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Данные памяти - ID flash memory</th>");
client.println("<th>");
client.print(ESPFlashChipId);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Версия SDK</th>");
client.println("<th>");
client.print(ESPSdkVersion);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Частота работы центрального процесора CPU (MHz)</th>");
client.println("<th>");
client.print(ESPCpuFreqMHz);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Частота работы памяти (MHz)</th>");
client.println("<th>");
client.print(ESPFlashChipMHz);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Объем свободной памяти указанный в SDK (Mb)</th>");
client.println("<th>");
client.print(ESPFlashChipSize);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Объем свободной памяти указанный в данных платы (Mb)</th>");
client.println("<th>");
client.print(ESPFlashChipRealSize);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Свободная память</th>");
client.println("<th>");
client.print(ESPFreeHeap);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Свободная память для загрузки скетча</th>");
client.println("<th>");
client.print(ESPFreeSketchSpace);
client.println("</th>");
client.println("</tr>");
client.println("<tr>");
client.println("<th>Размер загруженного скетча</th>");
client.println("<th>");
client.print(ESPSketchSize);
client.println("</th>");
client.println("</tr>");
client.println("</table>");
client.println("</body>");
client.println("</html>");
}
