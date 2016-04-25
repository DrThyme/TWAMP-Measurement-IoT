
#ifndef TWAMP_UDP_REFLECTOR_H_
#define TWAMP_UDP_REFLECTOR_H_

/**
 * Print unauthenticated received package.
 * Prepares and sends a packated based on the received packet.
 * Timestamps before sending  
 *
 * \param sender_pkt    The received packets actual data.
 *
 * \param ts_rcv        The timestamp from when the senders packet was received.
 *
 * \param sender_addr   The senders IP address.
 *
 * \return void 
 */
static void reflect_uauth(struct sender_unauthenticated_test sender_pkt,
	      struct TWAMPtimestamp ts_rcv,
              const uip_ipaddr_t *sender_addr);
/**
 * Print authenticated received package.
 * Prepares and sends a packated based on the received packet.
 * Timestamps before sending  
 *
 * \param sender_pkt    The received packets actual data.
 *
 * \param ts_rcv        The timestamp from when the senders packet was received.
 *
 * \param sender_addr   The senders IP address.
 *
 * \return void
 */
static void reflect_auth(struct sender_authenticated_test sender_pkt,
	     struct TWAMPtimestamp ts_rcv,
	     const uip_ipaddr_t *sender_addr);
/**
 * Handles recieved packets by delegating them to the correct receive function. 
 * Timestamps any incomming messages. 
 *
 * \param c              The specified udp connection.
 *
 * \param sender_addr    Senders IP address.
 *
 * \param sender_port    Senders port used to send udp packets.
 *
 * \param receiver_addr  Receivers IP address.
 *
 * \param receiver_port  Recievers port used to receive upd packets.
 *
 * \param data           The udp packets actual data.
 *
 * \param datalen        The entire udp packets length.
 *
 * \return void
 */
static void receiver(struct simple_udp_connection *c,
		const uip_ipaddr_t *sender_addr,
		uint16_t sender_port,
		const uip_ipaddr_t *receiver_addr,
		uint16_t receiver_port,
		const uint8_t *data,
		     uint16_t datalen);
/**
 * Init function for the node, gives the node an global address. 
 *
 * \return void
 */
static void set_global_address(void);
/**
 * Initializes the rpl dag.
 * 
 * \param ipaddr       The nodes global IP address.
 * 
 * \return void
 */
static void create_rpl_dag(uip_ipaddr_t *ipaddr);
/*---------------------------------------------------------------------------*/

#endif /* TWAMP_UDP_REFLECTOR_H_ */
