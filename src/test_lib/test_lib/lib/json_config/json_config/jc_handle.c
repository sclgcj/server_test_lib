#include "jc_private.h"
#include "jc_letter_hash_private.h"
#include "jc_handle_private.h"

#define JC_HANDLE "handle"
#define JC_HANDLE_LEVEL 600

struct jc_handle_module {
	struct jc_handle_oper oper;
};

struct jc_handle {
	jc_letter_t module;
};

static struct jc_handle global_handle;

int
jc_handle_module_add(
	char *module,
	struct jc_handle_oper *oper
)
{
	struct jc_handle_module *hm = NULL;

	hm = (typeof(hm))calloc(1, sizeof(*hm));
	if (!hm) {
		fprintf(stderr, "calloc %d bytes error: %s\n",
				sizeof(*hm), strerror(errno));
		exit(0);
	}
	if (oper)
		memcpy(&hm->oper, oper, sizeof(*oper));

	return jc_letter_add(module, 
			     (unsigned long)hm,
			     global_handle.module);
}

static int
jc_handle_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	struct jc_letter_param param;
	struct jc_handle_module *hm = NULL;
	struct jc_handle_private hp;

	memset(&param, 0, sizeof(param));
	param.name = jcc->result;
	hm = (typeof(hm))jc_letter_get(&param, global_handle.module);
	if (!hm)
		return JC_ERR;
	memset(&hp, 0, sizeof(hp));
	hp.node_name = node_name;
	jcc->module_private = (unsigned long)&hp;
	if (hm->oper.handle_init)
		return hm->oper.handle_init(jcc);

	return JC_OK;
}

static int
jc_handle_execute(
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	char name[128] = { 0 };
	struct jc_letter_param	param;
	struct jc_handle_module *hm = NULL;
	struct jc_handle_private hp;

	memset(&param, 0, sizeof(param));
	sprintf(name, "%s_%s", jcc->type, jcc->result);
	param.name = name;
	hm = (typeof(hm))jc_letter_get(&param, global_handle.module);
	if (!hm)
		return JC_ERR;
	memset(&hp, 0, sizeof(hp));
	hp.node_name = node_name;
	if (hm->oper.handle_execute)
		return hm->oper.handle_execute(jcc);

	return JC_OK;
}

static int
jc_handle_module_destroy(
	unsigned long data
)
{
	free((void*)data);
	return JC_OK;
}

int
json_config_handle_init()
{
	struct jc_oper oper;

	global_handle.module = jc_letter_create(NULL, NULL, 
						jc_handle_module_destroy);

	memset(&oper, 0, sizeof(oper));
	oper.jc_execute = jc_handle_execute;
	oper.jc_init = jc_handle_init;

	return jc_module_add(JC_HANDLE, JC_HANDLE_LEVEL, &oper);
}

int
json_config_handle_uninit()
{
	if (global_handle.module)
		return jc_letter_destroy(global_handle.module);

	return JC_OK;
}


