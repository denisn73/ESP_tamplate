#include <Ticker.h>
#include <Wire.h>

Ticker ticker;
void TCP_UART_init();
void TCP_UART_handle();

void tick() {
  int error, address, nDevices=0;
  DBG_OUTPUT_PORT.println("Read I2C devices...");
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if(error == 0) {
      nDevices++;
      byte data[10];
      byte i = 0;
      Wire.requestFrom(address, 5);
      delay(10);
      while(Wire.available()) {
        data[i] = (byte) Wire.read();
        i++;
      }
      if(data[0]) {
        DBG_OUTPUT_PORT.print(nDevices);
        DBG_OUTPUT_PORT.print(". Request 0x");
        if(address<16) DBG_OUTPUT_PORT.print("0");
        DBG_OUTPUT_PORT.print(address, HEX);
        DBG_OUTPUT_PORT.print(" - ");
        for(byte i=0; i<5; i++) {
          DBG_OUTPUT_PORT.print(data[i], DEC);
          DBG_OUTPUT_PORT.print(" ");
        }
        DBG_OUTPUT_PORT.println();
      }
    } else if(error == 4) {
      DBG_OUTPUT_PORT.print("Unknow error at address 0x");
      if(address<16) DBG_OUTPUT_PORT.print("0");
      DBG_OUTPUT_PORT.println(address, HEX);
    }
  }
}

void user_setup() {
    //Wire.begin();
    TCP_UART_init();
    //ticker.attach(5, tick);
}

void user_loop() {
    TCP_UART_handle();
}

