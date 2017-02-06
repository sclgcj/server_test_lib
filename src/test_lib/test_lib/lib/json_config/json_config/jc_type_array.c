#include "json_config_type_private.h"
#include "json_config_type_array_private.h"

#define JC_TYPE_ARRAY "array"

static cJSON *
jc_type_array_init_walk(
	struct json_config_comm *jcc	
)
{
	cJSON *obj  = NULL; 
	cJSON *tmp = NULL;
	cJSON *child = NULL;
	struct jc_type_private *jtp = NULL;

	if (!jcc->conf_val)
		return NULL;

	jtp = (struct jc_type_private *)jcc->module_private;
	obj = cJSON_CreateObject();
	child = jcc->conf_val->child;
	while (child) {
		tmp = jcc->walk_cb(jcc->id, jcc->depth + 1, jtp->user_data, child);
		if (tmp) 
			cJSON_AddItemToObject(obj, child->string, tmp);
		child = child->next;
	}

	return obj;
}

static int
jc_type_array_init(
	struct json_config_comm *jcc
)
{
	int i = 0;
	cJSON *tmp = NULL;
	cJSON *array = NULL;

	array = cJSON_CreateArray();

	tmp = jc_type_array_init_walk(jcc);
	if (!tmp)
		goto out;
		
	for (i; i < jcc->count; i++)
		cJSON_AddItemToArray(array, tmp);

out:
	jcc->out_data = (unsigned long)array;
	return JC_OK;
}

static int
jc_type_array_execute(
	struct json_config_comm *jcc
)
{
	jcc->out_data = (unsigned long)cJSON_CreateArray();
	return JC_OK;
}

int
json_config_type_array_uninit()
{
	return JC_OK;
}

int
json_config_type_array_init()
{
	struct jc_type_oper oper;

	oper.jc_type_init = jc_type_array_init;
	oper.jc_type_execute = jc_type_array_execute;

	return json_config_type_module_add(JC_TYPE_ARRAY, &oper);
}
