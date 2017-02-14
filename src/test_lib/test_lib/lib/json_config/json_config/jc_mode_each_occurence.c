#include "jc_mode_private.h"
#include "jc_mode_each_occurence_private.h"

#define JC_MODE_EACH_OCCURENCE  "each occurence"

static int
jc_mode_each_occurence_init(
	struct jc_comm *jcc
)
{
	if (jcc->mode_type)
		free(jcc->mode_type);
	jcc->mode_type = strdup(JC_MODE_EACH_OCCURENCE);

	return JC_OK;
}

static int
jc_mode_each_occurence_execute(
	struct jc_comm *jcc
)
{
	if (jcc->mode_type)
		free(jcc->mode_type);
	jcc->mode_type = strdup(JC_MODE_EACH_OCCURENCE);
	jcc->getvalue  = JC_GET_VALUE;
	
	return JC_OK;
}

int
json_config_mode_each_occurence_init()
{
	struct json_mode_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.json_mode_init = jc_mode_each_occurence_init;
	oper.json_mode_execute = jc_mode_each_occurence_execute;

	return json_mode_module_add(JC_MODE_EACH_OCCURENCE, 1, &oper);
}

int
json_config_mode_each_occurence_uninit()
{
	return JC_OK;
}
