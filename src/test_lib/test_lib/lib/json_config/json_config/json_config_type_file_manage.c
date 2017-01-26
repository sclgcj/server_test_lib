#include "json_config_comm_func_private.h"
#include "json_config_type_private.h"
#include "json_config_type_file_private.h"
#include "json_config_type_file_manage_private.h"

#define JC_TYPE_FILE_MODULE_EXTRA 10

//static struct jc_type_file_manage  global_manage;

struct jc_type_file_manage_node {
	struct jc_type_file_manage_oper oper;	
};

static int
jc_type_file_manage_node_destroy(
	unsigned long user_data
)
{
	if (user_data)
		free((void*)user_data);

	return JC_OK;
}

static int
jc_type_file_manage_module_add(
	char *module,
	struct jc_type_file_manage_oper *oper,
	struct jc_type_file_manage *man
)
{
	struct jc_type_file_manage_node *fmn = NULL;

	fmn = (typeof(fmn))calloc(1, sizeof(*fmn));
	if (!fmn) {
		fprintf(stderr, "can't calloc %d bytes: %s\n", 
				sizeof(*fmn), strerror(errno));
		exit(0);
	}
	if (oper)
		memcpy(&fmn->oper, 0, sizeof(fmn->oper));

	return jc_module_add(module, (unsigned long)oper, man->jvm);
}

static int
jc_type_file_manage_init(
	struct jc_type_file_manage *man,
	struct json_config_comm *jcc
)
{
	int ret = 0;
	int len = 0;
	char *module = NULL;
	struct jc_type_private *jtp = NULL;
	struct jc_var_module_param param;
	struct jc_type_file_private *jtfp = NULL;
	struct jc_type_file_manage_oper *oper = NULL;
	
	jtp = (typeof(jtp))jcc->module_private;
	jtfp = (typeof(jtfp))jtp->sub_module_private;
	len = strlen(jtfp->comm->end_action) + strlen(jtfp->comm->module) + 
		JC_TYPE_FILE_MODULE_EXTRA;
	module = (char*)calloc(1, len + 1);
	if (!module) {
		fprintf(stderr, "calloc %d bytes error : %s\n",
				len + 1, strerror(errno));
		exit(0);
	}
	sprintf(module, "%s_%s", jtfp->comm->end_action, jtfp->comm->module);
	memset(&param, 0, sizeof(param));
	param.un.module = module;
	oper = (typeof(oper))jc_module_get(&param, &man->jvm);
	if (!oper) 
		return JC_ERR;

	if (oper->manage_init)
		ret = oper->manage_init(jcc);

	return jc_var_add(jcc->depth, jtp->node_name, module, man->jvm);
}

static int
jc_type_file_manage_copy_walk(
	unsigned long m_data,
	unsigned long user_data
)
{
	int data_num = 0;
	struct jc_type_file_manage_oper *oper = NULL;

	oper = (typeof(oper))m_data;
	data_num = (unsigned int)user_data;

	if (oper->manage_copy)
		return oper->manage_copy(data_num);
	return JC_OK;
}

static int
jc_type_file_manage_copy(
	struct jc_type_file_manage *man,
	unsigned int data_num
)
{
	return jc_var_module_traversal(man->jvm, data_num, 
			               jc_type_file_manage_copy_walk);
}

static int
jc_type_file_manage_execute(
	struct jc_type_file_manage *man,
	struct json_config_comm *jcc
)
{
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_manage_oper *oper = NULL;
	struct jc_var_module_param param;

	jtp = (typeof(jtp))jcc->module_private;
	memset(&param, 0, sizeof(param));
	param.un.var = jtp->node_name;
	param.user_data = 0;
	param.depth = jcc->depth;
	oper = (typeof(oper))jc_var_module_get(&param,
					       man->jvm);
	if (oper->manage_execute) 
		return oper->manage_execute(jcc);

	return JC_OK;
}

int
json_config_type_file_manage_destroy(
	struct jc_type_file_manage *fm 
)
{
	if (!fm)
		return JC_OK;

	jc_var_module_destroy(fm->jvm);

	return JC_OK;
}

struct jc_type_file_manage *
json_config_type_file_manage_create()
{
	struct jc_type_file_manage *fm = NULL;

	fm = (typeof(fm))calloc(1, sizeof(*fm));
	if (!fm) {
		fprintf(stderr, "calloc %d bytes error : %s\n", 
				sizeof(*fm), strerror(errno));
		exit(0);
	}

	fm->jvm = jc_var_module_create(NULL, NULL, jc_type_file_manage_node_destroy);
	fm->init = jc_type_file_manage_init;
	fm->copy = jc_type_file_manage_copy;
	fm->execute = jc_type_file_manage_execute;
	fm->module_add = jc_type_file_manage_module_add;

	return JC_OK;
}

