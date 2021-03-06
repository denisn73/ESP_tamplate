/*
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done
  
  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include <FS.h>
#include <vector>
#include <algorithm>

//#define USE_SOFTSERIAL
#ifdef USE_SOFTSERIAL
#include <SoftwareSerial.h>
SoftwareSerial SoftSerial(5, -1, true,  128); // RX, TX, inverse_logic, buffSize ("-1" if pin not used)
#define SOFT_SERIAL_BAUD 115200
#endif

#define DBG_OUTPUT_PORT Serial
//#define DBG_OUTPUT_PORT SoftSerial
#ifdef DBG_OUTPUT_PORT
  #define DBG_PORT_BAUD 115200
#endif

//#define USE_RECOVER_PIN   12
#ifdef USE_RECOVER_PIN
  #define RECOVER_BTN_TIME 5000 // 1000 - 10000
#endif

const char* AP_ssid      = "BeerBoxESP";
const char* AP_password  = "1234567890";
const char* STA_ssid     = "LAN_Prodmash";
const char* STA_password = "admin@local";
const char* host         = "esp8266fs";
String      ESP_name     = "ESP_tamplate";
String      ESP_version  = "v1.0";

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);

void SPIFFS_init();
void serialEvent();
void WebServer_init();
void WiFi_init(void);
void WiFi_scan(void);
void user_setup();
void user_loop();
void FIRMWARE_BACKUP();
void WiFi_user_init();
void TCP_UART_init();
void WiFi_user_loop();
String formatBytes(size_t);
void read_AP(String path);

boolean wifi_connected_flag = false;

struct WiFiItem {
  String ssid;
  String pass;
  int    rssi = 0;
  bool   saved = false;
  bool   secure = false;
  bool   connected = false;
};

std::vector<WiFiItem> wifis;

void setup(void) {

  #ifdef DBG_OUTPUT_PORT    
    DBG_OUTPUT_PORT.begin(DBG_PORT_BAUD);
    DBG_OUTPUT_PORT.println();
    DBG_OUTPUT_PORT.print(ESP_name);
    DBG_OUTPUT_PORT.print(" (");
    DBG_OUTPUT_PORT.print(ESP_version);
    DBG_OUTPUT_PORT.println(") run! (DEBUG)");
    //DBG_OUTPUT_PORT.setDebugOutput(false);
  #endif
  #ifdef USE_SOFTSERIAL
    SoftSerialRX.begin(SOFT_SERIAL_BAUD);
    SoftSerialTX.begin(SOFT_SERIAL_BAUD);
//    #ifdef DBG_OUTPUT_PORT
//      if(DBG_OUTPUT_PORT == Serial) {
//        SoftSerialRX.begin(DBG_PORT_BAUD);
//        SoftSerialTX.begin(DBG_PORT_BAUD);
//      }
//    #else
//      SoftSerialRX.begin(SOFT_SERIAL_BAUD);
//      SoftSerialTX.begin(SOFT_SERIAL_BAUD);
//    #endif
  #endif
  //  *** WARNING ***  //
  // BE VERY CAREFULL! //
  SPIFFS_init();
  WebServer_init();
  WiFi_init();
  user_setup();
  //*******************//

}
 
void loop(void) {
  
  server.handleClient();
  if(wifiMulti.run() == WL_CONNECTED) {
    if(!wifi_connected_flag) {
      wifi_connected_flag = true;
      #ifdef DBG_OUTPUT_PORT
        DBG_OUTPUT_PORT.print("WiFi connected to: ");
        DBG_OUTPUT_PORT.println(WiFi.SSID());
        DBG_OUTPUT_PORT.print("IP-address: ");
        DBG_OUTPUT_PORT.println(WiFi.localIP());
      #endif
    }    
  } else {
    if(wifi_connected_flag) {
      wifi_connected_flag = false;
      #ifdef DBG_OUTPUT_PORT
        DBG_OUTPUT_PORT.println("WiFi disconnected!");
      #endif
    }
  }
  user_loop();
  #ifdef DBG_OUTPUT_PORT
    serialEvent();
  #endif
  
}

void WiFi_init(void) {
  read_AP("/wifi_config.txt");
  WiFi_scan();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_ssid, AP_password, 1, 1); // SSID, PASS, channel, ssid_hidden
  //WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  IPAddress AP_IP = WiFi.softAPIP();
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("WiFi init as AP_STA mode!");
    DBG_OUTPUT_PORT.print("AP_SSID: ");
    DBG_OUTPUT_PORT.println(AP_ssid);
    DBG_OUTPUT_PORT.print("AP_PASS: ");
    DBG_OUTPUT_PORT.println(AP_password);
    DBG_OUTPUT_PORT.print("AP_IP-address: ");
    DBG_OUTPUT_PORT.println(AP_IP);
    DBG_OUTPUT_PORT.println("Connecting multi...");
  #endif
  for(int i=0; i<wifis.size(); i++) {
    if(wifis[i].saved) wifiMulti.addAP(wifis[i].ssid.c_str(), wifis[i].pass.c_str());
  }
  wifiMulti.run();
}

void WiFi_scan(void) {
  wifis.erase( std::remove_if(wifis.begin(), wifis.end(), [](WiFiItem& item) -> bool {
    if (item.saved) item.rssi = 0;
    return !item.saved;
  }), wifis.end() );
  
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.print("Scan networks... ");
  #endif
  int n = WiFi.scanNetworks();
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("Done!");
  #endif
  if(n == 0) {
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.println(" no networks found");
    #endif
  } else {
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.print(n);
      DBG_OUTPUT_PORT.println(" networks found:");
    #endif

//    char buff[512], tmp_buff[128];
//    const char format[] = "{\"ssid\":\"%s\",\"secure\":%d,\"flag\":%d,}";    
//    int flag;      
//    String networks = "[";
//      flag = 0;
//      if(WiFi.SSID(i).equals(WiFi.SSID())) flag = 2;
//      sprintf(tmp_buff, format, WiFi.SSID(i).c_str(), WiFi.encryptionType(i), flag);
//      strcat(buff, tmp_buff);
//      networks += "{'\"SSID':\"";
//      networks += WiFi.SSID(i);
//      networks += "','SECURE':";
//      if(WiFi.encryptionType(i)==ENC_TYPE_NONE) networks += "false";
//      else networks += "true";
//      networks += "},";

    // пробегаем по каждой найденной сети
    for(int i = 0; i < n; ++i) {
      // получаем SSID
      String ssid = WiFi.SSID(i);
      // проверяем наличие сети в списке
      auto it = std::find_if(wifis.begin(), wifis.end(), [&ssid](const WiFiItem& item) -> bool {
        return item.ssid.equals(ssid);
      });
      // если сети нет в списке - добавляем
      if(it == wifis.end()) {
        WiFiItem item;
        item.saved  = false;
        item.ssid   = ssid;
        item.secure = WiFi.encryptionType(i) != ENC_TYPE_NONE;
        item.rssi   = WiFi.RSSI(i);
        wifis.push_back(item);
      }
      // иначе если SSID совпадает то меняем уровень сигнала
      else if (it->rssi > WiFi.RSSI(i)) {
        it->rssi = WiFi.RSSI(i);
      }
    }
    #ifdef DBG_OUTPUT_PORT
      for(int i=0; i<wifis.size(); i++) {
        if(wifis[i].rssi) {
          DBG_OUTPUT_PORT.print(i);
          DBG_OUTPUT_PORT.print(". SSID: ");
          DBG_OUTPUT_PORT.print(wifis[i].ssid);
          DBG_OUTPUT_PORT.print(", RSSI: ");
          DBG_OUTPUT_PORT.print(wifis[i].rssi);
          DBG_OUTPUT_PORT.println(wifis[i].secure?"*":"");
        }
      }      
    #endif
  }
}

// SPIFFS INIT
void SPIFFS_init() {  
  SPIFFS.begin();
  Dir dir = SPIFFS.openDir("/");
  while(dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    #endif
  }
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.printf("\n");
  #endif
  #ifdef USE_RECOVER_PIN
    unsigned int recover_button_counter = RECOVER_BTN_TIME;
    pinMode(USE_RECOVER_PIN, INPUT);
    digitalWrite(USE_RECOVER_PIN, HIGH);
    while(!digitalRead(USE_RECOVER_PIN)) recover_button_counter--;
    if(!recover_button_counter) {
      #ifdef DBG_OUTPUT_PORT
        DBG_OUTPUT_PORT.println("Recover - Init proccess!");
      #endif
      FIRMWARE_BACKUP();
    } else {
      #ifdef DBG_OUTPUT_PORT
        DBG_OUTPUT_PORT.println("Recover - skiped.");
      #endif
    }
  #endif  
}

void FIRMWARE_BACKUP() {
  #ifdef DBG_OUTPUT_PORT
    //DBG_OUTPUT_PORT.setDebugOutput(true);
  #endif
  uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if(!Update.begin(maxSketchSpace)){ // start with max available size
    #ifdef DBG_OUTPUT_PORT
      Update.printError(DBG_OUTPUT_PORT);
    #endif
    return;
  }
  if(!SPIFFS.exists("/backup.bin")) {
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.println("RECOVER failed: /backup.bin not found");
    #endif
    return;
  }
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.print("Open /backup.bin for read... ");
  #endif
  File file = SPIFFS.open("/backup.bin", "r"); 
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.print("OK! Size: ");
  #endif 
  size_t firmware_size = file.size();
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println(firmware_size);
    DBG_OUTPUT_PORT.print("Start RECOVER... ");
  #endif
  uint8_t data[100 + 1];
  while(file.available()) {
    firmware_size = file.read(data, 100);
    if(Update.write(data, firmware_size) != firmware_size) {
      file.close();
      #ifdef DBG_OUTPUT_PORT
        DBG_OUTPUT_PORT.println(" Failed!");
        Update.printError(DBG_OUTPUT_PORT); 
      #endif
      return;
    }
  }
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println(" END!");
  #endif
  file.close();
  if(Update.end(true)) { // true to set the size to the current progress
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.println("Recover Success! \nRebooting...\n");
    #endif
    ESP.restart();
  } else {
    #ifdef DBG_OUTPUT_PORT
      Update.printError(DBG_OUTPUT_PORT);
    #endif
    return;
  }
  #ifdef DBG_OUTPUT_PORT
    //DBG_OUTPUT_PORT.setDebugOutput(false);
  #endif
}

#ifdef DBG_OUTPUT_PORT
String inputString = "";
void serialEvent() {
  while(DBG_OUTPUT_PORT.available()) {
    char inChar = (char)DBG_OUTPUT_PORT.read();
    if(inChar != '\n') {
      if(inChar != '\r') {
        inputString += inChar;
      }
    } else {
      if(inputString=="SCAN") WiFi_scan();
      else if(inputString=="READ_AP") read_AP("/wifi_config.txt");
      inputString = "";
    }    
  }
}
#endif

