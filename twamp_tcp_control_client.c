#include <stdio.h>
#include <stdlib.h>
#include "contiki-net.h"
#include "servreg-hack.h"
#include "clock.h"
#include "twamp.h"

static struct psock ps;
static uint8_t buffer[100];

static int first = 1;
static int progress = 0;

#define SERVICE_ID 190

PROCESS(example_psock_client_process, "Example protosocket client");
AUTOSTART_PROCESSES(&example_psock_client_process);

/*---------------------------------------------------------------------------*/
static int
handle_connection(struct psock *p)
{
  PSOCK_BEGIN(p);
  ServerGreeting greet;
  int i;
  printf("Size of greeting: %d\n",sizeof(greet));
  //PSOCK_SEND_STR(p, "GET / HTTP/1.0\r\n");
  //PSOCK_SEND_STR(p, "Server: Contiki example protosocket client\r\n");
  //PSOCK_SEND_STR(p, "\r\n");
  while(1) {
    PSOCK_READTO(p, '\n');
    printf("Got: %s", buffer);
    if(first){
      memset(&greet, 0, sizeof(greet));
      greet.Modes = 5;
      int r = rand();
      printf("Int: %d\n",r);
      printf("Int: %d\n",(r % 16));
      printf("Greet challenge: ");
      for (i = 0; i < 16; i++){
        greet.Challenge[i] = rand() % 16;
        printf("%d\n",greet.Challenge[i]);}
      for (i = 0; i < 16; i++)
        greet.Salt[i] = rand() % 16;
      printf("\n");
      printf("Greet challenge: %d\n",greet.Challenge[0]);
      //greet.Count = (1 << 12);
      PSOCK_SEND(p, &greet, sizeof(greet));
      //PSOCK_SEND_STR(p, "HELLO!\n");
      first = 0;
    }
    memset(&buffer[0], 0, sizeof(buffer));
  }
  
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static int
handle_servergreeting(struct psock *p)
{
  PSOCK_BEGIN(p);
  ServerGreeting greet;
  int i;
  PSOCK_READBUF(p);
  memcpy(&greet,buffer,sizeof(greet));

  printf("Greet modes: %d\n",greet.Modes);
  printf("Greet challenge: ");
  for (i = 0; i < 16; i++){
      printf("%d",greet.Challenge[i]);}
  printf("\nGreet salt: ");
  for (i = 0; i < 16; i++){
      printf("%d",greet.Salt[i]);}
  printf("\n");

  memset(&buffer[0], 0, sizeof(buffer));
  
  progress++;
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static int
send_setupResponse(struct psock *p)
{
  PSOCK_BEGIN(p);
  printf("In setup response.\n");
  SetupResponseUAuth setup;
  setup.Modes = 1;
  PSOCK_SEND(p,&setup,sizeof(setup));
  progress++;
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static int
control_client(struct psock *p)
{
  PSOCK_BEGIN(p);
  static ServerGreeting greet;
  int i;
  /* We wait until we receive a server greeting from the server.  */
  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  if(PSOCK_NEWDATA(p)){
    PSOCK_READBUF(p);
    memcpy(&greet,buffer,sizeof(greet));
    
    printf("Greet modes: %d\n",greet.Modes);
    printf("Greet challenge: ");
    for (i = 0; i < 16; i++){
      printf("%d",greet.Challenge[i]);}
    printf("\nGreet salt: ");
    for (i = 0; i < 16; i++){
      printf("%d",greet.Salt[i]);}
    printf("\n");
  } else {
    printf("Timed out!\n");
  }
  //memset(&buffer, 0, sizeof(buffer));
  /* Send setup response to the server.  */
  static SetupResponseUAuth setup;
  memset(&setup,0,sizeof(setup));
  setup.Modes = (uint32_t) 1;
  PSOCK_SEND(p,&setup,sizeof(setup));
  PSOCK_SEND(p,&setup,sizeof(setup));
  //send_setupResponse(&ps);
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static void set_global_address(void)
{
  uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  //printf("ipaddr: %s",ipaddr);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
	
  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  } 
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_psock_client_process, ev, data)
{
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  uip_ip6addr(&addr, 0xfe80, 0, 0, 0, 0xc30c, 0, 0, 2);
  uip_debug_ipaddr_print(addr);
  printf("\n");

  tcp_connect(&addr, UIP_HTONS(861), NULL);

  printf("Connecting...\n");
  PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

  if(uip_aborted() || uip_timedout() || uip_closed()) {
    printf("Could not establish connection\n");
  } else if(uip_connected()) {
    printf("Connected\n");
    
    PSOCK_INIT(&ps, buffer, sizeof(buffer));
    
    //PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    //handle_servergreeting(&ps);

    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    do {
      //handle_connection(&ps);
      control_client(&ps);
      /*if(progress == 0){
          handle_servergreeting(&ps);
      }
      if(progress == 1){
          send_setupResponse(&ps);
      }*/
      PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    } while(!(uip_closed() || uip_aborted() || uip_timedout()));
    
    printf("\nConnection closed.\n");
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
