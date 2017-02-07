#include "jc_mode_private.h"
#include "jc_mode_unique_private.h"

#define JC_MODE_UNIQUE  "unique"

static int
jc_mode_unique_init(
	struct jc_comm *jcc
)
{
	if (jcc->mode_type)
		free(jcc->mode_type);
	jcc->mode_type = strdup(JC_MODE_UNIQUE);

	return JC_OK;	
}

static int
jc_mode_unique_execute(
	struct jc_comm *jcc
)
{
	if (jcc->mode_type)
		free(jcc->mode_type);
	jcc->mode_type = strdup(JC_MODE_UNIQUE);
	
	return JC_OK;
}

int
json_config_mode_unique_init()
{
	struct json_mode_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.json_mode_init = jc_mode_unique_init;
	oper.json_mode_execute = jc_mode_unique_execute;

	return json_mode_module_add("JC_MODE_UNIQUE", &oper);
}

int
json_config_mode_unique_uninit()
{
	return JC_OK;
}

