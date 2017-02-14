#include "jc_private.h"
#include "jc_result_private.h"

#define JC_RESULT "result"
#define JC_RESULT_LEVEL 200
#define JC_RESULT_TYPE_JSON "json"
#define JC_RESULT_TYPE_GET  "get"

struct jc_result {
	char *type;
};

static struct jc_result global_result;

static int
jc_result_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
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
jc_result_init_default(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	return jc_result_init(node_name, obj, user_data, jcc);
}

static int
jc_result_execute(
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc			
)
{
	if (!global_result.type)
		jcc->result = (JC_RESULT_TYPE_JSON);
	else
		jcc->result = (global_result.type);

	return JC_OK;
}

static int
jc_result_exe_default(
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	return jc_result_execute(node_name, user_data, jcc);
}

int
json_config_result_init()
{
	struct jc_oper oper;	

	memset(&oper, 0, sizeof(oper));
	oper.jc_init = jc_result_init;
	oper.jc_execute = jc_result_execute;
	oper.jc_execute_default = jc_result_exe_default;
	oper.jc_init_default = jc_result_init_default;

	return jc_module_add(JC_RESULT, JC_RESULT_LEVEL, &oper);
}

int
json_config_result_uninit()
{
	if (global_result.type)
		free(global_result.type);

	return JC_OK;
}

