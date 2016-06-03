#include <stdio.h>
#include <stdlib.h>

#include "contiki-net.h"
#include "clock.h"
#include "contiki.h"
#include "lib/random.h"

#include "powertrace.h"
#include "sys/node-id.h"

#include "simple-udp.h"


#include "twamp.h"

#include "dev/leds.h"
#include "dev/button-sensor.h"

#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"


/*
 * Global defines
 */
#define TEST_AMOUNT 100
#define UDP_SENDER_PORT 3333
#define UDP_RECEIVER_PORT 2222
#define RUNS 1

#define SEND_INTERVAL   (2 * CLOCK_SECOND)
#define SEND_TIME   (random_rand() % (SEND_INTERVAL))
#define POWERTRACE 0

/*
 * We define one protosocket since we've decided to only handle one
 * connection at a time. If we want to be able to handle more than one
 * connection at a time, each parallell connection needs its own
 * protosocket.
 */
static struct psock ps;
static struct pt pthread;

/*
 * We must have somewhere to put incoming data, and we use a 100 byte
 * buffer for this purpose.
 */
static uint8_t buffer[100];
static int state = 1;

static uip_ipaddr_t addr;
static struct simple_udp_connection unicast_connection;

static int offset;
static int delay;

PROCESS(twamp_tcp_control_client, "TWAMP TCP Control Client");
AUTOSTART_PROCESSES(&twamp_tcp_control_client);
/*---------------------------------------------------------------------------*/
/*
 * Timesynch functionality
 */
static
PT_THREAD(timesynch(struct psock *p))
{
  PSOCK_BEGIN(p);
  static clock_time_t t4;
  static TimesynchSRMsg time_pkt;
  memset(&time_pkt,0,sizeof(time_pkt));

  printf("Clock print: %u\n",clock_time());
  time_pkt.t1 = clock_time();
  PSOCK_SEND(p,&time_pkt,sizeof(time_pkt));
  //printf("T1 sent: %u\n",time_pkt.t1);
  printf("Clock print: %u\n",clock_time());

  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  if(PSOCK_NEWDATA(p)){
    clock_time_t t4 = clock_time();
    PSOCK_READBUF(p);
    memset(&time_pkt,0,sizeof(time_pkt));
    memcpy(&time_pkt,buffer,sizeof(time_pkt));

    offset = ((int)(time_pkt.t2 - time_pkt.t1) - (int)(t4 - time_pkt.t3))/2;
    delay = ((int)(time_pkt.t2 - time_pkt.t1) + (int)(t4 - time_pkt.t3))/2;

    printf("T1: %u\n",time_pkt.t1);
    printf("T2: %u\n",time_pkt.t2);
    printf("T3: %u\n",time_pkt.t3);
    printf("T4: %u\n",t4);
    printf("Offset: %d\n",offset);
    printf("Propagation: %d\n",delay);

  } else {
    printf("Timed out!\n");
    PSOCK_CLOSE_EXIT(p);
  }
  state = 4;
  PSOCK_END(p);
}
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
  request.NumOfPackets = TEST_AMOUNT;
  request.SenderPort = UDP_SENDER_PORT;
  request.RecieverPort = UDP_RECEIVER_PORT;

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
  state = 3;
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static void receiver(struct simple_udp_connection *c,
		const uip_ipaddr_t *sender_addr,
		uint16_t sender_port,
		const uip_ipaddr_t *receiver_addr,
		uint16_t receiver_port,
		const uint8_t *data,
		uint16_t datalen)
{
  unsigned int second;
  double temp;
  unsigned int clock;
  
  //cl_sec = clock_seconds();
  clock = clock_time() + offset;
  //second = (clock - start_time)/CLOCK_SECOND;
  second = clock/CLOCK_SECOND;
  temp = (double) (clock)/CLOCK_SECOND - second;


  /*	
  printf("received msg on clock time: %d",clock_time());
  printf("Data received from "); 
  uip_debug_ipaddr_print(sender_addr);
  printf(" on port %d from port %d \n",
  receiver_port, sender_port);
  */

  ReflectorUAuthPacket reflect_pkt;  
  memset(&reflect_pkt,0,sizeof(reflect_pkt));
  memcpy(&reflect_pkt, data, datalen);

  int rtt_sec = second - reflect_pkt.SenderTimestamp.Second;
  int rtt_fraction = temp*1000 - reflect_pkt.SenderTimestamp.Fraction;

  if(rtt_fraction < 0){
    rtt_sec = rtt_sec - 1;
    rtt_fraction = 1000 + rtt_fraction;
  }
		
  /*printf("Second: %d, SenderTimestamp: %"PRIu32"\n",second,reflect_pkt.SenderTimestamp.Second);
  printf("Fraction: %d, SenderTimestamp: %"PRIu32"\n",(int)(temp*1000),reflect_pkt.SenderTimestamp.Fraction);
  printf("RTT was: %d.%03d seconds.\n", rtt_sec, rtt_fraction);*/
  printf("%u.%03u,", rtt_sec, rtt_fraction);

  rtt_sec = reflect_pkt.ReceiverTimestamp.Second - reflect_pkt.SenderTimestamp.Second;
  rtt_fraction = reflect_pkt.ReceiverTimestamp.Fraction - reflect_pkt.SenderTimestamp.Fraction;
  if(rtt_fraction < 0){
    rtt_sec = rtt_sec - 1;
    rtt_fraction = 1000 + rtt_fraction;
  }
  /*printf("Second: %"PRIu32", SenderTimestamp: %"PRIu32"\n",reflect_pkt.ReceiverTimestamp.Second,reflect_pkt.SenderTimestamp.Second);
  printf("Fraction: %"PRIu32", SenderTimestamp: %"PRIu32"\n",reflect_pkt.ReceiverTimestamp.Fraction,reflect_pkt.SenderTimestamp.Fraction);
  printf("One-way was: %d.%03d seconds.\n", rtt_sec, rtt_fraction);
  printf("-------------------------------------\n");*/
  printf("%u.%03u\n", rtt_sec, rtt_fraction);
} 
/*---------------------------------------------------------------------------*/
//static
//PT_THREAD(run_test_session(struct pt *p))
PROCESS(run_test_session, "Run Test Session Process");
PROCESS_THREAD(run_test_session,ev,data)
{ 
  PROCESS_BEGIN();

  static struct sender_unauthenticated_test pkt;
  static struct TWAMPtimestamp t;
  
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  static int seq_id;
  static int run;
  static int i;
  static int k;

  static int cl_sec;
  unsigned int second;
  double temp;
  unsigned int clock;
  simple_udp_register(&unicast_connection, UDP_SENDER_PORT,
    NULL, UDP_RECEIVER_PORT, receiver);
  
  k = 0;
  while(k < 10){
    run = 0;
    if(POWERTRACE){
      powertrace_start(CLOCK_SECOND*2);
    }
    while(run < RUNS){
      i = 0;
      seq_id = 0;
      etimer_set(&periodic_timer, SEND_INTERVAL);
      //while(i < TEST_AMOUNT){
      while(i < TEST_AMOUNT+(10*k)){

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        etimer_reset(&periodic_timer);
        etimer_set(&send_timer, SEND_INTERVAL);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
        etimer_restart(&send_timer);
    
        /*
         * Here we send fill and send the test paket
         */
        pkt.SeqNo = seq_id;
        pkt.ErrorEstimate = 666;
        /*printf("Sending unicast to ");
        uip_debug_ipaddr_print(addr);
        printf("\n");*/

        //printf("Size of send pkt: %d \n",sizeof(pkt));

        seq_id++;
        clock = clock_time() + offset;

        second = clock/CLOCK_SECOND;
        temp = (double) (clock)/CLOCK_SECOND - second;
        t.Second = second;
        t.Fraction = temp * 1000;

        pkt.Timestamp = t;
        simple_udp_sendto(&unicast_connection, &pkt, sizeof(pkt), &addr);

        i++;
      }
    run++;
    printf(" ***** R NewRepeat ITERATION %d *****\n",run);
    }
    if(POWERTRACE){
      powertrace_stop();
    }
  printf(" ***** N NewSession %d *****\n",TEST_AMOUNT+(10*k));
  }

  state = 5;

  PROCESS_END();
  PROCESS_EXIT();
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
  static int repeat = 0;

  set_global_address();

  uip_ip6addr(&addr, 0xfe80, 0, 0, 0, 0xc30c, 0, 0, 0x006d);
  //uip_ip6addr(&addr, 0xfe80, 0, 0, 0, 0xc30c, 0, 0, 0x0002);

  SENSORS_ACTIVATE(button_sensor);
  leds_toggle(LEDS_BLUE);
  PROCESS_WAIT_EVENT_UNTIL((ev==sensors_event) && (data == &button_sensor));
  leds_toggle(LEDS_BLUE);
  leds_toggle(LEDS_GREEN);
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
  printf("\nConnecting...\n");
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
        if(state == 3){
          timesynch(&ps);
        }
        if(state == 4){
          process_start(&run_test_session,NULL);
          PROCESS_YIELD_UNTIL(!process_is_running(&run_test_session));
        }

      PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    } while(!(uip_closed() || uip_aborted() || uip_timedout()));
    
    printf("\nConnection closed.\n");
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
