#include "jc_private.h"
#include "jc_type_private.h"
#include "jc_var_module_hash_private.h"

#define JC_TYPE "type"
#define JC_TYPE_LEVEL  500
#define JC_TYPE_HASH_SIZE 26

struct jc_type_module_node {
	struct jc_type_oper oper;
};

struct jc_type {
	jc_var_module_t vm_hash;
};

static struct jc_type global_type;

int
jc_type_module_add(
	char *module,
	struct jc_type_oper *oper
)
{
	struct jc_type_module_node *jtmn = NULL;

	jtmn = (struct jc_type_module_node *)calloc(1, sizeof(*jtmn));
	if (!jtmn) {
		fprintf(stderr, "can't calloc %d bytes: %s\n", 
				sizeof(*jtmn), strerror(errno));
		exit(0);
	}
	if (oper)
		memcpy(&jtmn->oper, oper, sizeof(*oper));

	return jc_var_module_add(
			module, 
			(unsigned long)&jtmn->oper, 
			global_type.vm_hash);
}

static int
jc_type_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct jc_type_private jtp;
	struct jc_type_module_node *jtmn = NULL;
	struct jc_var_module_param param;

	memset(&param, 0, sizeof(param));
	param.un.module = obj->valuestring;
	jtmn = (typeof(jtmn))jc_module_get(&param, global_type.vm_hash);
	if (jtmn->oper.jc_type_init) {
		memset(&jtp, 0, sizeof(jtp));
		jtp.node_name = (node_name);
		jtp.obj = obj;
		jtp.user_data = user_data;
		jtp.init_cb = jc_type_init;
		jcc->module_private = (unsigned long)&jtp;
		ret = jtmn->oper.jc_type_init(jcc);
	}
	return jc_var_add(jcc->depth, node_name, 
			  obj->valuestring, 0, global_type.vm_hash);
}

static int
jc_type_module_execute(
	char *node_name,
	char *module,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	int ret = JC_OK;
	struct jc_type_private jtp;
	struct jc_type_module_node *jtmn = NULL;
	struct jc_var_module_param param;

	memset(&param, 0, sizeof(param));
	param.un.var = node_name;
	param.user_data = user_data;
	jtmn = (typeof(jtmn))jc_module_get(&param, global_type.vm_hash);
	if ((unsigned long)jtmn == (unsigned long)JC_ERR)
		return JC_ERR;
	if (jtmn->oper.jc_type_execute) {
		memset(&jtp, 0, sizeof(jtp));
		jtp.node_name = (node_name);
		jtp.user_data = user_data;
		jtp.execute_cb = jc_type_module_execute;
		jcc->module_private = (unsigned long)&jtp;
		ret = jtmn->oper.jc_type_execute(jcc);
	}

	return ret;
}

static int
jc_type_execute(
	char *node_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	char *module = NULL;
	struct jc_type_var_node *jtvn = NULL;
	struct jc_type_module_node *mnode = NULL;
	struct jc_var_module_param param;

	memset(&param, 0, sizeof(param));
	if (node_name)
		param.un.var = (node_name);
	param.depth = jcc->depth;	
	param.user_data = user_data;
	module = jc_var_data_get(&param, global_type.vm_hash);
	if (!module)
		return JC_ERR;

	return jc_type_module_execute(node_name, module, user_data, jcc);
}

static int
jc_type_module_copy(
	unsigned long user_data,
	unsigned long copy_data
)
{
	struct jc_type_module_node *jtmn = NULL;

	jtmn = (typeof(jtmn))user_data;
	if (jtmn->oper.jc_type_copy)
		return jtmn->oper.jc_type_copy((unsigned int)copy_data);

	return JC_OK;
}

static int
jc_type_copy(
	unsigned int data_num	
)
{
	return jc_var_module_traversal(
				global_type.vm_hash, 
				data_num, 
				jc_type_module_copy);
}

static int
jc_type_module_hash_destroy(
	unsigned long mod_data
)
{
	free((void*)mod_data);
	return JC_OK;
}

int
json_config_type_init()
{
	struct jc_oper oper;

	global_type.vm_hash = jc_var_module_create(
					NULL,
					NULL, 
					NULL,
					jc_type_module_hash_destroy);
	if (!global_type.vm_hash)
		return JC_ERR;

	memset(&oper, 0, sizeof(oper));
	oper.jc_init = jc_type_init;
	oper.jc_execute = jc_type_execute;
	oper.jc_copy = jc_type_copy;

	return JC_OK;
}

int
json_config_type_uninit()
{
	if (global_type.vm_hash)
		return jc_var_module_destroy(global_type.vm_hash);

	return JC_OK;
}

