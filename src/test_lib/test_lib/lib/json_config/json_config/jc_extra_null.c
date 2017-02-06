#include "json_config_extra_private.h"
#include "json_config_extra_null_private.h"

#define JC_EXTRA_NULL "null"

static int
jc_extra_null_init(
	struct json_config_comm *jcc
)
{
	return JC_OK;	
}

static int
jc_extra_null_execute(
	struct json_config_comm *jcc
)
{
	return JC_OK;
}


int
json_config_extra_null_uninit()
{
	struct jc_extra_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.extra_init    = jc_extra_null_init;
	oper.extra_execute = jc_extra_null_execute;

	return jc_extra_module_add(JC_EXTRA_NULL, &oper);
}

int
json_config_extra_null_init()
{
	return JC_OK;
}
