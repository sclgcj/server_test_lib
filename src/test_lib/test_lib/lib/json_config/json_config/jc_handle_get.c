#include "jc_handle_private.h"
#include "jc_handle_get_private.h"

#define JC_HANDLE_GET "get"

static int
jc_handle_get_init(
	struct jc_comm *jcc
)
{
	return JC_OK;
}

static int
jc_handle_get_execute(
	struct jc_comm *jcc
)
{	
	return JC_OK;
}

int
json_config_handle_get_init()
{
	struct jc_handle_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.handle_init = jc_handle_get_init;
	oper.handle_execute = jc_handle_get_execute;
	
	return jc_handle_module_add(JC_HANDLE_GET, &oper);
}

int
json_config_handle_get_uninit()
{
	return JC_OK;
}


