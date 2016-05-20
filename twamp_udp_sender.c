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
#include "sys/rtimer.h"
#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"

#include "dev/leds.h"
#include "dev/button-sensor.h"

#include "powertrace.h"

#include "sys/node-id.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include <stdio.h>
#include <string.h>

#include "twamp.h"

#define UDP_PORT 1443
#define UDP_TIMESYNCH_PORT 1554
#define SERVICE_ID 195

#define SYNCH_INTERVAL          (20 * CLOCK_SECOND)
#define SEND_INTERVAL		(4 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

#define AUTH_MODE 0
#define TEST_SESSION 1
#define TEST_AMOUNT 10

#define POWERTRACE 1

static struct simple_udp_connection unicast_connection;
static struct simple_udp_connection unicast_synch_connection;
static struct etimer time_synch;


static unsigned int seq_id;
static uip_ipaddr_t *addr;

static clock_time_t start_time;

static int rcv_num_pkt = 0; 
static int total_rtt = 0;

static int authority_level;
static rtimer_clock_t offset;
static int timesynch_handshake;
static int synch_var;
/*---------------------------------------------------------------------------*/
PROCESS(twamp_udp_sender, "TWAMP UDP Sender Process");
AUTOSTART_PROCESSES(&twamp_udp_sender);
/*---------------------------------------------------------------------------*/
static void receive_unauth(struct reflector_unauthenticated_test reflect_pkt){


	//Prints for debugging
	printf("SeqNo: %"PRIu32"\n", reflect_pkt.SeqNo);
	printf("RS-Seconds: %"PRIu32"\n", reflect_pkt.Timestamp.Second);
	printf("RS-Micro: %"PRIu32"\n", reflect_pkt.Timestamp.Fraction);
	printf("Error: %"PRIu16"\n", reflect_pkt.ErrorEstimate);

	printf("R-Seconds: %"PRIu32"\n", reflect_pkt.ReceiverTimestamp.Second);
	printf("R-Micro: %"PRIu32"\n", reflect_pkt.ReceiverTimestamp.Fraction);
	printf("Sender SeqNo: %"PRIu32"\n", reflect_pkt.SenderSeqNo);
	printf("S-Seconds: %"PRIu32"\n", reflect_pkt.SenderTimestamp.Second);
	printf("S-Micro: %"PRIu32"\n", reflect_pkt.SenderTimestamp.Fraction);
	printf("Sender Error: %"PRIu16"\n", reflect_pkt.SenderErrorEstimate);

	printf("Sender TTL: %d\n", reflect_pkt.SenderTTL);
	//TODO: Make the calculation take into consideration microseconds in relation to seconds. I.e. 6.4 - 5.5 = 0.9 and not 1.-1 
	if((reflect_pkt.SeqNo - reflect_pkt.SenderSeqNo) == 0){
		uint32_t second_temp = reflect_pkt.Timestamp.Second - reflect_pkt.SenderTimestamp.Second;
		uint32_t micro_temp = reflect_pkt.Timestamp.Fraction - reflect_pkt.SenderTimestamp.Fraction;
		if(micro_temp < 0){
			second_temp = second_temp - 1;
			micro_temp = 1000 + micro_temp;
		}
		printf("RTT was: %"PRIu32".%"PRIu32" seconds.", second_temp, micro_temp);
		//printf("RTT was: %"PRIu32".%"PRIu32" seconds.", (reflect_pkt.Timestamp.Second - reflect_pkt.SenderTimestamp.Second), (reflect_pkt.Timestamp.Fraction - reflect_pkt.SenderTimestamp.Fraction));
	}
}
/*---------------------------------------------------------------------------*/
static void receive_auth(struct reflector_authenticated_test reflect_pkt){


	//Prints for debugging
	printf("SeqNo: %"PRIu32"\n", reflect_pkt.SeqNo);
	printf("RS-Seconds: %"PRIu32"\n", reflect_pkt.Timestamp.Second);
	printf("RS-Micro: %"PRIu32"\n", reflect_pkt.Timestamp.Fraction);
	printf("Error: %"PRIu16"\n", reflect_pkt.ErrorEstimate);

	printf("R-Seconds: %"PRIu32"\n", reflect_pkt.ReceiverTimestamp.Second);
	printf("R-Micro: %"PRIu32"\n", reflect_pkt.ReceiverTimestamp.Fraction);

	printf("Sender SeqNo: %"PRIu32"\n", reflect_pkt.SenderSeqNo);
	printf("S-Seconds: %"PRIu32"\n", reflect_pkt.SenderTimestamp.Second);
	printf("S-Micro: %"PRIu32"\n", reflect_pkt.SenderTimestamp.Fraction);
	printf("Sender Error: %"PRIu16"\n", reflect_pkt.SenderErrorEstimate);

	printf("Sender TTL: %d\n", reflect_pkt.SenderTTL);
	//TODO: Make the calculation take into consideration microseconds in relation to seconds. I.e. 6.4 - 5.5 = 0.9 and not 1.-1 
	if((reflect_pkt.SeqNo - reflect_pkt.SenderSeqNo) == 0){
		uint32_t second_temp = reflect_pkt.Timestamp.Second - reflect_pkt.SenderTimestamp.Second;
		uint32_t micro_temp = reflect_pkt.Timestamp.Fraction - reflect_pkt.SenderTimestamp.Fraction;
		if(micro_temp < 0){
			second_temp = second_temp - 1;
			micro_temp = 1000 + micro_temp;
		}
		printf("RTT was: %"PRIu32".%"PRIu32" seconds.", second_temp, micro_temp);
		//printf("RTT was: %"PRIu32".%"PRIu32" seconds.", (reflect_pkt.Timestamp.Second - reflect_pkt.SenderTimestamp.Second), (reflect_pkt.Timestamp.Fraction - reflect_pkt.SenderTimestamp.Fraction));
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

	/*	
		printf("received msg on clock time: %d",clock_time());
		printf("Data received from "); 
		uip_debug_ipaddr_print(sender_addr);
		printf(" on port %d from port %d \n",
		receiver_port, sender_port);
	 */
	if(AUTH_MODE == 0){
		ReflectorUAuthPacket reflect_pkt;  
		//    printf("Sizeof reflect pkt: %d",sizeof(reflect_pkt));
		memset(&reflect_pkt,0,sizeof(reflect_pkt));
		memcpy(&reflect_pkt, data, datalen);
		if(TEST_SESSION == 0) receive_unauth(reflect_pkt);
		if(TEST_SESSION == 1){
			total_rtt += (reflect_pkt.Timestamp.Second - reflect_pkt.SenderTimestamp.Second);
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
send_test(int k){
  if(k == 0){
		Test1 test1;
		simple_udp_sendto(&unicast_connection, &test1, sizeof(test1), addr);
	}
  if(k == 1){
		Test2 test2;
		simple_udp_sendto(&unicast_connection, &test2, sizeof(test2), addr);
	}
  if(k == 2){
		Test3 test3;
		simple_udp_sendto(&unicast_connection, &test3, sizeof(test3), addr);
	}
  if(k == 3){
		Test4 test4;
		simple_udp_sendto(&unicast_connection, &test4, sizeof(test4), addr);
	}
  if(k == 4){
		Test5 test5;
		simple_udp_sendto(&unicast_connection, &test5, sizeof(test5), addr);
	}
  if(k == 5){
		Test6 test6;
		simple_udp_sendto(&unicast_connection, &test6, sizeof(test6), addr);
	}
  if(k == 6){
		Test7 test7;
		simple_udp_sendto(&unicast_connection, &test7, sizeof(test7), addr);
	}
  if(k == 7){
		Test8 test8;
		simple_udp_sendto(&unicast_connection, &test8, sizeof(test8), addr);
	}
  if(k == 8){
		Test9 test9;
		simple_udp_sendto(&unicast_connection, &test9, sizeof(test9), addr);
	}
}
/*---------------------------------------------------------------------------*/
static void receiver_test(struct simple_udp_connection *c,
		const uip_ipaddr_t *sender_addr,
		uint16_t sender_port,
		const uip_ipaddr_t *receiver_addr,
		uint16_t receiver_port,
		const uint8_t *data,
		uint16_t datalen)
{
	//do nothing
}
/*---------------------------------------------------------------------------*/
static void
send_to_unauth(){
	static struct sender_unauthenticated_test pkt;
	static struct TWAMPtimestamp t;
	int second;
	double temp;
	int clock;
	pkt.SeqNo = seq_id;
	pkt.ErrorEstimate = 666;
	/*printf("Sending unicast to ");
	  uip_debug_ipaddr_print(addr);
	  printf("\n");*/

	//printf("Size of send pkt: %d \n",sizeof(pkt));

	seq_id++;
	clock = clock_time();
	second = (clock - start_time)/CLOCK_SECOND;
	temp = (double) (clock - start_time)/CLOCK_SECOND - second;
	t.Second = second;
	t.Fraction = temp * 1000;
	/*printf("Second: %d\n", t.Second);
	  printf("Fraction: %d\n", t.Fraction);*/

	//t.Second = clock_time() - start_time;
	//t.Fraction = 0;
	//  printf("msg send at clock time: %d \n",clock);
	//printf("%d \n",clock);


	pkt.Timestamp = t;
	simple_udp_sendto(&unicast_connection, &pkt, sizeof(pkt), addr);
}

/*---------------------------------------------------------------------------*/
static void
send_to_auth(){
	static struct sender_authenticated_test pkt;
	static struct TWAMPtimestamp t;
	int second;
	int clock;
	double temp;
	pkt.SeqNo = seq_id;
	pkt.ErrorEstimate = 666;

	/*printf("Sending unicast to ");
	  uip_debug_ipaddr_print(addr);
	  printf("\n");*/

	seq_id++;
	clock = clock_time();
	second = (clock - start_time)/CLOCK_SECOND;
	temp = (double) (clock - start_time)/CLOCK_SECOND - second;
	t.Second = second;
	t.Fraction = temp * 1000;
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

	static void
adjust_offset(rtimer_clock_t authoritative_time, rtimer_clock_t local_time)
{
	offset = authoritative_time - local_time;
}


static void timesynch_receiver(struct simple_udp_connection *c,
		const uip_ipaddr_t *sender_addr,
		uint16_t sender_port,
		const uip_ipaddr_t *receiver_addr,
		uint16_t receiver_port,
		const uint8_t *data,
		uint16_t datalen){
	rtimer_clock_t rec_time = clock_time();
	printf("receive time: %d \n", clock_time());

	printf("Time synch msg received on port %d \n ", receiver_port);
	TimesynchMsg data_pkt;

	memset(&data_pkt, 0, sizeof(data_pkt));
	memcpy(&data_pkt, data, datalen);



	if(timesynch_handshake == 1){
		printf("Time synch done \n");
		printf("current time: %d \n", clock_time());
		synch_var = 1;
		return;
	}

	TimesynchMsg time_pkt;
	time_pkt.Timestamp = data_pkt.Timestamp;
	time_pkt.AuthorityLevel = authority_level +1 ;
	time_pkt.Dummy = 1;
	time_pkt.AuthorityOffset = 0;
	time_pkt.Seconds = clock_seconds(); 
	time_pkt.ClockTime = clock_time();
	printf("sent time: %d \n", clock_time());

	time_pkt.PropTime = clock_time() - rec_time;

	simple_udp_sendto(&unicast_synch_connection, &time_pkt, 
			sizeof(time_pkt), sender_addr);
	printf("Absolute time sent \n");
	printf("sent time2: %d \n", clock_time());

	timesynch_handshake = 1;
}

static void timesynch(){

	timesynch_handshake = 0;

	TimesynchMsg time_pkt;

	time_pkt.AuthorityLevel = 0;
	time_pkt.Dummy = 0;
	time_pkt.AuthorityOffset = 0;
	time_pkt.ClockTime = clock_time();
	time_pkt.Seconds = clock_seconds();
	time_pkt.Timestamp = 0;


	simple_udp_sendto(&unicast_synch_connection, &time_pkt, 
			sizeof(time_pkt), addr);
	printf("Time synch start \n");
	printf("size of time _pkt: %d \n",sizeof(time_pkt));
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(twamp_udp_sender, ev, data){
	static struct etimer periodic_timer;
	static struct etimer send_timer;
	static double test;
	static int test2;


	PROCESS_BEGIN();
	SENSORS_ACTIVATE(button_sensor);


	servreg_hack_init();

	set_global_address();

	start_time = clock_time();


	authority_level = 1;

	simple_udp_register(&unicast_synch_connection, UDP_TIMESYNCH_PORT,
			NULL, UDP_TIMESYNCH_PORT, timesynch_receiver);

	etimer_set(&time_synch, SYNCH_INTERVAL);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&time_synch));
	etimer_reset(&time_synch);

	addr = servreg_hack_lookup(SERVICE_ID);

	timesynch();


	simple_udp_register(&unicast_connection, UDP_PORT,
			NULL, UDP_PORT, receiver_test);

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
		static int i;
		static int k = 0;
		static int num;
		static int time;
		static int powertrace_on;
		static int flag;
		SENSORS_ACTIVATE(button_sensor);
		leds_toggle(LEDS_BLUE);
		PROCESS_WAIT_EVENT_UNTIL((ev==sensors_event) && (data == &button_sensor));
		leds_toggle(LEDS_BLUE);
		while(k < 9){
			num = 0;
			while(num < 25){
				i = 0;
				powertrace_on = 0;
				flag = 1;
				//SENSORS_ACTIVATE(button_sensor);
				//leds_toggle(LEDS_BLUE);
				//PROCESS_WAIT_EVENT_UNTIL((ev==sensors_event) && (data == &button_sensor));
				//leds_toggle(LEDS_BLUE);
				etimer_set(&periodic_timer, SEND_INTERVAL);
				while(flag){
					PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
					etimer_reset(&periodic_timer);
					addr = servreg_hack_lookup(SERVICE_ID);
					if(addr != NULL){
						flag = 0;
					} else{
						printf("Service %d not found, retrying!\n", SERVICE_ID);
					}
				}
				etimer_set(&periodic_timer, SEND_INTERVAL);
				while(i < TEST_AMOUNT) {
					if(powertrace_on == 0){
						if(POWERTRACE == 1){
							powertrace_start(CLOCK_SECOND * 2);
							powertrace_on = 1;
						}
					}
					time = (SEND_INTERVAL)/2;
					PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

					etimer_reset(&periodic_timer);
					etimer_set(&send_timer, time);

					PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
					etimer_restart(&send_timer);
					if(AUTH_MODE == 0){
						//send_to_unauth();
						send_test(k);
						i++;
					}
					else if(AUTH_MODE == 1){
						send_to_auth();
						i++;
					}


				}
				//print_statistics();
				if(POWERTRACE == 1){
					powertrace_stop();
				}
				printf(" ***** R NewRepeat Iteration %d *****\n",num+1);
				num++;
				//printf("Test session finished!\n");
				//SENSORS_DEACTIVATE(button_sensor);
			}
			//printf(" ***** N NewSession TEST AMOUNT %d *****\n",TEST_AMOUNT+(10*k));
			printf(" ***** N NewSession PACKET SIZE %d *****\n",10+(10*k));
			k++;
		}
		leds_toggle(LEDS_GREEN);
	}


	else {
		printf("Service %d not found\n", SERVICE_ID);
	}

	print_statistics();
	printf("Test session finished!\n");


	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
