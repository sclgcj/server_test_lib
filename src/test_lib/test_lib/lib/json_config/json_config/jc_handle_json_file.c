#include "jc_handle_private.h"
#include "jc_handle_json_file_private.h"

#define JC_HANDLE_FILE "file_json"

static int
jc_handle_json_file_init(
	struct jc_comm *jcc
)
{
	return JC_OK;
}

static int
jc_handle_json_file_execute(
	struct jc_comm *jcc
)
{
	cJSON *tmp = NULL, *obj = NULL;
	struct jc_handle_private *hp = NULL;

	if (jcc->retval)
		tmp = cJSON_CreateString(jcc->retval);
	else
		tmp = cJSON_CreateNull();
	hp = (typeof(hp))jcc->module_private;
	obj = (cJSON *)jcc->out_data;
	if (obj->type == cJSON_Object)
		cJSON_AddItemToObject(obj, hp->node_name, tmp);
	else
		cJSON_AddItemToArray(obj, tmp);


	return JC_OK;
}

int
json_config_handle_json_file_init()
{
	struct jc_handle_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.handle_init = jc_handle_json_file_init;
	oper.handle_execute = jc_handle_json_file_execute;

	return jc_handle_module_add(JC_HANDLE_FILE, &oper);
}

int
json_config_handle_json_file_uninit()
{
	return JC_OK;
}

