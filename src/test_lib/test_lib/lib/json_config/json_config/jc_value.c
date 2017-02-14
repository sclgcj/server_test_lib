#include "jc_private.h"
#include "jc_value_private.h"
#include "jc_var_hash_private.h"

#define JC_VALUE "value"
#define JC_VALUE_LEVEL 300

struct jc_value_node {
	cJSON *data;
};

struct jc_value {
	jc_variable_t var;
};

static struct jc_value global_val;

static int
jc_value_node_destroy(
	unsigned long user_data
)
{
	struct jc_value_node *vnode = NULL;

	vnode = (typeof(vnode))user_data;
	if (vnode->data)
		cJSON_Delete(vnode->data);

	return JC_OK;
}

static cJSON *
jc_value_get(
	cJSON *obj,
	struct jc_comm *jcc
)
{
	switch (obj->type) {
		case cJSON_Object:
			return cJSON_Duplicate(obj->child, 1);
		case cJSON_String:
			return cJSON_CreateString(obj->valuestring);
		default:
			return cJSON_CreateNumber(obj->valueint);
	}
}

static int
jc_value_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	cJSON *tmp = NULL;
	struct jc_value_node *vnode = NULL;

	vnode = (typeof(vnode))calloc(1, sizeof(*vnode));
	if (!vnode) {
		fprintf(stderr, "calloc %d bytes error: %s\n", 
				sizeof(*vnode), strerror(errno));
		return JC_ERR;
	}
	vnode->data = jc_value_get(obj, jcc);
	jcc->conf_val = vnode->data;

	return jc_variable_add(node_name, jcc->depth, 
			       (unsigned long)vnode, 
			       global_val.var);
}

static int
jc_value_execute( 
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	struct jc_value_node *vnode = NULL;

	vnode = (typeof(vnode))jc_variable_get(node_name, 
					       jcc->depth, 
					       0, 
					       global_val.var);
	if (!vnode)
		return JC_ERR;
	jcc->conf_val = vnode->data;

	return JC_OK;
}

int 
json_config_value_uninit()
{
	if (global_val.var)
		return jc_variable_destroy(global_val.var);
	return JC_OK;
}

int
json_config_value_init()
{
	struct jc_oper oper;

	global_val.var = jc_variable_create(NULL, NULL);
	if (!global_val.var)
		return JC_ERR;

	memset(&oper, 0, sizeof(oper));
	oper.jc_execute = jc_value_execute;
	oper.jc_init = jc_value_init;

	return jc_module_add(JC_VALUE, JC_VALUE_LEVEL, &oper);
}

