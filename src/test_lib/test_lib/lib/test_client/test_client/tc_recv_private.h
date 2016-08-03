#ifndef TC_RECV_PRIVATE_H
#define TC_RECV_RPIVATE_H 1

#include "tc_comm.h"

struct tc_recv_node {
	unsigned long user_data;
	struct list_head node;
};


int 
tc_recv_node_add(
	unsigned long user_data
);

#endif
