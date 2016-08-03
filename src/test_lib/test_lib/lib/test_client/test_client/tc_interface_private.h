#ifndef TC_INTERFACE_PRIVATE_H
#define TC_INTERFACE_PRIVATE_H 1

#include "tc_comm.h"

cJSON *
tc_interface_param_get(
	char *interface
);

void
tc_interface_func_execute(
	unsigned long user_data
);

void
tc_interface_param_set(
	char *name,
	char *val
);

#endif
