#ifndef TC_PARAM_API_PRIVATE_H
#define TC_PARAM_API_PRIVATE_H

#include "tc_param_manage_private.h"

typedef struct tc_param_manage		tc_param_manage_t;

tc_param_manage_t *
tc_param_create();

void
tc_param_destroy(
	tc_param_manage_t *pm
);

#endif
