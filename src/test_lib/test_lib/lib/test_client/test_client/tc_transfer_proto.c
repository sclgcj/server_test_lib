#include "tc_std_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init_private.h"
#include "tc_hash.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_transfer_proto_private.h"

struct tc_transfer_proto_node {
	int link_type;
	struct tc_transfer_proto_oper oper;
	struct hlist_node node;
};

struct tc_transfer_proto_config_node {
	char *proto_name;
	int  proto;
	struct hlist_node node;
};

struct tc_transfer_proto_config {
	char proto_name[64];
};

struct tc_transfer_proto_data {
	int proto_count;
	pthread_mutex_t  mutex;
	tc_hash_handle_t proto_config_hash;
	tc_hash_handle_t proto_hash;
};

#define TC_PROTO_HASH_SIZE 128
#define TC_PROTO_CONFIG_SIZE 26
static struct tc_transfer_proto_config global_proto_config;
static struct tc_transfer_proto_data global_proto_data;

static int
tc_transfer_proto_type_get(
	char *proto_name
)
{
	struct hlist_node *hnode = NULL;
	struct tc_transfer_proto_config_node *pnode = NULL;

	hnode = tc_hash_get(global_proto_data.proto_config_hash,
			    (unsigned long)proto_name, 
			    (unsigned long)proto_name);
	if (!hnode)
		TC_PANIC("Can't recognize proto type %s\n", proto_name);

	pnode = tc_list_entry(hnode, struct tc_transfer_proto_config_node, node);

	return pnode->proto;
}

static CONFIG_FUNC(TPROTO)
{
	int proto = 0;

	proto = tc_transfer_proto_type_get(val);
	*(int*)user_data = proto;
}

static int
tc_transfer_proto_config_set()
{
	TC_CONFIG_ADD("proto", global_proto_config.proto_name, FUNC_NAME(STR));
	//TC_CONFIG_ADD("device", &global_proto_config.dev_type, FUNC_NAME(DEV));
}


int
tc_transfer_proto_config_add(
	char *name,
	int  proto
)
{
	struct tc_transfer_proto_config_node *pnode = NULL;

	pnode = (struct tc_transfer_proto_config_node *)calloc(1, sizeof(*pnode));
	if (!pnode) 
		TC_PANIC("Can't calloc memory for %d bytes\n", sizeof(*pnode));
	if (name)
		pnode->proto_name = strdup(name);
	pnode->proto = proto;

	return tc_hash_add(global_proto_data.proto_config_hash, 
			   &pnode->node, (unsigned long)name);
}

int
tc_transfer_proto_add(
	struct tc_transfer_proto_oper *oper,
	int			      *proto_id
)
{
	struct tc_transfer_proto_node *pnode = NULL;	

	pnode = (struct tc_transfer_proto_node *)calloc(1, sizeof(*pnode));
	if (!pnode)
		TC_PANIC("Not enough memory for %d bytes\n", sizeof(*pnode));
	pthread_mutex_lock(&global_proto_data.mutex);
	pnode->link_type = global_proto_data.proto_count++;
	pthread_mutex_unlock(&global_proto_data.mutex);
	memcpy(&pnode->oper, oper, sizeof(*oper));
	(*proto_id) = pnode->link_type;

	return tc_hash_add(global_proto_data.proto_hash, &pnode->node, 0);
}

struct tc_transfer_proto_oper*
tc_transfer_proto_oper_get()
{
	struct hlist_node *hnode = NULL;
	struct tc_transfer_proto_config_node *cnode = NULL;
	struct tc_transfer_proto_node *pnode = NULL;

	hnode = tc_hash_get(global_proto_data.proto_config_hash, 
			    (unsigned long)global_proto_config.proto_name, 
			    (unsigned long)global_proto_config.proto_name);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGISTER_CMD);
		return NULL;
	}
	cnode = tc_list_entry(hnode, struct tc_transfer_proto_config_node, node);
	hnode = tc_hash_get(global_proto_data.proto_hash, 
			    cnode->proto, 
			    cnode->proto);
	if (!hnode)
		return NULL;
	pnode = tc_list_entry(hnode, struct tc_transfer_proto_node, node);

	return &pnode->oper;
}

struct tc_transfer_proto_oper*
tc_transfer_proto_oper_get_by_name(
	char *proto_name
)
{
	int proto_type = 0;
	struct hlist_node *hnode = NULL;
	struct tc_transfer_proto_node *pnode = NULL;

	proto_type = tc_transfer_proto_type_get(proto_name);

	hnode = tc_hash_get(global_proto_data.proto_hash, 
			    (unsigned long)proto_type, 
			    (unsigned long)proto_type);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGISTER_CMD);
		return NULL;
	}
	pnode = tc_list_entry(hnode, struct tc_transfer_proto_node, node);

	return &pnode->oper;
}

struct tc_transfer_proto_oper*
tc_transfer_proto_oper_get_by_type(
	int proto_type
)
{
	struct hlist_node *hnode = NULL;
	struct tc_transfer_proto_node *pnode = NULL;

	hnode = tc_hash_get(global_proto_data.proto_hash, 
			    (unsigned long)proto_type, 
			    (unsigned long)proto_type);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGISTER_CMD);
		return NULL;
	}
	pnode = tc_list_entry(hnode, struct tc_transfer_proto_node, node);

	return &pnode->oper;
}

static int
tc_transfer_proto_uninit()
{
	tc_hash_destroy(global_proto_data.proto_hash);
	return TC_OK;
}

static int
tc_transfer_proto_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data	
)
{
	int link_type = 0;
	struct tc_transfer_proto_node *proto_node = NULL;

	if (!hnode)
		link_type = (int)user_data;
	else {
		proto_node = tc_list_entry(hnode, struct tc_transfer_proto_node, node);
		link_type = proto_node->link_type;
	}

	return (link_type % TC_PROTO_HASH_SIZE);
}

static int
tc_transfer_proto_hash_get(
	struct hlist_node *hnode,
	unsigned long	  user_data
)
{
	struct tc_transfer_proto_node *pnode = NULL;

	if (!hnode)
		return TC_ERR;

	pnode = tc_list_entry(hnode, struct tc_transfer_proto_node, node);
	if ((int)user_data == pnode->link_type)
		return TC_OK;

	return TC_ERR;
}

static int
tc_transfer_proto_hash_destroy(
	struct hlist_node *hnode
)
{
	struct tc_transfer_proto_node *pnode = NULL;

	if (!hnode)
		return TC_ERR;

	pnode = tc_list_entry(hnode, struct tc_transfer_proto_node, node);
	TC_FREE(pnode);

	return TC_OK;
}

static int
tc_transfer_proto_config_hash(
	struct hlist_node *hnode,
	unsigned long     user_data
)
{
	char proto_name = 0;
	struct tc_transfer_proto_config_node *pnode = NULL;

	if (!hnode && !user_data) {
		return TC_ERR;
	}

	if (!hnode) 
		proto_name = ((char*)user_data)[0];
	else {
		pnode = tc_list_entry(hnode, struct tc_transfer_proto_config_node, node);
		if (pnode->proto_name)
			proto_name = pnode->proto_name[0];
	}

	return (proto_name % TC_PROTO_CONFIG_SIZE);
}

static int
tc_transfer_proto_config_hash_get(
	struct hlist_node *hnode,
	unsigned long     user_data
)
{
	struct tc_transfer_proto_config_node *pnode = NULL;

	if (!hnode)
		return TC_ERR;

	pnode = tc_list_entry(hnode, struct tc_transfer_proto_config_node, node);
	if (!pnode->proto_name && !user_data)
		return TC_OK;
	if (!pnode->proto_name || !user_data)
		return TC_ERR;
	if (strcmp(pnode->proto_name,(char*)user_data))
		return TC_ERR;

	return TC_OK;
}

static int
tc_transfer_proto_config_hash_destroy(
	struct hlist_node *hnode
)
{	
	struct tc_transfer_proto_config_node *pnode = NULL;

	if (!hnode)
		return TC_OK;

	pnode = tc_list_entry(hnode, struct tc_transfer_proto_config_node, node);
	TC_FREE(pnode->proto_name);
	TC_FREE(pnode);
}

int
tc_transfer_proto_init()
{
	int ret = 0;

	global_proto_data.proto_count = 1;
	pthread_mutex_init(&global_proto_data.mutex, NULL);
	global_proto_data.proto_hash = tc_hash_create(
						TC_PROTO_HASH_SIZE, 
						tc_transfer_proto_hash, 
						tc_transfer_proto_hash_get,
						tc_transfer_proto_hash_destroy);

	global_proto_data.proto_config_hash = tc_hash_create(
						TC_PROTO_CONFIG_SIZE, 
						tc_transfer_proto_config_hash, 
						tc_transfer_proto_config_hash_get, 
						tc_transfer_proto_hash_destroy);

	ret = tc_user_cmd_add(tc_transfer_proto_config_set);
	if (ret != TC_OK)
		TC_PANIC("add tc_transfer_proto_config_set error");

	return tc_uninit_register(tc_transfer_proto_uninit);
}

TC_MOD_INIT(tc_transfer_proto_init);
