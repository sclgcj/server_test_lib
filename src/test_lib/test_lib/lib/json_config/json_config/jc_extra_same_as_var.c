#include "jc_extra_private.h"
#include "jc_extra_same_as_var_private.h"
#include "tc_hash.h"

#define JC_EXTRA_SAME_AS_VAR "same as val"
#define JC_EXTRA_SAME_AS_VAR_HASH_SIZE  26

struct jc_extra_same_as_value{
	char *other_module;
};

static struct jc_extra_same_as_value global_value;

static int
jc_extra_same_as_var_init(
	struct jc_comm *jcc
)
{
	char *tmp = NULL;
	struct jc_extra_private *jep = NULL;

	jep = (struct jc_extra_private *)jcc->module_private;
	if (global_value.other_module)
		free(global_value.other_module);
	tmp = strrchr(jep->obj->valuestring, ' ');
	tmp++;
	global_value.other_module = strdup(tmp);

	return JC_OK;
}

static int
jc_extra_same_as_var_execute(
	struct jc_comm *jcc
)
{
	struct jc_extra_private *jep = NULL;

	jep = (struct jc_extra_private *)jcc->module_private;
	if (global_value.other_module)
		return jep->other_extra_module(jep->var_name,
					       global_value.other_module, 
					       jep->user_data, 
					       jcc);

	return JC_OK;
}

int
json_config_extra_same_as_var_uninit()
{
	if (global_value.other_module)
		free(global_value.other_module);
	return JC_OK;
}

int
json_config_extra_same_as_var_init()
{
	struct jc_extra_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.extra_init    = jc_extra_same_as_var_init;
	oper.extra_execute = jc_extra_same_as_var_execute;
	return jc_extra_module_add(JC_EXTRA_SAME_AS_VAR, &oper);
}
