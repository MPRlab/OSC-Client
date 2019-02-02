#include "UDPBroadcastSocket.h"

template<typename S>
UDPBroadcastSocket::UDPBroadcastSocket(S* stack) : UDPSocket() {

	open(stack);
}

void UDPBroadcastSocket::set_broadcast(bool broadcast) {
	struct lwip_socket *s = (struct lwip_socket *)_socket;
	if (broadcast)
		s->conn->pcb.ip->so_options |= SOF_BROADCAST;
	else
		s->conn->pcb.ip->so_options &= ~SOF_BROADCAST;
}