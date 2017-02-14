#include "jc_handle_private.h"
#include "jc_handle_json_array_private.h"

#define JC_HANDLE_ARRAY "array_json"

static int
jc_handle_array_init(
	struct jc_comm *jcc
)
{
	return JC_OK;
}

static int
jc_handle_array_execute(
	struct jc_comm *jcc
)
{
	cJSON *obj = NULL;
	struct jc_handle_private *hp = NULL;

	hp = (typeof(hp))jcc->module_private;
	obj = (cJSON *)jcc->out_data;
	if (hp->node_name) {
		cJSON_AddItemToObject(obj, hp->node_name, 
					      (cJSON*)jcc->retval);
	} else {
		if (obj->type == cJSON_Array)
			cJSON_AddItemToArray(obj, (cJSON*)jcc->retval);
	}

	return JC_OK;
}

int
json_config_handle_json_array_init()
{
	struct jc_handle_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.handle_init = jc_handle_array_init;
	oper.handle_execute = jc_handle_array_execute;

	return jc_handle_module_add(JC_HANDLE_ARRAY, &oper);
}

int
json_config_handle_json_array_uninit()
{
	return JC_OK;
}

