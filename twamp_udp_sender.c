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
#define TEST_SESSION 1
#define TEST_AMOUNT 10

static struct simple_udp_connection unicast_connection;

static unsigned int seq_id;
static uip_ipaddr_t *addr;

static clock_time_t start_time;

static int rcv_num_pkt = 0; 
static int total_rtt = 0;

/*---------------------------------------------------------------------------*/
PROCESS(twamp_udp_sender, "TWAMP UDP Sender Process");
AUTOSTART_PROCESSES(&twamp_udp_sender);
/*---------------------------------------------------------------------------*/
static void receive_unauth(struct reflector_unauthenticated_test reflect_pkt){

	//Prints for debugging
	printf("SeqNo: %"PRIu32"\n", reflect_pkt.SeqNo);
	printf("RS-Seconds: %"PRIu32"\n", reflect_pkt.Timestamp.second);
	printf("RS-Micro: %"PRIu32"\n", reflect_pkt.Timestamp.microsecond);
	printf("Error: %"PRIu16"\n", reflect_pkt.ErrorEstimate);

	printf("R-Seconds: %"PRIu32"\n", reflect_pkt.ReceiverTimestamp.second);
	printf("R-Micro: %"PRIu32"\n", reflect_pkt.ReceiverTimestamp.microsecond);
	printf("Sender SeqNo: %"PRIu32"\n", reflect_pkt.SenderSeqNo);
	printf("S-Seconds: %"PRIu32"\n", reflect_pkt.SenderTimestamp.second);
	printf("S-Micro: %"PRIu32"\n", reflect_pkt.SenderTimestamp.microsecond);
	printf("Sender Error: %"PRIu16"\n", reflect_pkt.SenderErrorEstimate);

	printf("Sender TTL: %d\n", reflect_pkt.SenderTTL);
	if((reflect_pkt.SeqNo - reflect_pkt.SenderSeqNo) == 0){
		printf("RTT was: %"PRIu32" ticks.", (reflect_pkt.Timestamp.second - reflect_pkt.SenderTimestamp.second));
	}
}
/*---------------------------------------------------------------------------*/
static void receive_auth(struct reflector_authenticated_test reflect_pkt){


	//Prints for debugging
	printf("SeqNo: %"PRIu32"\n", reflect_pkt.SeqNo);
	printf("RS-Seconds: %"PRIu32"\n", reflect_pkt.Timestamp.second);
	printf("RS-Micro: %"PRIu32"\n", reflect_pkt.Timestamp.microsecond);
	printf("Error: %"PRIu16"\n", reflect_pkt.ErrorEstimate);

	printf("R-Seconds: %"PRIu32"\n", reflect_pkt.ReceiverTimestamp.second);
	printf("R-Micro: %"PRIu32"\n", reflect_pkt.ReceiverTimestamp.microsecond);

	printf("Sender SeqNo: %"PRIu32"\n", reflect_pkt.SenderSeqNo);
	printf("S-Seconds: %"PRIu32"\n", reflect_pkt.SenderTimestamp.second);
	printf("S-Micro: %"PRIu32"\n", reflect_pkt.SenderTimestamp.microsecond);
	printf("Sender Error: %"PRIu16"\n", reflect_pkt.SenderErrorEstimate);

	printf("Sender TTL: %d\n", reflect_pkt.SenderTTL);
	if((reflect_pkt.SeqNo - reflect_pkt.SenderSeqNo) == 0){
		printf("RTT was: %"PRIu32" ticks.", (reflect_pkt.Timestamp.second - reflect_pkt.SenderTimestamp.second));
	}
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
	printf("Data received from "); 
	uip_debug_ipaddr_print(sender_addr);
	printf(" on port %d from port %d \n",
			receiver_port, sender_port);
	printf("###############\n");
	if(AUTH_MODE == 0){
		ReflectorUAuthPacket reflect_pkt;  
		memset(&reflect_pkt,0,sizeof(reflect_pkt));
		memcpy(&reflect_pkt, data, datalen);
		if(TEST_SESSION == 0) receive_unauth(reflect_pkt);
		if(TEST_SESSION == 1){
			total_rtt += (reflect_pkt.Timestamp.second - reflect_pkt.SenderTimestamp.second);
			rcv_num_pkt++;
		} 
	}
	else if(AUTH_MODE == 1){
		ReflectorAuthPacket reflect_pkt;  
		memset(&reflect_pkt,0,sizeof(reflect_pkt));
		memcpy(&reflect_pkt, data, datalen);
		receive_auth(reflect_pkt);
	}
}
/*---------------------------------------------------------------------------*/
static void set_global_address(void)
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
	pkt.SeqNo = seq_id;
	pkt.ErrorEstimate = 666;

	printf("Sending unicast to ");
	uip_debug_ipaddr_print(addr);
	printf("\n");

	seq_id++;
	t.second = clock_time() - start_time;
	t.microsecond = 0;
	pkt.Timestamp = t;
	simple_udp_sendto(&unicast_connection, &pkt, sizeof(pkt), addr);
}

/*---------------------------------------------------------------------------*/
static void
send_to_auth(){
	static struct sender_authenticated_test pkt;
	static struct TWAMPtimestamp t;
	pkt.SeqNo = seq_id;
	pkt.ErrorEstimate = 666;

	printf("Sending unicast to ");
	uip_debug_ipaddr_print(addr);
	printf("\n");

	seq_id++;
	t.second = clock_time() - start_time;
	t.microsecond = 0;
	pkt.Timestamp = t;
	simple_udp_sendto(&unicast_connection, &pkt, sizeof(pkt), addr);
}
/*---------------------------------------------------------------------------*/
static void print_statistics(){
	int pkt_success = TEST_AMOUNT - rcv_num_pkt - 1;
	int avg_rtt = total_rtt/rcv_num_pkt;
	printf("------------------------------------\n");
	printf("We lost %d packets.\n", pkt_success);
	printf("Average RTT was: %d\n", avg_rtt);
	printf("------------------------------------\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(twamp_udp_sender, ev, data){
	static struct etimer periodic_timer;
	static struct etimer send_timer;

	PROCESS_BEGIN();

	servreg_hack_init();

	set_global_address();

	start_time = clock_time();

	simple_udp_register(&unicast_connection, UDP_PORT,
			NULL, UDP_PORT, receiver);

	etimer_set(&periodic_timer, SEND_INTERVAL);
	if(TEST_SESSION == 0){
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
	}else if(TEST_SESSION == 1){
		static int i = 0;
		while(i < TEST_AMOUNT) {
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
			etimer_reset(&periodic_timer);
			etimer_set(&send_timer, SEND_TIME);

			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
			addr = servreg_hack_lookup(SERVICE_ID);
			if(addr != NULL) {
				if(AUTH_MODE == 0){
					send_to_unauth();
					i++;
				}
				else if(AUTH_MODE == 1){
					send_to_auth();
					i++;
				}


			} else {
				printf("Service %d not found\n", SERVICE_ID);
			}
		}
		print_statistics();
		printf("Test session finished!\n");
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
