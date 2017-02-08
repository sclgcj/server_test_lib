#include "jc_private.h"
#include "jc_mode_private.h"
#include "jc_var_module_hash_private.h"

#define JC_MODE_HASH_SIZE	26
#define JC_MODE			"mode"	
#define JC_MODE_LEVEL		100

struct json_mode_module {
	struct json_mode_oper oper;
};

struct json_mode {
	jc_var_module_t vm;
};

static struct json_mode global_mode;

int
json_mode_module_add(
	char *name, 
	struct json_mode_oper *oper
)
{
	struct json_mode_module *jmm = NULL;

	jmm = (struct json_mode_module*)calloc(1, sizeof(*jmm));
	if (!jmm) {
		fprintf(stderr, "calloc %d bytes error: %s\n", sizeof(*jmm), strerror(errno));
		exit(0);
	}
	if (oper)
		memcpy(&jmm->oper, oper, sizeof(*oper));

	return jc_var_module_add(name, (unsigned long)jmm, global_mode.vm);
}

static int
json_mode_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;
	struct json_mode_private jmp;
	struct json_mode_module *jmm = NULL;
	struct jc_var_module_param param;

	memset(&param, 0, sizeof(param));
	param.un.module = obj->valuestring;
	jmm = (typeof(jmm))jc_module_get(&param, global_mode.vm);
	memset(&jmp, 0, sizeof(jmp));
	jmp.obj = obj;
	jmp.node_name = (node_name);
	jcc->module_private = (unsigned long)&jmp;
	if (jmm->oper.json_mode_init)
		ret = jmm->oper.json_mode_init(jcc);
	if (ret == JC_OK)
		ret = jc_var_add(jcc->depth, node_name, 
				 obj->valuestring, 0, 
				 global_mode.vm);

	return ret;
}

static void
json_mode_private_free(
	struct json_mode_private *jmp
)
{
	if (jmp->node_name) {
		free(jmp->node_name);
		jmp->node_name = NULL;
	}	
}

static int 
json_mode_module_execute(
	char *module,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;
	struct json_mode_module *jmm = NULL;
	struct json_mode_private jmp;
	struct jc_var_module_param param;

	memset(&param, 0, sizeof(param));
	param.un.module = module;
	jmm = (typeof(jmm))jc_module_get(&param, global_mode.vm);
	memset(&jmp, 0, sizeof(jmp));
	jmp.other_mode_judge = json_mode_module_execute;
	jmp.data = user_data;
	jcc->module_private = (unsigned long)&jmp;
	if (jmm->oper.json_mode_execute)
		ret = jmm->oper.json_mode_execute(jcc);

	json_mode_private_free(&jmp);

	return ret;
}

static int
json_mode_execute(
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	int ret = 0;
	char *module = NULL;
	struct json_mode_var *jmv = NULL;
	struct jc_var_module_param param;

	memset(&param, 0, sizeof(param));
	param.un.var = node_name;
	param.depth = jcc->depth;
	param.user_data = user_data;
	module = jc_var_data_get(&param, global_mode.vm);
	if (!module)
		return JC_ERR;

	return json_mode_module_execute(module, user_data, jcc);
}

static int
jc_mode_hash_destroy(
	unsigned long user_data
)
{
	free((void*)user_data);
	return JC_OK;
}

int
jc_mode_init()
{
	struct jc_oper oper;

	global_mode.vm = jc_var_module_create(
					NULL, NULL, 
					NULL, jc_mode_hash_destroy);

	memset(&oper, 0, sizeof(oper));
	oper.jc_init = json_mode_init;
	oper.jc_execute = json_mode_execute;
	return jc_module_add(JC_MODE, JC_MODE_LEVEL, &oper);

	//json_mode_each_iteration_init();
	//json_mode_each_occurence_init();
	//json_mode_each_once_init();
	//json_mode_unique_init();
}

int
jc_mode_uninit()
{
	if (global_mode.vm) 
		jc_var_module_destroy(global_mode.vm);

	return JC_OK;
}
