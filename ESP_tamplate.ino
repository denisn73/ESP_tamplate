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
#include <FS.h>

//#define USE_SOFTSERIAL
#ifdef USE_SOFTSERIAL
#include <SoftwareSerial.h>
SoftwareSerial SoftSerial(5, 4, 128); // RX, TX, inverse_logic, buffSize
#define SOFT_SERIAL_BAUD 9600
#endif

//#define DBG_OUTPUT_PORT Serial
//#define DBG_OUTPUT_PORT SoftSerial
#ifdef DBG_OUTPUT_PORT
  #define DBG_PORT_BAUD 115200
#endif

#define USE_RESET_PIN   12
#ifdef USE_RESET_PIN
  #define RECOVER_BTN_TIME 5000 // 1000 - 10000
#endif

const char* AP_ssid      = "BeerBoxESP";
const char* AP_password  = "1234567890";
const char* host         = "esp8266fs";
String      ESP_name     = "ESP_tamplate";
String      ESP_version  = "v1.0";

ESP8266WebServer server(80);

void SPIFFS_init();
void WebServer_init();
void user_setup();
void user_loop();
void FIRMWARE_BACKUP();
void WiFi_user_init();
void TCP_UART_init();
void WiFi_user_loop();
String formatBytes(size_t);

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
    #ifdef DBG_OUTPUT_PORT
      if(DBG_OUTPUT_PORT != SoftSerial) SoftSerial.begin(DBG_PORT_BAUD);
    #else
      SoftSerial.begin(SOFT_SERIAL_BAUD);
    #endif
  #endif
  //  *** WARNING ***  //
  // BE VERY CAREFULL! //
  SPIFFS_init();
  WebServer_init();
  //WiFi_init();
  user_setup();
  //*******************//

}
 
void loop(void){
  
  server.handleClient();
  user_loop();
  
}

void WiFi_init() {
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.print("Configuring as AP... ");
  #endif
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_ssid, AP_password);
  IPAddress AP_IP = WiFi.softAPIP();
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("OK!");
    DBG_OUTPUT_PORT.print("SSID: ");
    DBG_OUTPUT_PORT.println(AP_ssid);
    DBG_OUTPUT_PORT.print("PASSWORD: ");
    DBG_OUTPUT_PORT.println(AP_password);
    DBG_OUTPUT_PORT.print("IP-address: ");
    DBG_OUTPUT_PORT.println(AP_IP);
  #endif
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
  #ifdef USE_RESET_PIN
    unsigned int recover_button_counter = RECOVER_BTN_TIME;
    pinMode(USE_RESET_PIN, INPUT);
    digitalWrite(USE_RESET_PIN, HIGH);
    while(!digitalRead(USE_RESET_PIN)) recover_button_counter--;
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
