#ifndef TC_SOCKET_H
#define TC_SOCKET_H

#include "tc_comm.h"

int
tc_create_socket(
	int proto,
	int link_linger,
	struct in_addr addr,
	unsigned short port,
	int *sock
);

int
tc_tcp_connect(
	int		sock,
	struct in_addr  addr,
	unsigned short  server_port
);

int
tc_tcp_accept(
	int sock
);

int
tc_udp_connect(
	int		sock,
	struct in_addr  addr,
	unsigned short	server_port
);

int
tc_udp_accept(
	int sock
);

#endif
