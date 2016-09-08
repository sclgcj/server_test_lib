#ifndef TC_INTERFACE_PRIVATE_H
#define TC_INTERFACE_PRIVATE_H 1

#include "tc_comm.h"


struct tc_create_link_data;
cJSON *
tc_interface_param_get(
	char *interface
);

int
tc_interface_func_execute(
	struct tc_create_link_data *cl_data
);

void
tc_interface_param_set(
	char *name,
	char *val
);

#endif
