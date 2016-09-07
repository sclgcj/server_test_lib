#ifndef TC_ADDR_MANAGE_PRIVATE_H
#define TC_ADDR_MANAGE_PRIVATE_H 1

#include "tc_addr_manage.h"

struct tc_address_oper {
	struct sockaddr* (*addr_create)(struct tc_address* ta);
	struct tc_address* (*addr_encode)(int type, struct sockaddr *addr);
	void (*addr_decode)(struct tc_address *taddr, struct sockaddr *addr);
	void (*addr_destroy)(struct tc_address *addr);
	int (*addr_length)();
};

int
tc_address_add(
	struct tc_address_oper *oper
);

struct tc_address *
tc_address_encode(
	int addr_type,
	struct sockaddr *addr
);

void
tc_address_decode(
	struct tc_address *ta,
	struct sockaddr *addr
);

void
tc_address_destroy(
	struct tc_address *ta
);

struct sockaddr *
tc_address_create(
	struct tc_address *ta
);

int
tc_address_length(
	int type
);

#endif
