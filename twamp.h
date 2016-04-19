typedef struct TWAMPtimestamp{
	uint32_t second;
	uint32_t microsecond;
} TWAMPTimestamp;

typedef struct server_greeting{
	uint8_t Unused[12];
	uint32_t Modes;
	uint8_t Challenge[16];
	uint8_t Salt[16];
	uint8_t Count[4];
	uint8_t MBZ[12]; 
} ServerGreeting;

typedef struct set_up_response{
	uint32_t Modes;
	uint8_t KeyID[80];
	uint8_t Token[64];
	uint8_t ClientIV[16];
} SetupResponse;

typedef struct server_start_msg{	
	uint8_t MBZ[15];
	uint8_t Accept;
	uint8_t ServerIV[16];
	TWAMPtimestamp timestamp;
	uint8_t MBZ[8];
} ServerStartMsg;

typedef struct request_session{
	uint8_t Type;
	uint8_t IPVN; // MBZ 4 bits, IPVN 4 bits
	uint8_t ConfSender;
	uint8_t ConfReciever;
	uint32_t ScheduleSlots;
	uint32_t NumOfPackets;
	uint16_t SenderPort;
	uint16_t RecieverPort;
	uint32_t SenderAddress;
	uint8_t Sender_MBZ[12]; //also sender address continued.
	uint32_t RecieverAddress;
	uint8_t Reciever_MBZ[12]; //also receiver address continued.
	uint8_t SID[16];
	uint32_t PaddingLen;
	TWAMPtimestamp StartTime;
	TWAMPtimestamp Timeout;
	uint32_t TypeP_Desc;
	uint8_t MBZ[8];
	uint8_t HMAC[16];
} RequestSession;

typedef struct schedule_slot{
	uint8_t SlotType;
	uint8_t MBZ[7];
	TWAMPtimestamp SlotParameter;
} ScheduleSlot;

typedef struct hmac{
	uint8_t HMAC[16];
} HMAC;

typedef struct accept_session{
	uint8_t Accept;
	uint8_t MBZ;
	uint16_t Port;
	uint8_t SID[16];
	uint8_t MBZ[12];
	uint8_t HMAC[16];
} AcceptSession;

typedef struct start_test_session{
	uint8_t Type;
	uint8_t MBZ[15];
	uint8_t HMAC[16];
	
} StartTest;

typedef struct start_ack{
	uint8_t Accept;
	uint8_t MBZ[15];
	uint8_t HMAC[16];
} StartAck; 

typedef struct stop_session{
	uint8_t Type;
	uint8_t Accept;
	uint16_t MBZ;
	uint32_t NumOfSessions;
	uint8_t MBZ[8];
} StopSession;

typedef struct session_descriptor{
	uint8_t SID[16];
	uint32_t NextSeqno;
	uint32_t NumOfSkipRanges;
} SessionDesc;

typedef struct num_of_skip_ranges{
	uint32_t FirstSeqnoSkipped;
	uint32_t LastSeqnoSkipped;
} SkipRange;

/* 
#####################################################
##########		TWAMP Test specific		#############
#####################################################
*/

#define TEST_PACKET_SIZE 512

typedef struct sender_unauthenticated_test{
	uint32_t SeqNo;
	TWAMPtimestamp Timestamp;
	uint16_t ErrorEstimate;
	//uint8_t Padding[]; size of padding?
} SenderUAuthPacket;

typedef struct reflector_unauthenticated_test{
	uint32_t SeqNo;
	TWAMPtimestamp Timestamp;
	uint16_t ErrorEstimate;
	uint16_t MBZ1;
	TWAMPtimestamp ReceiverTimestamp;
	uint32_t SenderSeqNumb;
	TWAMPtimestamp SenderTimestamp;
	uint16_t SenderErrorEstimate;
	uint16_t MBZ2;
	uint8_t SenderTTL;
	// char[] Padding; packet padding?? how to??
} ReflectorUAuthPacket;

typedef struct sender_authenticated_test{
	uint32_t SeqNo;
	uint8_t MBZ1[12];
	TWAMPtimestamp Timestamp;
	uint16_t ErrorEstimate;
	uint8_t MBZ2[6];
	uint8_t HMAC[16];
	//uint8_t Padding; size of padding?
} SenderAuthPacket;

typedef struct reflector_authenticated_test{
	uint32_t SeqNo;
	uint8_t MBZ1[12];
	TWAMPtimestamp timestamp;
	uint16_t ErrorEstimate;
	uint8_t MBZ2[6];
	TWAMPtimestamp ReceiverTimestamp;
	uint8_t MBZ3[8];
	uint32_t SenderSeqNo;
	uint8_t MBZ4[12];
	TWAMPtimestamp SenderTimestamp;
	uint16_t SenderErrorEstimate;
	uint8_t MBZ5[6];
	uint8_t SenderTTL;
	uint8_t MBZ6[15];
	uint8_t HMAC[16];
	//uint8_t Padding[] what size should the padding be?
} ReflectorAuthPacket;

