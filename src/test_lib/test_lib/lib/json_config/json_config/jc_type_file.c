#include "jc_type_private.h"
#include "jc_type_file_private.h"
#include "jc_comm_func_private.h"
#include "jc_var_module_hash_private.h"
#include "tc_hash.h"

#define JC_TYPE_FILE "file"
#define JC_TYPE_FILE_HASH_SIZE 26

struct jc_type_file_module_node {
	struct jc_type_file_oper oper;
};

struct jc_type_file_var_module_node {
	struct jc_type_file_comm comm;
};

struct jc_type_file {
	char *default_module;
	jc_var_module_t vm;
};

static struct jc_type_file global_file;

static int
jc_type_file_execute(
	struct jc_comm *jcc
);

int
jc_type_file_module_add(
	int  default_module,
	char *module,
	struct jc_type_file_oper *oper
)
{
	struct jc_type_file_module_node *fmn = NULL;

	fmn = (typeof(*fmn) *)calloc(1, sizeof(*fmn));
	if (!fmn) {
		fprintf(stderr, "can't calloc %d bytes: %s\n", 
				sizeof(*fmn), strerror(errno));
		exit(0);
	}
	if (oper)
		memcpy(&fmn->oper, oper, sizeof(*oper));
	if (default_module)
		global_file.default_module = strdup(module);

	return jc_var_module_add(module, (unsigned long)fmn, global_file.vm);
}

static int
jc_type_file_var_module_add(
	int  depth,
	char *var,
	struct jc_type_file_comm *comm
)
{
	struct jc_type_file_var_module_node *fvmn = NULL;

	fvmn = (typeof(*fvmn)*)calloc(1, sizeof(*fvmn));
	if (!fvmn) {
		fprintf(stderr, "can't calloc %d bytes\n", 
				sizeof(*fvmn), strerror(errno));
		exit(0);
	}
	if (comm)
		memcpy(&fvmn->comm, comm, sizeof(*comm));

	return jc_var_add(depth, var, fvmn->comm.module, 
			  (unsigned long)fvmn, global_file.vm);
}

static void
jc_type_file_comm_get(
	cJSON *root,
	struct jc_type_file_comm *comm
)
{
	cJSON *obj = NULL;

	obj = cJSON_GetObjectItem(root, "path");
	if (obj && obj->valuestring)
		comm->path = jc_path_get(obj->valuestring);
	else {
		fprintf(stderr, "no file path set\n");
		exit(0);
	}

	obj = cJSON_GetObjectItem(root, "end_action");
	if (!obj || !obj->valuestring) 
		comm->end_action = strdup(JC_TYPE_FILE_COMM_DEFAULT_END);
	else 
		comm->end_action = strdup(obj->valuestring);

	obj = cJSON_GetObjectItem(root, "next_row");
	if (!obj || !obj->valuestring) 
		comm->module = strdup(global_file.default_module);
	else 
		comm->module = strdup(obj->valuestring);	
		
	obj = cJSON_GetObjectItem(root, "separate");
	if (!obj || !obj->valuestring)	
		comm->separate = JC_TYPE_FILE_COMM_DEFAULT_SEP;
	else 
		comm->separate = obj->valuestring[0];

	obj = cJSON_GetObjectItem(root, "col_name");
	if (obj && obj->valuestring)
		comm->col_num = jc_col_num_get(comm->separate, obj->valuestring, comm->path);

	if (comm->col_num <= 0) {
		obj = cJSON_GetObjectItem(root, "col_num");
		if (!obj || obj->valueint == 0)
			comm->col_num = 1;
		else
			comm->col_num = atoi(obj->valuestring);
	}	
}

static int
jc_type_file_init(
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private jtfp;
	struct jc_type_file_comm comm;
	struct jc_type_file_module_node *fmn = NULL;
	struct jc_var_module_param mparam;

	memset(&comm, 0, sizeof(comm));
	jc_type_file_comm_get(jcc->conf_val, &comm);
	jtp = (typeof(*jtp)*)jcc->module_private;
	memset(&mparam, 0, sizeof(mparam));
	mparam.un.module = comm.module;
	fmn = (typeof(fmn))jc_module_get(&mparam, global_file.vm);
	if (fmn->oper.file_init) {
		memset(&jtfp, 0, sizeof(jtfp));
		jtfp.file_init_cb = jc_type_file_init;
		jtp->sub_module_private = (unsigned long)&jtfp;
		ret = fmn->oper.file_init(jcc);
	}

	return jc_type_file_var_module_add(jcc->depth, 
					   jtp->node_name,
					   &comm);
}

static int
jc_type_file_module_execute(
	struct jc_type_file_var_module_node *fvmn,
	struct jc_type_private *jtp,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct jc_type_file_private jtfp;
	struct jc_type_file_module_node *fmn = NULL;
	struct jc_var_module_param mparam;

	memset(&mparam, 0, sizeof(mparam));
	mparam.un.module = fvmn->comm.module;
	fmn = (typeof(fmn))jc_module_get(&mparam, global_file.vm);
	if (fmn->oper.file_execute) {
		memset(&jtfp, 0, sizeof(jtfp));
		jtfp.file_execute_cb = jc_type_file_execute;
		jtfp.comm = &fvmn->comm;
		jtp->sub_module_private = (unsigned long)&jtfp;
		ret = fmn->oper.file_execute(jcc);
	}

	return ret;
}

static int
jc_type_file_execute(
	struct jc_comm *jcc
)
{
	struct jc_type_private *jtp;
	struct jc_type_file_var_module_node *fvmn = NULL;
	struct jc_var_module_param mparam;

	jtp = (typeof(*jtp)*)jcc->module_private;
	memset(&mparam, 0, sizeof(mparam));
	mparam.un.var = jtp->node_name;
	mparam.depth = jcc->depth;
	fvmn = (typeof(fvmn))jc_var_module_get(&mparam, global_file.vm);
	if ((unsigned long)fvmn == (unsigned long)JC_ERR)
		return JC_ERR;

	return jc_type_file_module_execute(fvmn, jtp, jcc);
}

static int
jc_type_file_copy_walk(
	unsigned long user_data,
	unsigned long copy_data
)
{
	struct jc_type_file_module_node *fmn = NULL;

	fmn = (typeof(fmn))user_data;
	if (fmn->oper.file_copy) 
		fmn->oper.file_copy((unsigned int)copy_data);

	return JC_OK;
}

static int
jc_type_file_copy(
	unsigned int data_num
)
{
	return jc_var_module_traversal(
				global_file.vm,
				(unsigned long)data_num, 
				jc_type_file_copy_walk);
}

int
json_config_type_file_uninit()
{
	if (global_file.vm)
		jc_var_module_destroy(global_file.vm);
	if (global_file.default_module)
		free(global_file.default_module);

	return JC_OK;
}

static int
jc_type_file_module_hash_destroy(
	unsigned long user_data
)
{
	free((void*)user_data);
	return JC_OK;
}

static int
jc_type_file_var_module_hash_destroy(
	unsigned long user_data
)
{
	struct jc_type_file_var_module_node *fvmn = NULL;

	fvmn = (typeof(fvmn))user_data;
	if (fvmn->comm.col_name)
		free(fvmn->comm.col_name);
	if (fvmn->comm.module)
		free(fvmn->comm.module);
	free(fvmn);

	return JC_OK;
}

int
json_config_type_file_init()
{
	struct jc_type_oper oper;

	global_file.vm = jc_var_module_create(
					NULL,
					jc_type_file_var_module_hash_destroy, 
					NULL,
					jc_type_file_module_hash_destroy);

	memset(&oper, 0, sizeof(oper));
	oper.jc_type_copy = jc_type_file_copy;
	oper.jc_type_init = jc_type_file_init;
	oper.jc_type_execute = jc_type_file_execute;

	return jc_type_module_add(JC_TYPE_FILE, &oper);
}
