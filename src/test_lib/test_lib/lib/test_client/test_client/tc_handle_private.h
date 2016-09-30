#ifndef TC_HANDLE_PRIVATE_H
#define TC_HANDLE_PRIVATE_H 


#include "tc_std_comm.h"

struct tc_address;
int
tc_handle_node_add(
	unsigned long user_data
);


int
tc_hub_destroy();

#endif
