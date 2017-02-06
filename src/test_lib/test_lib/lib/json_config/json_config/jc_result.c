#include "json_config_private.h"
#include "json_config_result_private.h"

#define JC_RESULT "result"
#define JC_RESULT_LEVEL 200
#define JC_RESULT_TYPE_JSON "json"
#define JC_RESULT_TYPE_GET  "get"

struct json_config_result {
	char *type;
};

static struct json_config_result global_result;

static int
jc_result_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct json_config_comm *jcc
)
{
	if (global_result.type)
		return JC_OK;

	if (!obj->valuestring)
		global_result.type = strdup(JC_RESULT_TYPE_JSON);
	else 
		global_result.type = strdup(obj->valuestring);

	return JC_OK;
}

static int
jc_result_execute(
	char *node_name,
	unsigned long user_data,
	struct json_config_comm *jcc			
)
{
	if (!global_result.type)
		jcc->result = strdup(JC_RESULT_TYPE_JSON);
	else
		jcc->result = strdup(global_result.type);

	return JC_OK;
}

int
json_config_result_init()
{
	struct json_config_oper oper;	

	memset(&oper, 0, sizeof(oper));
	oper.jc_init = jc_result_init;
	oper.jc_execute = jc_result_execute;

	return json_config_module_add(JC_RESULT, JC_RESULT_LEVEL, &oper);
}

int
json_config_result_uninit()
{
	if (global_result.type)
		free(global_result.type);

	return JC_OK;
}

