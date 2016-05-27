#include <stdio.h>
#include <stdlib.h>
#include "contiki-net.h"
#include "servreg-hack.h"
#include "clock.h"
#include "twamp.h"

/*
 * We define one protosocket since we've decided to only handle one
 * connection at a time. If we want to be able to handle more than one
 * connection at a time, each parallell connection needs its own
 * protosocket.
 */
static struct psock ps;

/*
 * We must have somewhere to put incoming data, and we use a 100 byte
 * buffer for this purpose.
 */
static uint8_t buffer[100];
static int state = 1;

static uip_ipaddr_t addr;

PROCESS(twamp_tcp_control_client, "TWAMP TCP Control Client");
AUTOSTART_PROCESSES(&twamp_tcp_control_client);
/*---------------------------------------------------------------------------*/
/*
 * A protosocket always requires a protothread. The protothread
 * contains the code that uses the protosocket. We define the
 * protothread here.
 */
static
PT_THREAD(connection_setup(struct psock *p))
{
  /*
   * A protosocket's protothread must start with a PSOCK_BEGIN(), with
   * the protosocket as argument.
   */
  PSOCK_BEGIN(p);
  
  /*
   * Here we define all the thread local variables that we need to
   * utilize.
   */
  static ServerGreeting greet;
  static SetupResponseUAuth setup;
  static ServerStartMsg start;
  int i;
  int acceptedMode = 1;

  /* 
   * We wait until we receive a server greeting from the server.
   * PSOCK_NEWDATA(p) returns 1 when new data has arrived in 
   * the protosocket.  
   */
  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  if(PSOCK_NEWDATA(p)){
    /*
     * We read data from the buffer now that it has arrived.
     * Using memcpy we store it in our local variable.
     */
    PSOCK_READBUF(p);
    memcpy(&greet,buffer,sizeof(greet));
    
    /*
     * We check to see if the server accepts our communication.
     * If not we terminate.
     */
    if(greet.Modes != acceptedMode){
      printf("Server did not match our modes!\n");
      PSOCK_CLOSE_EXIT(p);
    } else{
      /*
       * Temporary prints for debugging purpose.
       */
      printf("Greet modes: %"PRIu32"\n",greet.Modes);
      printf("Greet challenge: ");
      for (i = 0; i < 16; i++){
        printf("%d",greet.Challenge[i]);}
      printf("\nGreet salt: ");
      for (i = 0; i < 16; i++){
        printf("%d",greet.Salt[i]);}
      printf("\n");
    
      /*
       * We respond to the server-greeting with a set-up-response
       * message. We set the mode we wish to use and send the message
       * to the server. 
       */
      memset(&setup,0,sizeof(setup));
      setup.Modes = (uint32_t) 1;
      /*
       * BUG: The message is sent twice, seems this has to be done
       * in order to properly receive it. 
       */
      PSOCK_SEND(p,&setup,sizeof(setup));
      PSOCK_SEND(p,&setup,sizeof(setup));
      /* 
       * We wait until we receive a response from the server.
       * PSOCK_NEWDATA(p) returns 1 when new data has arrived in 
       * the protosocket.  
       */
      PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
      if(PSOCK_NEWDATA(p)){
      /*
       * We read data from the buffer now that it has arrived.
       * Using memcpy we store it in our local variable.
       */
        PSOCK_READBUF(p);
        memcpy(&start,buffer,sizeof(start));

      /*
       * Prints for debugging.
       */
      printf("Accept: %d\n",start.Accept);
      printf("Start seconds: %"PRIu32"\n",start.timestamp.Second);
      printf("Start fractions: %"PRIu32"\n",start.timestamp.Fraction);
      } else{
        printf("Timed out!\n");
      }

    }
  } else {
    printf("Timed out!\n");
    PSOCK_CLOSE_EXIT(p);
  }
  state = 2;
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(create_test_session(struct psock *p))
{ 
  /*
   * A protosocket's protothread must start with a PSOCK_BEGIN(), with
   * the protosocket as argument.
   */
  PSOCK_BEGIN(p);
  
  /*
   * Here we define all the thread local variables that we need to
   * utilize.
   */
  static RequestSession request;
  static AcceptSession accept;

  memset(&request, 0, sizeof(request));
  request.Type = 1;
  request.IPVN = 6;
  request.ConfSender = 0;
  request.ConfReciever = 0;
  request.ScheduleSlots = 0;
  request.NumOfPackets = 15;
  request.SenderPort = 2222;
  request.RecieverPort = 3333;

  if(request.IPVN == 4){
    //currently not implemented.
  } else{
    request.SenderAddress = (uint32_t)0; 
    request.SenderMBZ[0] = 0; 
    request.RecieverAddress = (uint32_t)0; 
    request.RecieverMBZ[0] = 0; 
  }
  request.SID[0] = 0;
  request.PaddingLen = 28;
  request.StartTime.Second = 30;
  request.StartTime.Fraction = 0;
  request.Timeout.Second = 120;
  request.Timeout.Fraction = 0;
  request.TypePDesc = 0;

  PSOCK_SEND(p, &request, sizeof(request));

  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  if(PSOCK_NEWDATA(p)){
    /*
     * We read data from the buffer now that it has arrived.
     * Using memcpy we store it in our local variable.
     */
    PSOCK_READBUF(p);
    memcpy(&accept,buffer,sizeof(accept));
    if(accept.Accept != 0){
      PSOCK_CLOSE_EXIT(p);
    } else {
      printf("Server accepted, got port: %d\n",accept.Port);
    }
  } else{
    printf("Timed out!");
  }
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
PROCESS_THREAD(twamp_tcp_control_client, ev, data)
{

  PROCESS_BEGIN();

  set_global_address();

  uip_ip6addr(&addr, 0xfe80, 0, 0, 0, 0xc30c, 0, 0, 2);
  uip_debug_ipaddr_print(addr);
  printf("\n");
  uint16_t a;
  a = (addr.u8[0] << 8) + addr.u8[1];
  printf("Test: %x\n",a);
  int i; 
  for(i=0; i<16;i++){
    printf("%02x", addr.u8[i]);
  }

  /*
   * We start with connecting to the server using the 
   * well known port 862. UIP_HTONS() is used to convert
   * the port number to network byte order.
   */
  tcp_connect(&addr, UIP_HTONS(862), NULL);

  /*
   * We wait for the first TCP/IP event, most likely due
   * to someone responding with an ACK to our connect request.
   */
  printf("Connecting...\n");
  PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

  /*
   * If we are unable to make a connection due to various
   * reasons we exit gracefully.
   */
  if(uip_aborted() || uip_timedout() || uip_closed()) {
    printf("Could not establish connection\n");
  /*
   * If a connection is successfully made we initialize the 
   * protosocket.
   */
  } else if(uip_connected()) {
    printf("Connected\n");
    /*
     * The PSOCK_INIT() function initializes the protosocket and
     * binds the input buffer to the protosocket.
     */
    PSOCK_INIT(&ps, buffer, sizeof(buffer));
    
    /*
     * We wait to allow the server to catch up.
     */
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    /*
     * We loop until the connection is aborted, closed, or times out.
     */
    do {
      /*
       * Run the client side of the control protocol.
       */
        if(state == 1){
          connection_setup(&ps);
        }
        if(state == 2){
          create_test_session(&ps);
        }

      PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    } while(!(uip_closed() || uip_aborted() || uip_timedout()));
    
    printf("\nConnection closed.\n");
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
