#include "jc_private.h"
#include "jc_count_private.h"
#include "tc_hash.h"

#define JC_COUNT "count"
#define JC_COUNT_LEVEL  200
#define JC_COUNT_HASH_SIZE 26

struct jc_count_node {
	char *var;
	int count;
	struct hlist_node node;
};

struct jc_count {
	int count;
	tc_hash_handle_t count_hash;
};

static struct jc_count global_jc_count;

static int
jc_count_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	struct jc_count_node *jcn = NULL;

	jcn = (struct jc_count_node*)calloc(1, sizeof(*jcn));
	if (!jcn) {
		fprintf(stderr, "can't calloc %d bytes: %s\n",
				sizeof(*jcn), strerror(errno));
		exit(0);
	}
	if (node_name)
		jcn->var = strdup(node_name);
	if (obj->type == cJSON_String)
		jcn->count = atoi(obj->valuestring);
	else
		jcn->count = obj->valueint;
	jcc->count = jcn->count;

	return tc_hash_add(global_jc_count.count_hash, 
			   &jcn->node, 
			   (unsigned long)node_name);
}

static int
jc_count_execute(
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	struct hlist_node *hnode = NULL;
	struct jc_count_node *jcn = NULL;

	hnode = tc_hash_get(global_jc_count.count_hash, 
			  (unsigned long)node_name, 
			  (unsigned long)node_name);
	if (!hnode) {
		fprintf(stderr, "no variable named %s\n", node_name);
		return JC_ERR;
	}
	jcn = tc_list_entry(hnode, struct jc_count_node, node);
	jcc->count = jcn->count;

	return JC_OK;
}

static int
jc_count_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int name = 0;
	struct jc_count_node *jcn = NULL;	

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (hnode) {
		jcn = tc_list_entry(hnode, struct jc_count_node, node);
		if (jcn->var)
			name = jcn->var[0];
	}

	return (name % JC_COUNT_HASH_SIZE);
}

static int
jc_count_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_count_node *jcn = NULL;	

	jcn = tc_list_entry(hnode, struct jc_count_node, node);
	if (!user_data && !jcn->var)	
		return JC_OK;
	if (!user_data || !jcn->var)
		return JC_ERR;
	if (!strcmp(jcn->var, (char*)user_data))
		return JC_OK;
	return JC_ERR;
}

static int
jc_count_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_count_node *jcn = NULL;

	jcn = tc_list_entry(hnode, struct jc_count_node, node);
	if (jcn->var)
		free(jcn->var);

	free(jcn);

	return JC_OK;
}

int
json_config_count_init()
{
	struct jc_oper oper;	

	global_jc_count.count_hash = tc_hash_create(
						JC_COUNT_HASH_SIZE,
						jc_count_hash, 
						jc_count_hash_get,
						jc_count_hash_destroy);

	memset(&oper, 0, sizeof(oper));
	oper.jc_init = jc_count_init;
	oper.jc_execute = jc_count_execute;

	return jc_module_add(JC_COUNT, JC_COUNT_LEVEL, &oper);
}

int
json_config_count_uninit()
{
	if (global_jc_count.count_hash)
		tc_hash_destroy(global_jc_count.count_hash);

	return JC_OK;
}
