
#define MAX_SRV_CLIENTS 1
#define TCP_UART_PORT   4444
#define TCP_UART_SERIAL SoftSerial

WiFiServer tcp_uart_server(TCP_UART_PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];

void TCP_UART_init() {
  tcp_uart_server.begin();
  tcp_uart_server.setNoDelay(true);
  #ifdef DBG_OUTPUT_PORT
    DBG_OUTPUT_PORT.print("TCP<->UART server inited: ");
    DBG_OUTPUT_PORT.println(TCP_UART_PORT);
  #endif
}

void TCP_UART_handle() {
  uint8_t i;
  //check if there are any new clients
  if (tcp_uart_server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = tcp_uart_server.available();
        #ifdef DBG_OUTPUT_PORT
          DBG_OUTPUT_PORT.print("New client: "); DBG_OUTPUT_PORT.print(i);
        #endif
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = tcp_uart_server.available();
    serverClient.stop();
  }
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      if(serverClients[i].available()){
        //get data from the telnet client and push it to the UART
        while(serverClients[i].available()) TCP_UART_SERIAL.write(serverClients[i].read());
      }
    }
  }
  //check UART for data
  if(TCP_UART_SERIAL.available()) {
    size_t len = TCP_UART_SERIAL.available();
    uint8_t sbuf[len];
    TCP_UART_SERIAL.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
        serverClients[i].write(sbuf, len);
      }
    }
  }
}

