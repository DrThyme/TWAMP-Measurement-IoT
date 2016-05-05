/*
mode will be 1 for unauth
with unauth challnge, salt and count are not used 
*/
static ServerGreeting
mk_srv_gret(uint32_t mode) { 
  ServerGreeting srvGret;
  uint32_t[12] MBZ = {0};
  srvGret.Modes = mode;
  srvGret.Challenge = 666; 
  srvGret.Salt = 42; 
  srvGret.Count = 1024; 
  srvGret.MBZ = MBZ;
  return srvGret;
}

static SetupResponseUAuth
mk_resp_uauth() { 
  SetupResponseUAuth response;
  response.modes = 1;
  return response;
}


/*
Accept value is 0 if eveything is fine and non-zero for different errors.

1    Failure, reason unspecified (catch-all).
2    Internal error.
3    Some aspect of request is not supported.
4    Cannot perform request due to permanent resource limitations.
5    Cannot perform request due to temporary resource limitations.

*/
static ServerStartMsg
mk_server_start(uint8_t accept) { 
  ServerStartMsg srvStart;
  uint8_t[15] MBZ1 = {0};
  uint8_t[8] MBZ2 = {0};
  srvStart.MBZ1 = MBZ1;
  srvStart.MBZ2 = MBZ2;
  srvStart.Accept = accept;
  srvStart.ServerIV = 0; // unused in unauth
  return srvStart;
}

static RequestSession 
mk_rqst(uint8_t type, 
	uint8_t ipv, 
	uint8_t confSend, 
	uint8_t confRecv, 
	uint32_t schedslots,
	uint32_t npackets,
	uint16_t recvPort,
	uint16_t sendPort,
	uint32_t sendAddr,
	uint32_t recvAddr,
	uint8_t sendMbz,
	uint8_t recvMbz,
	uint8_t sid,
	uint32_t pad,
	TWAMPtimestamp start,
	TWAMPtimestamp timeout,
	uint32_t typePDesc) {
  RequestSession rqstSess;
  rqstSess.Type = type;
  rqstSess.IPVN = ipv;
  rqstSess.ConfSender = confSend;
  rqstSess.ConfReciver = confRecv;
  rqstSess.ScheduleSlots = schedslots;
  rqstSess.NumOfPackets = npackets;
  rqstSess.SenderPort = sendPort;
  rqstSess.RecieverPort = recvPort;
  rqstSess.SenderAddress = sendAddr;
  rqstSess.RecieverAddress = recvAddr;
  rqstSess.SID = sid;
  rqstSess.PaddingLen = pad;
  rqstSess.StartTime = start;
  rqstSess.Timeout = timeout;
  rqstSess.TypePDesc = typePDesc;
  //TODO: understand how these should be used to store the remaining part of the ipv6 address.
  //rqstSess.SenderMBZ
  //rqstSess.RecieverMBZ
  return rqstSess;
}

static AcceptSession
mk_accept() {
  
}
