
void handleFileList();
void handleFileUpload();
void handleFileDelete();
void handleFileCreate();
bool handleFileRead(String path);
String getContentType(String filename);

// holds the current upload
File fsUploadFile;

// SERVER INIT
void WebServer_init() {

  MDNS.begin(host);
  
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.print("Open http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local/edit to see the file browser");
    DBG_OUTPUT_PORT.print("Open http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local/update to upload new firmware");
  #endif
  
  server.on("/wifi_scan.htm", HTTP_GET, []() {
    if(!handleFileRead("/wifi_scan.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/wifi_scan", HTTP_GET, []() {
    
  DynamicJsonBuffer jsonArrayBuffer;
  JsonArray& json = jsonArrayBuffer.createArray();
  
    String data = "[";
    for(int i=0; i<wifis.size(); i++) {

    byte flag = 0;
    if(wifis[i].ssid.equals(WiFi.SSID())) flag = 2;
    else if(wifis[i].saved) flag = 1;
    
    JsonObject &obj = json.createNestedObject();
    obj["ssid"] = wifis[i].ssid;
    obj["rssi"] = wifis[i].rssi;
    obj["secure"] = wifis[i].secure;
    obj["flag"] = flag;
    
//      data += "{\"ssid\":\"";
//      data += wifis[i].ssid;
//      data += "\", \"rssi\":";
//      data += wifis[i].rssi;
//      data += ", \"secure\":";
//      data += wifis[i].secure;
//      data += ", \"flag\":";
//      byte flag = 0;
//      if(wifis[i].ssid.equals(WiFi.SSID())) flag = 2;
//      else if(wifis[i].saved) flag = 1;
//      data += flag;
//      data += "}";
//      if(i<wifis.size()-1) data += ",";
    }
    //data += "]";
    String t;
    json.printTo(t);
    json.printTo(Serial);
    
    server.send(200, "application/json", t);
    //
  });
  server.on("/wifi_connect", HTTP_POST, []() {
    server.sendHeader("Location", "/wifi_scan.htm");
    server.send(301);          
    if(server.hasArg("ssid")) {
      String ssid = server.arg("ssid");
      String pass = "";
      if(server.hasArg("password")) pass = server.arg("password");
      if(server.hasArg("save")) {
        if(server.arg("save")=="on") {
          #ifdef DBG_OUTPUT_PORT
            DBG_OUTPUT_PORT.print("Add ");
            DBG_OUTPUT_PORT.print(ssid);
            DBG_OUTPUT_PORT.print(" to saved networks file");
          #endif
          File file = SPIFFS.open("/wifi_config.txt", "a");
          file.print(ssid + ",");
          file.print(pass + "\n");
          file.close();
        }
      }
      #ifdef DBG_OUTPUT_PORT
        DBG_OUTPUT_PORT.print("");
      #endif
      if(ssid != WiFi.SSID()) {
        #ifdef DBG_OUTPUT_PORT
          DBG_OUTPUT_PORT.print("Disconnect from ");
          DBG_OUTPUT_PORT.println(WiFi.SSID());
        #endif
        WiFi.disconnect();
        #ifdef DBG_OUTPUT_PORT
          DBG_OUTPUT_PORT.print("Try connect to \"");
          DBG_OUTPUT_PORT.print(ssid);
          DBG_OUTPUT_PORT.print("\" with pass \"");
          DBG_OUTPUT_PORT.print(pass);
          DBG_OUTPUT_PORT.println("\"");
        #endif
      } else {
        #ifdef DBG_OUTPUT_PORT
          DBG_OUTPUT_PORT.print(WiFi.SSID());
          DBG_OUTPUT_PORT.println(" already used.");
        #endif        
      }
      //wifiMulti.addAP(ssid.c_str(), pass.c_str());
      if(pass.length()>0) WiFi.begin(ssid.c_str(), pass.c_str());
      else WiFi.begin(ssid.c_str());
    }
    yield();
  });
  
  server.on("/auth", HTTP_GET, []() {
    if(!handleFileRead("/auth.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/info", HTTP_GET, []() {
    if(!handleFileRead("/info.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/backup=1", HTTP_GET, []() {
    server.send(404, "text/plain", "Recovering...");
    FIRMWARE_BACKUP();
  });
  server.on("/update", HTTP_GET, []() {
    if(!handleFileRead("/update.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    },[](){
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START) {
        #ifdef DBG_OUTPUT_PORT
          //DBG_OUTPUT_PORT.setDebugOutput(true);
        #endif
        WiFiUDP::stopAll();
        #ifdef DBG_OUTPUT_PORT
          DBG_OUTPUT_PORT.printf("Update: %s\n", upload.filename.c_str());
        #endif
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          #ifdef DBG_OUTPUT_PORT
            Update.printError(DBG_OUTPUT_PORT);
          #endif
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          #ifdef DBG_OUTPUT_PORT
            Update.printError(DBG_OUTPUT_PORT);
          #endif
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          #ifdef DBG_OUTPUT_PORT
            DBG_OUTPUT_PORT.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          #endif
        } else {
          #ifdef DBG_OUTPUT_PORT
            Update.printError(DBG_OUTPUT_PORT);
          #endif
        }
        #ifdef DBG_OUTPUT_PORT
          //DBG_OUTPUT_PORT.setDebugOutput(false);
        #endif
      }
      yield();
    });
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor page
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  // create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"analog\":"+String(analogRead(A0));
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  MDNS.addService("http", "tcp", 80);
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("HTTP server started");
  #endif
  
}

//========================== ПРОВЕРКА АВТОРИЗАЦИИ ==========================//
bool is_authentified() {
  #ifdef DBG_OUTPUT_PORT
  DBG_OUTPUT_PORT.println("Enter is_authentified");
  #endif
  if(server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.print("Found cookie: ");
      DBG_OUTPUT_PORT.println(cookie);
    #endif
    if(cookie.indexOf("ESPSESSIONID=1") != -1) {
      //admin_flag = 1;
      #ifdef DBG_OUTPUT_PORT
        DBG_OUTPUT_PORT.println("Authentificated as 'Admin'");
      #endif
      return true;
    } else if(cookie.indexOf("ESPSESSIONID=2") != -1) {
      //admin_flag = 0;
      #ifdef DBG_OUTPUT_PORT
        DBG_OUTPUT_PORT.println("Authentificated as 'User'");
      #endif
      return true;
    }
  }
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("Authentification Failed");
  #endif
  return false;
}

// format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename) {
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}




bool handleFileRead(String path) {
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  #endif
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
//    String t = file.read(); 
//    t.indexOf(',');
//    //...
//    WiFiItem item;
//    item.ssid = t.substring(...);
//    item.pwd = ./.;
//    wifis.push_back(item);
//    for (int i = 0; i < wifis.size();i++)
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    #endif
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) fsUploadFile.close();
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
    #endif
  }
}

void handleFileDelete() {
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  #endif
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  #endif
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}  
  String path = server.arg("dir");
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.println("handleFileList: " + path);
  #endif
  Dir dir = SPIFFS.openDir(path);
  path = String();
  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }  
  output += "]";
  server.send(200, "text/json", output);
}

