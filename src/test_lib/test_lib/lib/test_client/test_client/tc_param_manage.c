#include "tc_param_manage_private.h"
#include "tc_init.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_print.h"
#include "tc_create_private.h"

/*
 * Here we ignore the thread-safe at first.
 * We may add it later, but we are not sure.
 */

#define TC_PARAM_MANAGE_HASH_SIZE 26
struct tc_param_manage_type_node {
	char *param_type;
	struct tc_param_oper oper;
	struct hlist_node node;
};

struct tc_param_manage_node {
	char *param_name;
	struct tc_param_oper *oper;
	struct tc_param *param;
	struct list_head  lnode;
	struct hlist_node node;
};

struct tc_param_manage_data {
	int param_num;
	struct list_head lhead;
	pthread_mutex_t mutex;
	tc_hash_handle_t param_hash;  //param name map hash
	tc_hash_handle_t param_type_hash; //param type hash
};

static struct tc_param_manage_data global_param_manage;

static struct tc_param_manage_type_node *
tc_param_manage_type_node_create(
	char *param_type,
	struct tc_param_oper *oper
)
{
	struct tc_param_manage_type_node *tnode = NULL;
	
	tnode = (struct tc_param_manage_type_node *)calloc(1, sizeof(*tnode));
	if (!tnode)
		TC_PANIC("Not enough memory for %d bytes\n", sizeof(*tnode));

	tnode->param_type = strdup(param_type);
	memcpy(&tnode->oper, oper, sizeof(*oper));

	return tnode;
}

static void
tc_param_manage_type_node_destroy(
	struct tc_param_manage_type_node *tnode
)
{
	TC_FREE(tnode->param_type);
}

int
tc_param_type_add(
	char *param_type,
	struct tc_param_oper *oper
)
{
	struct tc_param_manage_type_node *tnode = NULL;

	if (!oper || !param_type) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	if (!oper->param_copy || !oper->param_destroy || 
	    !oper->param_value_get || !oper->param_oper ||
	    !oper->param_set) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	tnode = tc_param_manage_type_node_create(param_type, oper);

	return tc_hash_add(
			global_param_manage.param_type_hash, 
			&tnode->node, 0);
}

static struct tc_param_oper *
tc_param_manage_type_oper_get(
	char *param_type
)
{
	struct hlist_node *hnode = NULL;
	struct tc_param_manage_type_node *tnode = NULL;

	hnode = tc_hash_get(global_param_manage.param_type_hash, 
			   (unsigned long)param_type, 
			   (unsigned long)param_type);
	if (!hnode) {
		TC_ERRNO_SET(TC_NO_SUCH_PARAM_TYPE);
		return NULL;
	}

	tnode = tc_list_entry(hnode, struct tc_param_manage_type_node, node);
	return &tnode->oper;
}

static struct tc_param_manage_node *
tc_param_manage_node_create(
	char *param_name,
	char *param_type,
	struct tc_param *param
)
{
	struct tc_param_manage_node *pm_node = NULL;

	pm_node = (struct tc_param_manage_node*)calloc(1, sizeof(*pm_node));
	if (!pm_node)
		TC_PANIC("Not enough memory for %d bytes\n", sizeof(*pm_node));

	pm_node->param_name = strdup(param_name);
	pm_node->oper = tc_param_manage_type_oper_get(param_type);
	if (!pm_node) 
		goto err;

	pm_node->param = pm_node->oper->param_copy(param);

	return pm_node;
err:
	TC_FREE(pm_node->param_name);
	TC_FREE(pm_node);
	return NULL;
}

static void
tc_param_manage_node_destroy(
	struct tc_param_manage_node *pm_node
)
{
	TC_FREE(pm_node->param_name);
	if (pm_node->oper && pm_node->oper->param_destroy)
		pm_node->oper->param_destroy(pm_node->param);
}

int
tc_param_add(
	char *param_name,
	char *param_type,
	struct tc_param *param
)
{
	struct tc_param_manage_node *pm_node = NULL;

	pm_node = tc_param_manage_node_create(param_name, param_type, param);
	if (!pm_node)
		return TC_ERR;

	pthread_mutex_lock(&global_param_manage.mutex);
	list_add_tail(&pm_node->lnode, &global_param_manage.lhead);
	global_param_manage.param_num++;
	pthread_mutex_unlock(&global_param_manage.mutex);

	return tc_hash_add(global_param_manage.param_hash,
			   &pm_node->node, 0);
}

static struct tc_param_manage_node *
tc_param_manage_node_get(
	char *param_name
)
{
	struct hlist_node *hnode = NULL;	
	struct tc_param_manage_node *pm_node = NULL;

	hnode = tc_hash_get(global_param_manage.param_hash, 
			    (unsigned long)param_name, 
			    (unsigned long)param_name);
	if (!hnode) {
		TC_ERRNO_SET(TC_NO_SUCH_PARAMETER_SET);
		return NULL;
	}
	
	pm_node = tc_list_entry(hnode, struct tc_param_manage_node, node);

	return pm_node;
}

int
tc_param_set(
	char *param_name,
	struct tc_param *param
)
{
	struct tc_param_manage_node *pm_node = NULL;

	pm_node = tc_param_manage_node_get(param_name);
	if (!pm_node)
		return TC_ERR;

	if (pm_node->oper && pm_node->oper->param_set) 
		pm_node->oper->param_set(param, pm_node->param);

	return TC_OK;
}

int
tc_param_del(
	char *param_name
)
{
	struct hlist_node *hnode = NULL;

	hnode = tc_hash_get(global_param_manage.param_hash,
			    (unsigned long)param_name, 
			    (unsigned long)param_name);
	if (!hnode) {
		TC_ERRNO_SET(TC_NO_SUCH_PARAMETER_SET);
		return TC_ERR;
	}	
	return tc_hash_del_and_destroy(global_param_manage.param_hash, 
				hnode,
				(unsigned long)param_name);
}

struct tc_param *
tc_param_config_get(
	char *param_name
)
{
	struct tc_param_manage_node *pm_node = NULL;

	pm_node = tc_param_manage_node_get(param_name);
	if (!pm_node)
		return NULL;
	
	return pm_node->oper->param_copy(pm_node->param);
}

char *
tc_param_value_get(
	char *param_name,
	unsigned long user_data
)
{
	struct tc_create_link_data *data = NULL;
	struct tc_param_manage_node *pm_node = NULL;

	pm_node = tc_param_manage_node_get(param_name);
	if (!pm_node)
		return NULL;

	data = tc_list_entry((char*)user_data, struct tc_create_link_data, data);

	return pm_node->oper->param_value_get(data->private_link_data.link_id,
					      pm_node->param);
}

int
tc_param_oper(
	int oper_cmd,
	char *param_name
)
{
	struct tc_param_manage_node *pm_node = NULL;

	pm_node = tc_param_manage_node_get(param_name);
	if (!pm_node)
		return TC_ERR;

	return pm_node->oper->param_oper(oper_cmd, pm_node->param);
}

int
tc_param_list_get(
	struct tc_param_list *plist
)
{
	int i = 0;
	struct tc_param_manage_node *pm_node = NULL;

	if (!plist) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	pthread_mutex_lock(&global_param_manage.mutex);
	plist->param_num = global_param_manage.param_num;
	plist->param_name = (char**)calloc(plist->param_num, sizeof(char*));
	if (!plist->param_name) 
		TC_PANIC("Not enough memory for %d bytes\n", 
					plist->param_num * sizeof(char*));
	list_for_each_entry(pm_node, (&global_param_manage.lhead), lnode) {
		plist->param_name[i] = strdup(pm_node->param_name);
		i++;
	}
	pthread_mutex_unlock(&global_param_manage.mutex);

	return TC_OK;
}

void
tc_param_list_free(
	struct tc_param_list *plist
)
{
	int i = 0;

	if (!plist || !plist->param_name || plist->param_num <= 0)
		return;

	for (; i < plist->param_num; i++) {
		TC_FREE(plist->param_name[i]);
	}
	TC_FREE(plist->param_name);
}

static int
tc_param_manage_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct tc_param_manage_node *pm_node = NULL;

	if (!hnode && !user_data)
		return TC_ERR;

	if (!hnode)
		name = ((char*)user_data)[0];
	else {
		pm_node = tc_list_entry(hnode, struct tc_param_manage_node, node);
		if (pm_node->param_name)
			name = pm_node->param_name[0];
	}

	return (name % TC_PARAM_MANAGE_HASH_SIZE);
}

static int
tc_param_manage_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct tc_param_manage_node *pm_node = NULL;

	pm_node = tc_list_entry(hnode, struct tc_param_manage_node, node);
	if (!user_data && !pm_node->param_name)
		return TC_OK;
	if (!user_data || !pm_node->param_name)
		return TC_ERR;
	if (!strcmp((char*)user_data, pm_node->param_name)) 
		return TC_OK;

	return TC_ERR;
}

static int
tc_param_manage_hash_destroy(
	struct hlist_node *hnode
)
{
	struct tc_param_manage_node *pm_node = NULL;

	pm_node = tc_list_entry(hnode, struct tc_param_manage_node, node);
	TC_FREE(pm_node->param_name);
	if (pm_node->oper->param_destroy && pm_node->param)
		pm_node->oper->param_destroy(pm_node->param);
	TC_FREE(pm_node);

	return TC_OK;
}

static int
tc_param_manage_type_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct tc_param_manage_type_node *tnode = NULL;

	if (!hnode && !user_data) 
		return TC_ERR;

	if (!hnode) 
		name = ((char*)user_data)[0];
	else {
		tnode = tc_list_entry(hnode, struct tc_param_manage_type_node, node);
		name = tnode->param_type[0];
	}

	return (name % TC_PARAM_MANAGE_HASH_SIZE);
}

static int
tc_param_manage_type_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct tc_param_manage_type_node *tnode = NULL;

	if (!user_data || !hnode)
		return TC_ERR;

	tnode = tc_list_entry(hnode, struct tc_param_manage_type_node, node);
	if (!tnode->param_type)
		return TC_ERR;
	if (!strcmp((char*)user_data, tnode->param_type))
		return TC_OK;

	return TC_ERR;
}

static int
tc_param_manage_type_hash_destroy(
	struct hlist_node *hnode
)
{
	struct tc_param_manage_type_node *tnode = NULL;

	tnode = tc_list_entry(hnode, struct tc_param_manage_type_node, node);
	TC_FREE(tnode->param_type);
	TC_FREE(tnode);

	return TC_OK;
}

static int
tc_param_manage_uninit()
{
	tc_hash_destroy(global_param_manage.param_hash);
	tc_hash_destroy(global_param_manage.param_type_hash);

	return TC_OK;
}

int
tc_param_manage_init()
{
	memset(&global_param_manage, 0, sizeof(global_param_manage));
	pthread_mutex_init(&global_param_manage.mutex, NULL);
	INIT_LIST_HEAD(&global_param_manage.lhead);
	global_param_manage.param_hash = tc_hash_create(
					       TC_PARAM_MANAGE_HASH_SIZE,
						tc_param_manage_hash, 
						tc_param_manage_hash_get,
						tc_param_manage_hash_destroy);
	if (global_param_manage.param_hash == TC_HASH_ERR)
		return TC_ERR;
	global_param_manage.param_type_hash = tc_hash_create(
						TC_PARAM_MANAGE_HASH_SIZE,
						tc_param_manage_type_hash,
						tc_param_manage_type_hash_get, 
						tc_param_manage_type_hash_destroy);
	if (global_param_manage.param_hash == TC_HASH_ERR)
		return TC_ERR;

	return tc_param_manage_uninit(tc_param_manage_uninit);
}

TC_MOD_INIT(tc_param_manage_init);
