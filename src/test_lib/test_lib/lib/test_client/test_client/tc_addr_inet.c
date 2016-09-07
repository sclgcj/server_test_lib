#include "tc_comm.h"
#include "tc_err.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_addr_inet.h"
#include "tc_addr_manage_private.h"

struct tc_addr_inet_data {
	int addr_id;
};

static struct tc_addr_inet_data global_inet_data;

int
tc_addr_inet_id_get()
{
	return global_inet_data.addr_id;
}

static int
tc_addr_inet_length()
{
	return sizeof(struct sockaddr_in);
}

static struct sockaddr *
tc_addr_inet_create(
	struct tc_address *ta
)
{
	struct sockaddr_in *addr = NULL;
	struct tc_addr_inet *ti = NULL;

	addr = (struct sockaddr_in*)calloc(1, sizeof(*addr));
	if (!addr) 
		TC_PANIC("Not enough memory for %d bytes\n", sizeof(*addr));

	if (ta) {
		ti = (struct tc_addr_inet*)ta->data;
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = ti->ip;
		addr->sin_port = htons(ti->port);
	}

	return (struct sockaddr*)addr;
}

static struct tc_address*

tc_addr_inet_encode(
	int addr_type,
	struct sockaddr *addr
)
{
	struct tc_addr_inet *ti = NULL;
	struct tc_address *ta = NULL;
	struct sockaddr_in *in_addr = NULL;
	
	in_addr = (struct sockaddr_in*)addr;
	
	ta = (struct tc_address *)calloc(1, sizeof(*ta) + sizeof(*ti));
	if (!ta) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	ta->addr_type = addr_type;
	ti = (struct tc_addr_inet*)ta->data;
	ti->ip = in_addr->sin_addr.s_addr;
	ti->port = ntohs(in_addr->sin_port);
	
	return ta;
}

static void
tc_addr_inet_decode(
	struct tc_address *ta,
	struct sockaddr *addr
)
{
	struct tc_addr_inet *ti = NULL;
	struct sockaddr_in *in_addr = NULL;

	in_addr = (struct sockaddr_in *)addr;
	ti = (struct tc_addr_inet*)ta->data;
	in_addr->sin_family = AF_INET;
	in_addr->sin_port = htons(ti->port);
	in_addr->sin_addr.s_addr = ti->ip;
}

static void
tc_addr_inet_destroy(
	struct tc_address *ta
)
{
	TC_FREE(ta);
}

static int
tc_addr_inet_setup()
{
	struct tc_address_oper oper;

	oper.addr_destroy = tc_addr_inet_destroy;
	oper.addr_decode  = tc_addr_inet_decode;
	oper.addr_encode  = tc_addr_inet_encode;
	oper.addr_create  = tc_addr_inet_create;
	oper.addr_length  = tc_addr_inet_length;

	global_inet_data.addr_id = tc_address_add(&oper);

	return TC_OK;
}

int
tc_addr_inet_init()
{
	int ret = 0;	

	ret = tc_init_register(tc_addr_inet_setup);
	if (ret != TC_OK)
		return ret;

	return TC_OK;
}

TC_MOD_INIT(tc_addr_inet_init);
