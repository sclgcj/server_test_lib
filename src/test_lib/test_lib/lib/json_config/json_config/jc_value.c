#include "jc_private.h"
#include "jc_value_private.h"

#define JC_VALUE "value"
#define JC_VALUE_LEVEL 300

static int
jc_value_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	if (obj->child) {
		fprintf(stderr, "no child in value node\n");
		return JC_ERR;
	}
	jcc->conf_val = cJSON_Duplicate(obj->child, 1);

	return JC_OK;
}

static int
jc_value_execute(
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	return JC_OK;
}

int 
json_config_value_uninit()
{
	struct jc_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.jc_execute = jc_value_execute;
	oper.jc_init = jc_value_init;

	return jc_module_add(JC_VALUE, JC_VALUE_LEVEL, &oper);
}

int
json_config_value_init()
{
	return JC_OK;
}
