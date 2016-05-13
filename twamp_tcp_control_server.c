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

/*
 * We must have somewhere to put incoming data, and we use a 10 byte
 * buffer for this purpose.
 */
static uint8_t buffer[100];

static uint8_t mask = 0xFF;
static int progress = 0;
/*---------------------------------------------------------------------------*/
/*
 * A protosocket always requires a protothread. The protothread
 * contains the code that uses the protosocket. We define the
 * protothread here.
 */
static
PT_THREAD(handle_connection(struct psock *p))
{
  /*
   * A protosocket's protothread must start with a PSOCK_BEGIN(), with
   * the protosocket as argument.
   *
   * Remember that the same rules as for protothreads apply: do NOT
   * use local variables unless you are very sure what you are doing!
   * Local (stack) variables are not preserved when the protothread
   * blocks.
   */
  PSOCK_BEGIN(p);
  ServerGreeting greet;
  /*
   * We start by sending out a welcoming message. The message is sent
   * using the PSOCK_SEND_STR() function that sends a null-terminated
   * string.
   */
  PSOCK_SEND_STR(p, "Welcome, please type something and press return.\n");
  
  /*
   * Next, we use the PSOCK_READTO() function to read incoming data
   * from the TCP connection until we get a newline character. The
   * number of bytes that we actually keep is dependant of the length
   * of the input buffer that we use. Since we only have a 10 byte
   * buffer here (the buffer[] array), we can only remember the first
   * 10 bytes received. The rest of the line up to the newline simply
   * is discarded.
   */
  //PSOCK_READTO(p, '\n');
  printf("Before read.\n");
  PSOCK_READBUF_LEN(p,sizeof(greet));
  memcpy(&greet,buffer,sizeof(greet));
  printf("Greet modes: %d\n",greet.Modes);
  int i;
  for (i = 0; i < 16; i++){
        printf("%d\n",greet.Challenge[i]);}
  /*
   * And we send back the contents of the buffer. The PSOCK_DATALEN()
   * function provides us with the length of the data that we've
   * received. Note that this length will not be longer than the input
   * buffer we're using.
   */
  //PSOCK_SEND_STR(p, "Got the following data: ");
  //PSOCK_SEND(p, buffer, PSOCK_DATALEN(p));
  //PSOCK_SEND_STR(p, "Good bye!\r\n");

  /*
   * We close the protosocket.
   */
  //PSOCK_CLOSE(p);

  /*
   * And end the protosocket's protothread.
   */
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_serverGreeting(struct psock *p))
{
  PSOCK_BEGIN(p);
  static ServerGreeting greet;
  int i;
  memset(&greet, 0, sizeof(greet));
  greet.Modes = 5;
  printf("Greet modes: %d\n",greet.Modes);
  int r = rand();
  printf("Greet challenge: ");
  for (i = 0; i < 16; i++){
      greet.Challenge[i] = rand() % 16;
      printf("%d",greet.Challenge[i]);}
  printf("\nGreet salt: ");
  for (i = 0; i < 16; i++){
      greet.Salt[i] = rand() % 16;
      printf("%d",greet.Salt[i]);}
  printf("\n");
  //greet.Count = (1 << 12);
  PSOCK_SEND(p, &greet, sizeof(greet));
  //PSOCK_SEND_STR(p, "Hello\n");
  progress++;
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
static int
handle_setupResponse(struct psock *p)
{
  PSOCK_BEGIN(p);
  printf("In setup response.\n");

  SetupResponseUAuth setup;
  PSOCK_READBUF_LEN(p,2);
  memcpy(&setup,buffer,sizeof(setup));
  printf("Client agreed to mode: %d\n",setup.Modes);
  //progress++;
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
static int
control_server(struct psock *p)
{
  PSOCK_BEGIN(p);
  static ServerGreeting greet;
  static int i;
  /* We wait until we receive a server greeting from the server.  */
  //send_serverGreeting(&ps);
  memset(&greet, 0, sizeof(greet));
  greet.Modes = 5;
  //printf("Greet modes: %d\n",greet.Modes);
  int r = rand();
  //printf("Greet challenge: ");
  for (i = 0; i < 16; i++){
      greet.Challenge[i] = rand() % 16;
      //printf("%d",greet.Challenge[i]);
      }
  //printf("\nGreet salt: ");
  for (i = 0; i < 16; i++){
      greet.Salt[i] = rand() % 16;
      //printf("%d",greet.Salt[i]);
      }
  //printf("\n");
  //greet.Count = (1 << 12);
  PSOCK_SEND(p, &greet, sizeof(greet));

  /* Handle setup-response message  */
  static SetupResponseUAuth setup;
  PSOCK_WAIT_UNTIL(p,PSOCK_NEWDATA(p));
  if(PSOCK_NEWDATA(p)){
    PSOCK_READBUF(p);
    //PSOCK_READBUF_LEN(p,sizeof(setup));
    memcpy(&setup,buffer,sizeof(setup));
    printf("Client agreed to mode: %d\n",setup.Modes);
  } else {
    printf("Timed out!\n");
  }
  //memset(&buffer[0], 0, sizeof(buffer));
  
  
  PSOCK_END(p);
}
/*---------------------------------------------------------------------------*/
/*
 * We declare the process and specify that it should be automatically started.
 */
PROCESS(example_psock_server_process, "Example protosocket server");
AUTOSTART_PROCESSES(&example_psock_server_process);
/*---------------------------------------------------------------------------*/
/*
 * The definition of the process.
 */
PROCESS_THREAD(example_psock_server_process, ev, data)
{
  /*
   * The process begins here.
   */
  uip_ipaddr_t *ipaddr;
  
  PROCESS_BEGIN();
  /*
   * We start with setting up a listening TCP port. Note how we're
   * using the UIP_HTONS() macro to convert the port number (1010) to
   * network byte order as required by the tcp_listen() function.
   */
  tcp_listen(UIP_HTONS(861));

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
        //printf("Stuff happened!\n");
	/*
	 * Here is where the real work is taking place: we call the
	 * handle_connection() protothread that we defined above. This
	 * protothread uses the protosocket to receive the data that
	 * we want it to.

	 */
        /*if(progress == 0){
        	send_serverGreeting(&ps);
	}
        if(progress == 1){
        	handle_setupResponse(&ps);
	}*/
	control_server(&ps);
	//handle_connection(&ps);
      }
    }
  }
  
  /*
   * We must always declare the end of a process.
   */
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
