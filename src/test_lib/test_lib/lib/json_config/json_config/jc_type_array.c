#include "jc_type_private.h"
#include "jc_type_array_private.h"

#define JC_TYPE_ARRAY "array"

static int
jc_type_array_walk(
	struct jc_comm *jcc	
)
{
	cJSON *obj  = NULL; 
	cJSON *tmp = NULL;
	cJSON *child = NULL;
	struct jc_type_private *jtp = NULL;

	if (!jcc->conf_val)
		return JC_ERR;

	jtp = (struct jc_type_private *)jcc->module_private;
	jcc->out_data = jcc->walk_cb(jcc->id, jcc->depth + 1, 
				     jcc->out_data, jcc->conf_val);

	return JC_OK;
}

static int
jc_type_array_init(
	struct jc_comm *jcc
)
{
	jcc->type = JC_TYPE_ARRAY;
	jc_type_array_walk(jcc);
	return JC_OK;
}

static int
jc_type_array_execute(
	struct jc_comm *jcc
)
{
	int i = 0; 
	int ret = 0;

	jcc->type = JC_TYPE_ARRAY;
	jcc->out_data = (unsigned long)cJSON_CreateArray();
	for (; i < jcc->count; i++) {
		ret = jc_type_array_walk(jcc);
		if (ret != JC_OK) {
			cJSON_Delete((cJSON*)jcc->out_data);
			return JC_ERR;
		}
	}
	jcc->retval = (char*)jcc->out_data;

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

	return jc_type_module_add(JC_TYPE_ARRAY, &oper);
}

