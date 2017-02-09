#include "jc_handle_private.h"
#include "jc_handle_json_private.h"

#define JC_HANDLE_OBJECT "object_json"

static int
jc_handle_json_init(
	struct jc_comm *jcc
)
{
	return JC_OK;
}

static int
jc_handle_json_execute(
	struct jc_comm *jcc
)
{
	cJSON *obj = NULL;
	struct jc_handle_private *hp = NULL;

	hp = (typeof(hp))jcc->module_private;
	obj = (cJSON *)jcc->out_data;
	cJSON_AddItemToObject(obj, hp->node_name, 
			      (cJSON*)jcc->retval);

	return JC_OK;
}

int
json_config_handle_json_init()
{
	struct jc_handle_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.handle_init = jc_handle_json_init;
	oper.handle_execute = jc_handle_json_execute;

	return jc_handle_module_add(JC_HANDLE_OBJECT, &oper);
}

int
json_config_handle_json_uninit()
{
	return JC_OK;
}

