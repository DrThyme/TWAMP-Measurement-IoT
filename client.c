#include "contiki-net.h"
#include "sys/cc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void example1_init(void) {
   uip_listen(HTONS(1234));
}
void example1_app(void) {
   if(uip_newdata() || uip_rexmit()) {
      uip_send("ok\n", 3);
   }
}

PROCESS(tcp_server_process, "TCP echo process");
AUTOSTART_PROCESSES(&tcp_server_process);

PROCESS_THREAD(tcp_server_process, ev, data)
{
  PROCESS_BEGIN();
	example1_init();
	example1_app();
  PROCESS_END();
}
