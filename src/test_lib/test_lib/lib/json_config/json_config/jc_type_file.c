#include "jc_type_private.h"
#include "jc_type_file_private.h"
#include "jc_comm_func_private.h"
#include "tc_hash.h"

#define JC_TYPE_FILE "file"
#define JC_TYPE_FILE_HASH_SIZE 26

struct jc_type_file_module_node {
	char *module;
	struct jc_type_file_oper oper;
	struct hlist_node node;
};

struct jc_type_file_var_module_node {
	int depth;
	char *var;
//	char *module;
	struct jc_type_file_comm comm;
	struct hlist_node node;
};

struct jc_type_file_var_module_param {
	int depth;
	char *var;
};

struct jc_type_file {
	char *default_module;
	tc_hash_handle_t module_hash;
	tc_hash_handle_t var_hash;
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
	if (module)
		fmn->module = strdup(module);
	if (oper)
		memcpy(&fmn->oper, oper, sizeof(*oper));
	if (default_module)
		global_file.default_module = strdup(module);

	return tc_hash_add(global_file.module_hash, 
			   &fmn->node, 
			   (unsigned long)fmn->module);
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
	fvmn->depth = depth;
	if (var)
		fvmn->var = strdup(var);
	if (comm)
		memcpy(&fvmn->comm, comm, sizeof(*comm));

	return tc_hash_add(global_file.var_hash,
			   &fvmn->node, 
			   (unsigned long)fvmn->var);
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
	struct hlist_node *hnode = NULL;
	struct jc_type_private *jtp = NULL;
	struct jc_type_file_private jtfp;
	struct jc_type_file_comm comm;
	struct jc_type_file_module_node *fmn = NULL;

	memset(&comm, 0, sizeof(comm));
	jc_type_file_comm_get(jcc->conf_val, &comm);


	jtp = (typeof(*jtp)*)jcc->module_private;
	hnode = tc_hash_get(global_file.module_hash, 
			    (unsigned long)comm.module,
			    (unsigned long)comm.module);
	if (!hnode) {
		fprintf(stderr, "no module named %s\n", comm.module);
		return JC_ERR;
	}
	fmn = tc_list_entry(hnode, typeof(*fmn), node);
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
	struct hlist_node *hnode = NULL;	
	struct jc_type_file_private jtfp;
	struct jc_type_file_module_node *fmn = NULL;

	hnode = tc_hash_get(global_file.module_hash, 
			    (unsigned long)fvmn->comm.module, 
			    (unsigned long)fvmn->comm.module);
	if (!hnode) {
		fprintf(stderr, "no module named %s\n", fvmn->comm.module);
		return JC_ERR;
	}
	fmn = tc_list_entry(hnode, typeof(*fmn), node);
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
	struct hlist_node *hnode = NULL;
	struct jc_type_private *jtp;
	struct jc_type_file_var_module_node *fvmn = NULL;
	struct jc_type_file_var_module_param fvmp;

	jtp = (typeof(*jtp)*)jcc->module_private;
	memset(&fvmp, 0, sizeof(fvmp));
	if (jtp->node_name)
		fvmp.var = strdup(jtp->node_name);
	fvmp.depth = jcc->depth;
	hnode = tc_hash_get(global_file.var_hash, 
			    (unsigned long)jtp->node_name,
			    (unsigned long)&fvmp);
	if (fvmp.var)
		free(fvmp.var);
	if (!hnode) {
		fprintf(stderr, "no variable named %s in depth %d\n", 
					jtp->node_name, fvmp.depth);
		return JC_ERR;
	}
	fvmn = tc_list_entry(hnode, typeof(*fvmn), node);
	
	return jc_type_file_module_execute(fvmn, jtp, jcc);
}

static int
jc_type_file_copy_walk(
	unsigned long user_data,
	struct hlist_node *hnode,
	int *flag
)
{
	struct jc_type_file_module_node *fmn = NULL;

	(*flag) = 0;
	fmn = tc_list_entry(hnode, typeof(*fmn), node);
	if (fmn->oper.file_copy) 
		fmn->oper.file_copy((unsigned int)user_data);

	return JC_OK;
}

static int
jc_type_file_copy(
	unsigned int data_num
)
{
	return tc_hash_traversal(
			(unsigned long)data_num, 
			global_file.module_hash, 
			jc_type_file_copy_walk);
}

int
json_config_type_file_uninit()
{
	if (global_file.module_hash) {
		tc_hash_destroy(global_file.module_hash);
		free(global_file.module_hash);
	}
	if (global_file.var_hash) {
		tc_hash_destroy(global_file.var_hash);
		free(global_file.var_hash);
	}
	if (global_file.default_module)
		free(global_file.default_module);

	return JC_OK;
}

static int
jc_type_file_module_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_type_file_module_node *fmn = NULL;

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (!user_data)
		name = 0;
	else {
		fmn = tc_list_entry(hnode, typeof(*fmn), node);
		if (fmn->module)
			name = fmn->module[0];
	}

	return (name % JC_TYPE_FILE_HASH_SIZE);
}

static int
jc_type_file_module_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_type_file_module_node *fmn = NULL;

	fmn = tc_list_entry(hnode, typeof(*fmn), node);
	if (!user_data && !fmn->module)
		return JC_OK;
	if (!user_data || !fmn->module)
		return JC_ERR;
	if (!strncmp((char*)user_data, fmn->module, strlen(fmn->module)))
		return JC_OK;

	return JC_ERR;
}

static int
jc_type_file_module_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_file_module_node *fmn = NULL;

	fmn = tc_list_entry(hnode, typeof(*fmn), node);
	if (fmn->module)
		free(fmn);
	free(fmn);

	return JC_OK;
}

static int
jc_type_file_var_module_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_type_file_var_module_node *fvmn = NULL;

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (!user_data)
		name = 0;
	else {
		fvmn = tc_list_entry(hnode, typeof(*fvmn), node);
		if (fvmn->var)
			name = fvmn->var[0];
	}

	return (name % JC_TYPE_FILE_HASH_SIZE);
}

static int
jc_type_file_var_module_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int flag = 0;
	struct jc_type_file_var_module_node *fvmn = NULL;
	struct jc_type_file_var_module_param *fvmp = NULL;

	fvmp = (typeof(*fvmp)*)user_data;
	fvmn = tc_list_entry(hnode, typeof(*fvmn), node);
	if (!fvmp->var && !fvmn->var)
		flag = 1;
	else if (!fvmp->var || !fvmn->var)
		flag = 0;
	else if (!strcmp(fvmp->var, fvmn->var))
		flag = 1;
	else 
		flag = 0;
	if (flag) {
		if (fvmp->depth == fvmn->depth)
			return JC_OK;
	}

	return JC_ERR;
}

static int
jc_type_file_var_module_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_file_var_module_node *fvmn = NULL;

	fvmn = tc_list_entry(hnode, typeof(*fvmn), node);
	if (fvmn->var)
		free(fvmn->var);
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

	global_file.module_hash = tc_hash_create(
						JC_TYPE_FILE_HASH_SIZE, 
						jc_type_file_module_hash,
						jc_type_file_module_hash_get, 
						jc_type_file_module_hash_destroy);
	if (global_file.module_hash == TC_HASH_ERR)
		return JC_ERR;

	global_file.var_hash = tc_hash_create(
					     JC_TYPE_FILE_HASH_SIZE, 
					     jc_type_file_var_module_hash, 
					     jc_type_file_var_module_hash_get,
					     jc_type_file_var_module_hash_destroy);
	if (global_file.var_hash == TC_HASH_ERR) {
		tc_hash_destroy(global_file.module_hash);
		free(global_file.module_hash);
		return JC_ERR;
	}

	memset(&oper, 0, sizeof(oper));
	oper.jc_type_copy = jc_type_file_copy;
	oper.jc_type_init = jc_type_file_init;
	oper.jc_type_execute = jc_type_file_execute;

	return jc_type_module_add(JC_TYPE_FILE, &oper);
}
