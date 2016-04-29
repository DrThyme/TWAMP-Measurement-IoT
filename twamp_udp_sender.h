
#ifndef TWAMP_UDP_SENDER_H_
#define TWAMP_UDP_SENDER_H_

/**
 * Print unauthenticated received package. 
 *
 * \param reflect_pkt   The udp packets actual data.
 *
 * \return void 
 */
static void receive_unauth(struct reflector_unauthenticated_test reflect_pkt);
/**
 * Print authenticated received package. 
 *
 * \param reflect_pkt   The udp packets actual data.
 *
 * \return void
 */
static void receive_auth(struct reflector_authenticated_test reflect_pkt);
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
 * Sends predefined packet to a predefined address.
 * Timestamps packet before sending.
 * 
 * \return void
 */
static void send_to_unauth();
/**
 * Sends predefined packet to a predefined address.
 * Timestamps packet before sending.
 *
 * \return void
 */
static void send_to_auth();
/**
 * Runs when test session is done, prints statistics from the run.
 *
 * Prints - the packet loss. 
 *
 * Prints - the average RTT.
 *
 * \return void
 */
static void print_statistics();
/*---------------------------------------------------------------------------*/

#endif /* TWAMP_UDP_SENDER_H_ */
