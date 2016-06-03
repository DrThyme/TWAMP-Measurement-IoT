/*
 * This is a small example of how to write a TCP server using
 * Contiki's protosockets. It is a simple server that accepts one line
 * of text from the TCP connection, and echoes back the first 10 bytes
 * of the string, and then closes the connection.
 *
 * The server only handles one connection at a time.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "clock.h"
#include <string.h>
#include "servreg-hack.h"

#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"



/*
 * We include "contiki-net.h" to get all network definitions and
 * declarations.
 */
#include "contiki-net.h"
#include "net/rpl/rpl.h"

#include "twamp.h"
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

static uint8_t mask = 0xFF;
/*
 * Global variables that are used within several functions
 */
static int state = 1;
static int current = 0;
static int UDP_SENDER_PORT;
static int UDP_RECEIVER_PORT;
static int TEST_AMOUNT;
static int seqno = 0;

static struct simple_udp_connection unicast_connection;
/*---------------------------------------------------------------------------*/
/*
 * Timesynch functionality
 */
static
PT_THREAD(timesynch(struct psock *p))
{
  PSOCK_BEGIN(p);
  printf("TIME: %d\n",clock_time());
  static TimesynchSRMsg time_pkt;
  memset(&time_pkt,0,sizeof(time_pkt));

  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  if(PSOCK_NEWDATA(p)){
    clock_time_t t2 = clock_time();
    PSOCK_READBUF(p);
    memcpy(&time_pkt,buffer,sizeof(time_pkt));
    
    time_pkt.t1 = time_pkt.t1;
    time_pkt.t2 = t2;
    time_pkt.t3 = clock_time();
    
    printf("Clock print: %u\n",clock_time());
    PSOCK_SEND(p,&time_pkt,sizeof(time_pkt));
    printf("Clock print: %u\n",clock_time());

    printf("T1: %u\n",time_pkt.t1);
    printf("T2: %u\n",time_pkt.t2);
    printf("T3: %u\n",time_pkt.t3);

  } else {
    printf("Timed out!\n");
    PSOCK_CLOSE_EXIT(p);
  }
  state = 4;
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t * 
set_global_address(void)
{
  uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
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
  return &ipaddr;
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
  printf("TIME: %d\n",clock_time());

  /*
   * Here we define all the thread local variables that we need to
   * utilize.
   */
  static ServerGreeting greet;
  static SetupResponseUAuth setup;
  static ServerStartMsg start;
  static int i;
  static int acceptedMode = 1;

  /* 
   * We configure the Server-Greeting that we want to send. Setting
   * the accepted modes to those we accept. The following modes are 
   * meaningful:
   * 1 - Unauthenticated
   * 2 - Authenticated
   * 3 - Encrypted
   * 0 - Do not wish to communicate.
   */
  memset(&greet, 0, sizeof(greet));
  greet.Modes = 1;
  /*
   * We generate random sequence of octects for the challenge and
   * salt. 
   */
  for (i = 0; i < 16; i++){
      greet.Challenge[i] = rand() % 16;
      }
  for (i = 0; i < 16; i++){
      greet.Salt[i] = rand() % 16;
      }
  /*
   * Count must be a power of 2 and be at least 1024.
   */
  //greet.Count = (1 << 12);
  /*
   * We set the MBZ octets to zero.
   */
  for (i = 0; i < 12; i++){
    greet.MBZ[i] = 0;
  }

  /*
   * Using PSOCK_SEND() we send the Server-Greeting to the connected
   * client.
   */
  PSOCK_SEND(p, &greet, sizeof(greet));

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
    memcpy(&setup,buffer,sizeof(setup));
    if(setup.Modes != acceptedMode){
      printf("Client did not match our modes!\n");
      PSOCK_CLOSE_EXIT(p);
    } else{
      /*
       * We have agreed upon the mode. Now we send the Server-Start
       * message.
       */
      memset(&start,0,sizeof(start));
      /* 
       * We set the MBZ octets to zero.
       */
      for (i = 0; i < 15; i++){
        start.MBZ1[i] = 0;
      }
      for (i = 0; i < 8; i++){
        start.MBZ2[i] = 0;
      }
      /*
       * The accept field is set to zero if the server wishes to continue
       * communicating. A non-zero value is defined as in RFC 4656.
       */
      start.Accept = 0;
      /*
       * Timestamp is set to the time the Server started.
       */
      double temp;
      start.timestamp.Second = clock_seconds();
      temp = (double) clock_time()/CLOCK_SECOND - start.timestamp.Second;
      start.timestamp.Fraction = temp*1000;
      /*
       * Using PSOCK_SEND() we send the Server-Start to the connected
       * client.
       */
      PSOCK_SEND(p, &start, sizeof(start));
      printf("Client agreed to mode: %d\n",setup.Modes);
    }
  } else {
    printf("Timed out!\n");
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
  
  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  if(PSOCK_NEWDATA(p)){
    /*
     * We read data from the buffer now that it has arrived.
     * Using memcpy we store it in our local variable.
     */
    PSOCK_READBUF(p);
    memcpy(&request,buffer,sizeof(request));

    TEST_AMOUNT = (int) request.NumOfPackets;
    UDP_SENDER_PORT = (int) request.SenderPort;
    UDP_RECEIVER_PORT = (int) request.RecieverPort;
    
    /*
     * Prints for debugging.
     */
    printf("Type: %"PRIu32"\n",request.Type);
    printf("SenderPort: %"PRIu32"\n",request.SenderPort);
    printf("ReceiverPort: %"PRIu32"\n",request.RecieverPort);
    printf("SenderAddress: %08x,%x\n",request.SenderAddress,request.SenderMBZ);
    printf("ReceiverAddress: %08x,%x\n",request.RecieverAddress,request.RecieverMBZ);
    printf("StartTime: %u\n",request.StartTime.Second);
    
    accept.Accept = 0;
    accept.Port = request.RecieverPort; 

    PSOCK_SEND(p, &accept, sizeof(accept));
    PSOCK_SEND(p, &accept, sizeof(accept));
  } else {
    printf("Timed out!\n");
  }  
  state = 3;
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
	 const uint8_t *data,
         uint16_t datalen)
{
  static int cl_temp;
  static int cl_sec;
  unsigned int clock;
  unsigned int second;
  double temp;

  //printf("recieved msg at clock time: %d \n",clock_time()-  offset - prop_delay);
  //printf( "%d \n",clock_time()-  offset - prop_delay);
    

  TWAMPtimestamp ts_rcv;

  clock = clock_time();
  second = clock/CLOCK_SECOND;
  printf("RECEIVE AT: clock: %d || second: %d \n",clock,second);
  //temp = (double) (clock - start_time)/CLOCK_SECOND - second;
  temp = (double) (clock)/CLOCK_SECOND - second;
  ts_rcv.Second = second;
  ts_rcv.Fraction = temp * 1000;

  printf("Data received from ");  
  uip_debug_ipaddr_print(sender_addr);
  printf(" on port %d from port %d \n", receiver_port, sender_port);
  
  printf("Datalen = %d\n",datalen);
  SenderUAuthPacket sender_pkt;
  memset(&sender_pkt, 0, sizeof(sender_pkt));
  memcpy(&sender_pkt, data, datalen);
  
  ReflectorUAuthPacket reflect_pkt;
  //printf("Paket size = %d\n", sizeof(reflect_pkt));
  reflect_pkt.SeqNo = seqno;
  reflect_pkt.ErrorEstimate = 999;
  reflect_pkt.ReceiverTimestamp = ts_rcv;
  reflect_pkt.SenderSeqNo = sender_pkt.SeqNo;
  reflect_pkt.SenderTimestamp = sender_pkt.Timestamp;
  reflect_pkt.SenderErrorEstimate = sender_pkt.ErrorEstimate;
  reflect_pkt.SenderTTL = 255;


  clock = clock_time();
  second = clock/CLOCK_SECOND;
  temp = (double) (clock)/CLOCK_SECOND - second;
  reflect_pkt.Timestamp.Second = second;
  reflect_pkt.Timestamp.Fraction = temp * 1000;

  simple_udp_sendto(&unicast_connection, &reflect_pkt,
                    sizeof(reflect_pkt), sender_addr);

  printf("Packet reflected to:\n");
  uip_debug_ipaddr_print(sender_addr);
  printf("\n#################\n");

  //Prints for debugging
  printf("SeqNo: %"PRIu32"\n", sender_pkt.SeqNo);
  printf("Seconds: %"PRIu32"\n", sender_pkt.Timestamp.Second);
  printf("Micro: %"PRIu32"\n", sender_pkt.Timestamp.Fraction);
  printf("Error: %"PRIu16"\n", sender_pkt.ErrorEstimate);

  current++;
}
/*---------------------------------------------------------------------------*/
PROCESS(run_test_session,"Run Test Session Client");
PROCESS_THREAD(run_test_session,ev,data)
{
  PROCESS_BEGIN();
  current = 0;
  simple_udp_register(&unicast_connection, UDP_RECEIVER_PORT,
                      NULL, UDP_SENDER_PORT, receiver);

  PROCESS_WAIT_UNTIL(current == TEST_AMOUNT-1);
  //PROCESS_WAIT_UNTIL(current == 2749);
 
  PROCESS_END();
  PROCESS_EXIT();
}
/*---------------------------------------------------------------------------*/
/*
 * We declare the process and specify that it should be automatically started.
 */
PROCESS(twamp_tcp_control_server, "TWAMP TCP Control Server");
AUTOSTART_PROCESSES(&twamp_tcp_control_server);
/*---------------------------------------------------------------------------*/
/*
 * The definition of the process.
 */
PROCESS_THREAD(twamp_tcp_control_server, ev, data)
{
  /*
   * The process begins here.
   */
  uip_ipaddr_t *ipaddr;
  
  PROCESS_BEGIN();
  set_global_address();
  /*
   * We start with setting up a listening TCP port. Note how we're
   * using the UIP_HTONS() macro to convert the port number (862) to
   * network byte order as required by the tcp_listen() function.
   */
  tcp_listen(UIP_HTONS(862));

  /*
   * We loop for ever, accepting new connections.
   */
  while(1) {

    /*
     * We wait until we get the first TCP/IP event, which probably
     * comes because someone connected to us.
     */
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

    /*
     * If a peer connected with us, we'll initialize the protosocket
     * with PSOCK_INIT().
     */
    if(uip_connected()) {
      
      /*
       * The PSOCK_INIT() function initializes the protosocket and
       * binds the input buffer to the protosocket.
       */
      PSOCK_INIT(&ps, buffer, sizeof(buffer));
      printf("Someone connected!\n");
      
      /*
       * We loop until the connection is aborted, closed, or times out.
       */
      while(!(uip_aborted() || uip_closed() || uip_timedout())) {
	/*
	 * We wait until we get a TCP/IP event. Remember that we
	 * always need to wait for events inside a process, to let
	 * other processes run while we are waiting.
	 */
	PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
	/*
	 * Here is where the real work is taking place: we call the
	 * handle_connection() protothread that we defined above. This
	 * protothread uses the protosocket to receive the data that
	 * we want it to.
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
          //PT_INIT(&pthread);
          //run_test_session(&ps);
          process_start(&run_test_session,NULL);
          PROCESS_YIELD_UNTIL(!process_is_running(&run_test_session));
        }
      }
    }
  }
  
  /*
   * We must always declare the end of a process.
   */
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
