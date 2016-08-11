#include <ESP8266WiFiMulti.h>

ESP8266WiFiMulti wifiMulti;

const char* STA_ssid     = "LAN_Prodmash";
const char* STA_password = "admin@local";

void WiFi_single();
void WiFi_multi();

// WIFI USER INIT
void WiFi_user_init() {

  //WiFi_single();
  WiFi_multi();
  
}

void WiFi_single() {
  WiFi.mode(WIFI_STA);
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("WiFI deinit to STA mode");
    DBG_OUTPUT_PORT.printf("Connecting to %s\n", STA_ssid);
  #endif
    
  if(String(WiFi.SSID()) != String(STA_ssid)) {
    WiFi.begin(STA_ssid, STA_password);
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.print(".");
    #endif
  }
  
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("Connected! IP address: ");
    DBG_OUTPUT_PORT.println(WiFi.localIP());
  #endif  
}


void WiFi_multi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_ssid, AP_password);
  IPAddress AP_IP = WiFi.softAPIP();
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("WiFI deinit to STA mode");
    DBG_OUTPUT_PORT.println("Connecting multi...");
  #endif
  wifiMulti.addAP("LAN_Prodmash", "admin@local");
  wifiMulti.addAP("TP-LINK_C3F368", "ltybcrfn73");
  if(wifiMulti.run() == WL_CONNECTED) {
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.println("");
      DBG_OUTPUT_PORT.print("Connected! IP address: ");
      DBG_OUTPUT_PORT.println(WiFi.localIP());
    #endif
  }
}

void WiFi_user_loop() {
  if(wifiMulti.run() != WL_CONNECTED) {
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.println("WiFi not connected!");
    #endif
    delay(1000);
    return;
  }
  TCP_UART_handle();
}

