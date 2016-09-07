#include "tc_comm.h"
#include "tc_err.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_addr_unix.h"
#include "tc_addr_manage_private.h"

struct tc_addr_unix_data {
	int addr_id;
};

static struct tc_addr_unix_data global_unix_data;

int
tc_addr_unix_id_get()
{
	return global_unix_data.addr_id;
}

static int
tc_addr_unix_length()
{
	return sizeof(struct sockaddr_un);
}

static struct sockaddr *
tc_addr_unix_create(
	struct tc_address *ta
)
{
	struct sockaddr_un *addr = NULL;
	struct tc_addr_unix *tu = NULL;

	addr = (struct sockaddr_un*)calloc(1, sizeof(*addr));
	if (!addr) 
		TC_PANIC("Not enough memory for %d bytes\n", sizeof(*addr));
	if (ta) {
		tu = (struct tc_addr_unix *)ta->data;
		memcpy(addr->sun_path, tu->unix_path, TC_UNIX_PATH_MAX);
	}

	return (struct sockaddr*)addr;
}

static struct tc_address*
tc_addr_unix_encode(
	int addr_type,
	struct sockaddr *addr
)
{
	struct tc_addr_unix *tu = NULL;
	struct tc_address *ta = NULL;
	struct sockaddr_un *in_addr = NULL;
	
	in_addr = (struct sockaddr_un*)addr;
	
	ta = (struct tc_address *)calloc(1, sizeof(*ta) + sizeof(*tu));
	if (!ta) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	ta->addr_type = addr_type;
	tu = (struct tc_addr_unix*)ta->data;
	memcpy(tu->unix_path, in_addr->sun_path, TC_UNIX_PATH_MAX);
	
	return ta;
}

static void
tc_addr_unix_decode(
	struct tc_address *ta,
	struct sockaddr *addr
)
{
	struct tc_addr_unix *tu = NULL;
	struct sockaddr_un *in_addr = NULL;

	in_addr = (struct sockaddr_un *)addr;
	tu = (struct tc_addr_unix*)ta->data;
	in_addr->sun_family = AF_UNIX;
	memcpy(in_addr->sun_path, tu->unix_path, TC_UNIX_PATH_MAX);
}

static void
tc_addr_unix_destroy(
	struct tc_address *ta
)
{
	TC_FREE(ta);
}

static int
tc_addr_unix_setup()
{
	struct tc_address_oper oper;

	oper.addr_destroy = tc_addr_unix_destroy;
	oper.addr_decode  = tc_addr_unix_decode;
	oper.addr_encode  = tc_addr_unix_encode;
	oper.addr_create  = tc_addr_unix_create;
	oper.addr_length  = tc_addr_unix_length;

	global_unix_data.addr_id = tc_address_add(&oper);

	return TC_OK;
}

int
tc_addr_unix_init()
{
	int ret = 0;	

	ret = tc_init_register(tc_addr_unix_setup);
	if (ret != TC_OK)
		return ret;

	return TC_OK;
}

TC_MOD_INIT(tc_addr_unix_init);
