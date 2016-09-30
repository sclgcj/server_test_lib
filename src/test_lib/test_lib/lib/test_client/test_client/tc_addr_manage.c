#include "tc_std_comm.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_init_private.h"
#include "tc_hash.h"
#include "tc_addr_manage_private.h"

struct tc_address_data {
	int tc_address_count;
	pthread_mutex_t mutex;
	tc_hash_handle_t addr_hash;
};

struct tc_address_data_node {
	int addr_type;
	struct tc_address_oper oper;
	struct hlist_node node;
};

#define TC_ADDR_HASH_SIZE 8
static struct tc_address_data global_addr_data;

static int
tc_address_setup()
{
	return TC_OK;
}

static int
tc_address_uninit()
{
	return tc_hash_destroy(global_addr_data.addr_hash);
}

int
tc_address_add(
	struct tc_address_oper *oper
)
{
	int ret = 0;
	struct tc_address_data_node *dnode = NULL;

	dnode = (struct tc_address_data_node *)calloc(1, sizeof(*dnode));
	if (!dnode) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	pthread_mutex_lock(&global_addr_data.mutex);
	dnode->addr_type = global_addr_data.tc_address_count++;
	pthread_mutex_unlock(&global_addr_data.mutex);
	memcpy(&dnode->oper, oper, sizeof(*oper));

	ret = tc_hash_add(global_addr_data.addr_hash, &dnode->node, dnode->addr_type);
	if (ret != TC_OK)
		return ret;

	return dnode->addr_type;
}

static int
tc_address_hash(
	struct hlist_node *hnode,
	unsigned long	  user_data
)
{
	int addr_type = 0;
	struct tc_address_data_node *dnode = NULL;
	
	if (!hnode)
		return TC_ERR;
	if (!hnode)
		addr_type = (int)user_data;
	else {
		dnode = tc_list_entry(hnode, struct tc_address_data_node, node);
		addr_type = dnode->addr_type;
	}

	return (addr_type % TC_ADDR_HASH_SIZE);
}

int
tc_address_length(
	int addr_type
)
{
	struct hlist_node *hnode = NULL;
	struct tc_address_data_node *dnode = NULL;

	hnode = tc_hash_get(global_addr_data.addr_hash, addr_type, addr_type);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGISTER_ADDR);
		return TC_ERR;
	}
	dnode = tc_list_entry(hnode, struct tc_address_data_node, node);
	if (dnode->oper.addr_length)
		return dnode->oper.addr_length();

	return TC_OK;
}

struct sockaddr *
tc_address_create(
	struct tc_address *ta
)
{
	struct hlist_node *hnode = NULL;
	struct tc_address_data_node *dnode = NULL;

	hnode = tc_hash_get(global_addr_data.addr_hash, ta->addr_type, ta->addr_type);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGISTER_ADDR);
		return NULL;
	}
	dnode = tc_list_entry(hnode, struct tc_address_data_node, node);
	if (dnode->oper.addr_create)
		return dnode->oper.addr_create(ta);

	return NULL;
}

struct tc_address *
tc_address_encode(
	int addr_type,
	struct sockaddr *addr
)
{
	struct hlist_node *hnode = NULL;
	struct tc_address_data_node *dnode = NULL;

	hnode = tc_hash_get(global_addr_data.addr_hash, addr_type, addr_type);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGISTER_ADDR);
		return NULL;
	}
	dnode = tc_list_entry(hnode, struct tc_address_data_node, node);
	if (dnode->oper.addr_encode)
		return dnode->oper.addr_encode(addr_type, addr);

	return NULL;
}

void
tc_address_decode(
	struct tc_address *ta,
	struct sockaddr *addr
)
{
	struct hlist_node *hnode = NULL;
	struct tc_address_data_node *dnode = NULL;

	hnode = tc_hash_get(global_addr_data.addr_hash, ta->addr_type, ta->addr_type);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGISTER_ADDR);
		return;
	}
	dnode = tc_list_entry(hnode, struct tc_address_data_node, node);
	if (dnode->oper.addr_decode)
		dnode->oper.addr_decode(ta, addr);
}

void
tc_address_destroy(
	struct tc_address *ta
)
{
	struct hlist_node *hnode = NULL;
	struct tc_address_data_node *dnode = NULL;

	hnode = tc_hash_get(global_addr_data.addr_hash, ta->addr_type, ta->addr_type);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGISTER_ADDR);
		return;
	}
	dnode = tc_list_entry(hnode, struct tc_address_data_node, node);
	if (dnode->oper.addr_destroy)
		dnode->oper.addr_destroy(ta);
}

static int
tc_address_hash_get(
	struct hlist_node *hnode,
	unsigned long	  user_data
)
{
	struct tc_address_data_node *dnode = NULL;

	dnode = tc_list_entry(hnode, struct tc_address_data_node, node);
	if (dnode->addr_type == (int)user_data)
		return TC_OK;

	return TC_ERR;
}

static int
tc_address_hash_destroy(
	struct hlist_node *hnode
)
{
	struct tc_address_data_node *dnode = NULL;

	dnode = tc_list_entry(hnode, struct tc_address_data_node, node);

	TC_FREE(dnode);
}

int
tc_address_init()
{
	int ret = 0;

	memset(&global_addr_data, 0, sizeof(global_addr_data));
	pthread_mutex_init(&global_addr_data.mutex, NULL);
	global_addr_data.addr_hash = tc_hash_create(
						TC_ADDR_HASH_SIZE, 
						tc_address_hash, 
						tc_address_hash_get, 
						tc_address_hash_destroy);

	ret = tc_local_init_register(tc_address_setup);
	if (ret != TC_OK )
		return ret;

	return tc_uninit_register(tc_address_uninit);
}

TC_MOD_INIT(tc_address_init);
