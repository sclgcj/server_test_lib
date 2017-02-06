#include "jc_mode_private.h"
#include "jc_mode_null_private.h"

#define JC_NULL "null"

static int
jc_mode_null_init(
	struct jc_comm *jcc
)
{
	return JC_OK;
}

static int
jc_mode_null_execute(
	struct jc_comm *jcc
)
{
	jcc->getvalue = JC_IGNORE_VALUE;
	return JC_OK;
}

int
json_config_mode_null_uninit()
{
	return JC_OK;
}

int
json_config_mode_null_init()
{
	struct json_mode_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.json_mode_execute = jc_mode_null_execute;
	oper.json_mode_init = jc_mode_null_init;
	return json_mode_module_add(JC_NULL, &oper);
}
