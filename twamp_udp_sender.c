/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"

#include "sys/node-id.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include <stdio.h>
#include <string.h>

#include "twamp.h"

#define UDP_PORT 1234
#define SERVICE_ID 190

#define SEND_INTERVAL		(60 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define AUTH_MODE 0

static struct simple_udp_connection unicast_connection;

static unsigned int seq_id;
static uip_ipaddr_t *addr;

/*---------------------------------------------------------------------------*/
PROCESS(unicast_sender_process, "Unicast sender example process");
AUTOSTART_PROCESSES(&unicast_sender_process);
/*---------------------------------------------------------------------------*/

static void
receive_unauth(struct reflector_unauthenticated_test reflect_pkt){

 
//Prints for debugging

  printf("SeqNo: %d\n", reflect_pkt.SeqNo);
  printf("RS-Seconds: %d\n", reflect_pkt.Timestamp.second);
  printf("RS-Micro: %d\n", reflect_pkt.Timestamp.microsecond);
  printf("Error: %d\n", reflect_pkt.ErrorEstimate);
  
  printf("Sender SeqNo: %d\n", reflect_pkt.SenderSeqNo);
  printf("R-Seconds: %d\n", reflect_pkt.ReceiverTimestamp.second);
  printf("R-Micro: %d\n", reflect_pkt.ReceiverTimestamp.microsecond);
  printf("Sender Error: %d\n", reflect_pkt.SenderErrorEstimate);
  
  printf("S-Seconds: %d\n", reflect_pkt.SenderTimestamp.second);
  printf("S-Micro: %d\n", reflect_pkt.SenderTimestamp.microsecond);
  printf("Sender TTL: %d\n", reflect_pkt.SenderTTL);
}


/*---------------------------------------------------------------------------*/

static void
receive_auth(struct reflector_authenticated_test reflect_pkt){

 
//Prints for debugging

  printf("SeqNo: %d\n", reflect_pkt.SeqNo);
  printf("RS-Seconds: %d\n", reflect_pkt.Timestamp.second);
  printf("RS-Micro: %d\n", reflect_pkt.Timestamp.microsecond);
  printf("Error: %d\n", reflect_pkt.ErrorEstimate);
  
  printf("Sender SeqNo: %d\n", reflect_pkt.SenderSeqNo);
  printf("R-Seconds: %d\n", reflect_pkt.ReceiverTimestamp.second);
  printf("R-Micro: %d\n", reflect_pkt.ReceiverTimestamp.microsecond);
  printf("Sender Error: %d\n", reflect_pkt.SenderErrorEstimate);
  
  printf("S-Seconds: %d\n", reflect_pkt.SenderTimestamp.second);
  printf("S-Micro: %d\n", reflect_pkt.SenderTimestamp.microsecond);
  printf("Sender TTL: %d\n", reflect_pkt.SenderTTL);
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
  printf("Data received from "); 
  uip_debug_ipaddr_print(sender_addr);
  printf(" on port %d from port %d \n",
         receiver_port, sender_port);
  printf("###############\n");

  if(AUTH_MODE == 0){
    ReflectorUAuthPacket reflect_pkt;  
    memcpy(&reflect_pkt, data, datalen);
    receive_unauth(reflect_pkt);
  }
  else if(AUTH_MODE == 1){
    ReflectorAuthPacket reflect_pkt;  
    memcpy(&reflect_pkt, data, datalen);
    receive_auth(reflect_pkt);
  }
  
 
  

}

/*---------------------------------------------------------------------------*/
static void
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
}
/*---------------------------------------------------------------------------*/
static void
send_to_unauth(){
static struct sender_unauthenticated_test pkt;
      static struct TWAMPtimestamp t;
      t.second = 222;
      t.microsecond = 345;
      pkt.SeqNo = seq_id;
      pkt.Timestamp = t;
      pkt.ErrorEstimate = 666;
      
      printf("Sending unicast to ");
      uip_debug_ipaddr_print(addr);
      printf("\n");
      
      seq_id++;
      simple_udp_sendto(&unicast_connection, &pkt, sizeof(pkt), addr);
}

/*---------------------------------------------------------------------------*/
static void
send_to_auth(){
static struct sender_authenticated_test pkt;
      static struct TWAMPtimestamp t;
      t.second = 222;
      t.microsecond = 345;
      pkt.SeqNo = seq_id;
      pkt.Timestamp = t;
      pkt.ErrorEstimate = 666;
      
      printf("Sending unicast to ");
      uip_debug_ipaddr_print(addr);
      printf("\n");
      
      seq_id++;
      simple_udp_sendto(&unicast_connection, &pkt, sizeof(pkt), addr);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  
  PROCESS_BEGIN();

  servreg_hack_init();

  set_global_address();

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    addr = servreg_hack_lookup(SERVICE_ID);
    if(addr != NULL) {
      if(AUTH_MODE == 0){
	 send_to_unauth();
      }
      else if(AUTH_MODE == 1){
	send_to_auth();
      }

    
    } else {
      printf("Service %d not found\n", SERVICE_ID);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
