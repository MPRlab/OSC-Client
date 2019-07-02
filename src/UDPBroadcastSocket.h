#ifndef UDPBROADCASTSOCKET_H_
#define UDPBROADCASTSOCKET_H_

#include "mbed.h"
#include "UDPSocket.h"
// An ugly hack from a hero at https://os.mbed.com/questions/75208/Broadcast-in-mbedos-5/ to get
// around the fact that mbed-os v5 doesn't have a method for setting a UDPSocket as a broadcasting
// socket (v2 had this functionality)
// -----------------------------------------------
#include "lwip/ip.h"
#include "lwip/api.h"

struct lwip_socket {
	bool in_use;
	struct netconn *conn;
	struct netbuf *buf;
	u16_t offset;

	void(*cb)(void *);
	void *data;
};

class UDPBroadcastSocket: public UDPSocket {
	public:
		template<typename S> UDPBroadcastSocket(S* stack) : UDPSocket() {
			this->open(stack);
		}
		// template<typename S> UDPBroadcastSocket(S* stack);  <-- is apparently incorrect???
		void set_broadcast(bool broadcast);
};

#endif /* UDPBROADCASTSOCKET_H_ */