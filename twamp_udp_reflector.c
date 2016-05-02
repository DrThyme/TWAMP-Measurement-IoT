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

#include "clock.h"
#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"

#include "twamp.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include "net/rpl/rpl.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define UDP_PORT 1234
#define SERVICE_ID 190

#define SEND_INTERVAL		(10 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection unicast_connection;
static uint32_t seqno = 0;
static int AUTH_MODE = 0;
static clock_time_t start_time;
/*---------------------------------------------------------------------------*/
PROCESS(twamp_udp_reflector, "TWAMP UDP Reflector Process");
AUTOSTART_PROCESSES(&twamp_udp_reflector);
/*---------------------------------------------------------------------------*/
static void
reflect_uauth(struct sender_unauthenticated_test sender_pkt,
	      struct TWAMPtimestamp ts_rcv,
              const uip_ipaddr_t *sender_addr)
{
  int clock;
  int second;
  double temp;
  //TODO: Set all MBZs to zero!
  ReflectorUAuthPacket reflect_pkt;
  printf("Paket size = %d\n", sizeof(reflect_pkt));
  reflect_pkt.SeqNo = seqno;
  reflect_pkt.ErrorEstimate = 999;
  reflect_pkt.ReceiverTimestamp = ts_rcv;
  reflect_pkt.SenderSeqNo = sender_pkt.SeqNo;
  reflect_pkt.SenderTimestamp = sender_pkt.Timestamp;
  reflect_pkt.SenderErrorEstimate = sender_pkt.ErrorEstimate;
  reflect_pkt.SenderTTL = 255;

  //TODO: Proper timestamp here
  clock = clock_time();
  second = (clock - start_time)/CLOCK_SECOND;
  temp = (double) (clock - start_time)/CLOCK_SECOND - second;
  reflect_pkt.Timestamp.second = second;
  reflect_pkt.Timestamp.microsecond = temp * 1000;

  simple_udp_sendto(&unicast_connection, &reflect_pkt, 
	sizeof(reflect_pkt), sender_addr);

  printf("Packet reflected to:\n");
  uip_debug_ipaddr_print(sender_addr);
  printf("\n#################\n");
  //Prints for debugging
  printf("SeqNo: %"PRIu32"\n", sender_pkt.SeqNo);
  printf("Seconds: %"PRIu32"\n", sender_pkt.Timestamp.second);
  printf("Micro: %"PRIu32"\n", sender_pkt.Timestamp.microsecond);
  printf("Error: %"PRIu16"\n", sender_pkt.ErrorEstimate);
  seqno++;
}
/*---------------------------------------------------------------------------*/
static void
reflect_auth(struct sender_authenticated_test sender_pkt,
	     struct TWAMPtimestamp ts_rcv,
             const uip_ipaddr_t *sender_addr)
{
  int clock;
  int second;
  double temp;
  //TODO: Set all MBZs to zero!
  ReflectorAuthPacket reflect_pkt;

  printf("Paket size = %d\n", sizeof(reflect_pkt));
  reflect_pkt.SeqNo = seqno;
  reflect_pkt.ErrorEstimate = 999;
  reflect_pkt.ReceiverTimestamp = ts_rcv;
  reflect_pkt.SenderSeqNo = sender_pkt.SeqNo;
  reflect_pkt.SenderTimestamp = sender_pkt.Timestamp;
  reflect_pkt.SenderErrorEstimate = sender_pkt.ErrorEstimate;
  reflect_pkt.SenderTTL = 255;

  //TODO: Proper timestamp here
  clock = clock_time();
  second = (clock - start_time)/CLOCK_SECOND;
  temp = (double) (clock - start_time)/CLOCK_SECOND - second;
  reflect_pkt.Timestamp.second = second;
  reflect_pkt.Timestamp.microsecond = temp * 1000;

  simple_udp_sendto(&unicast_connection, &reflect_pkt, 
	sizeof(reflect_pkt), sender_addr);

  printf("Packet reflected to:\n");
  uip_debug_ipaddr_print(sender_addr);
  printf("#################\n");
  //Prints for debugging
  printf("SeqNo: %"PRIu32"\n", sender_pkt.SeqNo);
  printf("Seconds: %"PRIu32"\n", sender_pkt.Timestamp.second);
  printf("Micro: %"PRIu32"\n", sender_pkt.Timestamp.microsecond);
  printf("Error: %"PRIu16"\n", sender_pkt.ErrorEstimate);
  seqno++;
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
  int clock;
  int second;
  double temp;

  TWAMPtimestamp ts_rcv;
  //Do proper timestamp here!
  clock = clock_time();
  second = (clock - start_time)/CLOCK_SECOND;
  temp = (double) (clock - start_time)/CLOCK_SECOND - second;
  ts_rcv.second = second;
  ts_rcv.microsecond = temp * 1000;

  printf("Data received from ");  
  uip_debug_ipaddr_print(sender_addr);
  printf(" on port %d from port %d \n", receiver_port, sender_port);
  
  if(AUTH_MODE == 0){
  	SenderUAuthPacket sender_pkt;
	memset(&sender_pkt, 0, sizeof(sender_pkt));
 	memcpy(&sender_pkt, data, datalen);
    	reflect_uauth(sender_pkt,ts_rcv,sender_addr);
  }else if(AUTH_MODE == 1){
  	SenderAuthPacket sender_pkt;
	memset(&sender_pkt, 0, sizeof(sender_pkt));
	memcpy(&sender_pkt, data, datalen);
	reflect_auth(sender_pkt,ts_rcv,sender_addr);
  }
  
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t *
set_global_address(void)
{
  static uip_ipaddr_t ipaddr;
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
static void
create_rpl_dag(uip_ipaddr_t *ipaddr)
{
  struct uip_ds6_addr *root_if;

  root_if = uip_ds6_addr_lookup(ipaddr);
  if(root_if != NULL) {
    rpl_dag_t *dag;
    uip_ipaddr_t prefix;
    
    rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
    dag = rpl_get_any_dag();
    uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  } else {
    PRINTF("failed to create a new RPL DAG\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(twamp_udp_reflector, ev, data)
{
  uip_ipaddr_t *ipaddr;

  PROCESS_BEGIN();

  servreg_hack_init();

  ipaddr = set_global_address();

  create_rpl_dag(ipaddr);

  servreg_hack_register(SERVICE_ID, ipaddr);

  start_time = clock_time();

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  while(1) {
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
