
#define MAX_SRV_CLIENTS 1
#define TCP_UART_PORT   4444
<<<<<<< HEAD
#define TCP_UART_SERIAL Serial
#include <SoftwareSerial.h>
SoftwareSerial SoftSerial(-1, 4, 128);
=======
//#define TCP_UART_SERIAL_RX SoftSerialRX
//#define TCP_UART_SERIAL_TX SoftSerialTX
>>>>>>> origin/master

WiFiServer tcp_uart_server(TCP_UART_PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];

void TCP_UART_init() {
  TCP_UART_SERIAL.begin(9600);
  SoftSerial.begin(9600);
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
    for(i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
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
  for(i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      if(serverClients[i].available()){
        //get data from the telnet client and push it to the UART
<<<<<<< HEAD
        while(serverClients[i].available()) SoftSerial.write(serverClients[i].read());
=======
        //while(serverClients[i].available()) TCP_UART_SERIAL_TX.write(serverClients[i].read());
>>>>>>> origin/master
      }
    }
  }
  //check UART for data
//  if(TCP_UART_SERIAL_RX.available()) {
//    size_t len = TCP_UART_SERIAL_RX.available();
//    uint8_t sbuf[len];
//    TCP_UART_SERIAL_RX.readBytes(sbuf, len);
//    //push UART data to all connected telnet clients
//    for(i = 0; i < MAX_SRV_CLIENTS; i++) {
//      if (serverClients[i] && serverClients[i].connected()){
//        serverClients[i].write(sbuf, len);
//      }
//    }
//  }
}

