#include <Ticker.h>


void user_setup() {
  
  WiFi_user_init();
  TCP_UART_init();
  
}

void user_loop() {
  
  WiFi_user_loop();

  
    
}
