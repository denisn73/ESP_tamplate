



void WiFi_parse() {

  

}

// read /wifi_config.txt
void read_AP(String path) { 
  
  if(SPIFFS.exists(path)) {
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.println("Read networks from file /wifi_config.txt");
    #endif
    long cur_pos = 0, indexSSID, indexPASS;
    File file = SPIFFS.open(path, "r");
    String str = file.readString();
    while (1) {
      indexSSID = str.indexOf(',', cur_pos);
      indexPASS = str.indexOf('\n', cur_pos);
      if (indexSSID != -1 && indexPASS > indexSSID) {
        WiFiItem item;
        item.saved = true;
        item.ssid = str.substring(cur_pos, indexSSID);
        cur_pos = indexSSID+1;
        item.pass =  str.substring(cur_pos, indexPASS);
        item.secure = item.pass.length();
        cur_pos = indexPASS+1;  
        wifis.push_back(item);
      } else break;
    };
    file.close();
    #ifdef DBG_OUTPUT_PORT
      for(int i=0; i<wifis.size(); i++) {
        DBG_OUTPUT_PORT.print(i);
        DBG_OUTPUT_PORT.print(". SSID: ");
        DBG_OUTPUT_PORT.print(wifis[i].ssid);
        DBG_OUTPUT_PORT.print(", PASS: ");
        DBG_OUTPUT_PORT.println(wifis[i].pass);
      }      
    #endif
  } else{
    #ifdef DBG_OUTPUT_PORT
      DBG_OUTPUT_PORT.println("Can't read file /wifi_config.txt");
    #endif
  }
}

