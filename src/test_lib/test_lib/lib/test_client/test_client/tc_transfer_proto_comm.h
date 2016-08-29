#ifndef TC_TRANSFER_PROTO_COMM_H
#define TC_TRANSFER_PROTO_COMM_H

#include "tc_addr_manage.h"

int
tc_transfer_proto_comm_data_send_add(
	char *send_data,
	int  send_len,
	unsigned long user_data
);

int
tc_transfer_proto_comm_udp_data_send_add(
	char *send_data,
	int  send_len,
	struct tc_address *ta,
	unsigned long user_data
);


#endif
