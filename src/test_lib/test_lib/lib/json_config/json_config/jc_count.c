#include "jc_private.h"
#include "jc_count_private.h"
#include "jc_var_hash_private.h"
#include "tc_hash.h"

#define JC_COUNT "count"
#define JC_COUNT_LEVEL  200
#define JC_COUNT_HASH_SIZE 26

struct jc_count_node {
	int count;
};
struct jc_count {
	jc_variable_t vh;	
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
	if (obj->type == cJSON_String)
		jcn->count = atoi(obj->valuestring);
	else
		jcn->count = obj->valueint;
	jcc->count = jcn->count;

	return jc_variable_add(node_name, jcc->depth, 
			      (unsigned long)jcn, 
			      global_jc_count.vh);
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

	jcn = (typeof(jcn))jc_variable_get(node_name, 
					   jcc->depth, 
					   0, 
					   global_jc_count.vh);
	if (!jcn)
		return JC_ERR;
	jcc->count = jcn->count;

	return JC_OK;
}

static int
jc_count_hash_destroy(
	unsigned long data
)
{
	free((void*)data);
	return JC_OK;
}

int
json_config_count_init()
{
	struct jc_oper oper;	

	global_jc_count.vh = jc_variable_create(NULL, jc_count_hash_destroy);
	memset(&oper, 0, sizeof(oper));
	oper.jc_init = jc_count_init;
	oper.jc_execute = jc_count_execute;

	return jc_module_add(JC_COUNT, JC_COUNT_LEVEL, &oper);
}

int
json_config_count_uninit()
{
	if (global_jc_count.vh)
		jc_variable_destroy(global_jc_count.vh);
	return JC_OK;
}

