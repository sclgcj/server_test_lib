#ifndef TC_HASH_HUB_PRIVATE_H
#define TC_HASH_HUB_PRIVATE_H 1

#include "tc_create_private.h"

void
tc_hash_hub_link_del(
	unsigned long hub_data
);

int
tc_hash_hub_create(
	char *app_proto,
	struct tc_create_config *conf
);

#endif
