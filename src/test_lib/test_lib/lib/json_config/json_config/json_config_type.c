#include "json_config_private.h"
#include "json_config_type_private.h"
#include "tc_hash.h"

#define JC_TYPE "type"
#define JC_TYPE_LEVEL  500
#define JC_TYPE_HASH_SIZE 26

struct jc_type_module_node {
	char *module;
	struct jc_type_oper oper;
	struct hlist_node node;
};

struct jc_type_var_node {
	int depth;
	char *var;
	char *module;
	struct hlist_node node;
};

struct jc_type_var_param {
	int depth;
	char *var;
};

struct json_config_type {
	tc_hash_handle_t module_hash;
	tc_hash_handle_t var_hash;		
};

static struct json_config_type global_type;

int
json_config_type_module_add(
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
	if (module)
		jtmn->module = strdup(module);
	if (oper)
		memcpy(&jtmn->oper, oper, sizeof(*oper));

	return tc_hash_add(global_type.module_hash, 
			   &jtmn->node,
			   (unsigned long)module);
}

static int
jc_type_var_module_add(
	int depth,
	char *var,
	char *module
)
{
	struct jc_type_var_node *jtvn = NULL;

	jtvn = (struct jc_type_var_node *)calloc(1, sizeof(*jtvn));
	if (jtvn) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				sizeof(*jtvn), strerror(errno));
		exit(0);
	}
	if (var)
		jtvn->var = strdup(var);
	if (module)
		jtvn->module = strdup(module);
	jtvn->depth = depth;

	return tc_hash_add(global_type.var_hash, 
			   &jtvn->node,
			   (unsigned long)jtvn->var);
}

static int
jc_type_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct json_config_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;	
	struct jc_type_private jtp;
	struct jc_type_module_node *jtmn = NULL;

	hnode = tc_hash_get(global_type.module_hash, 
			    (unsigned long)obj->valuestring, 
			    (unsigned long)obj->valuestring);
	if (!hnode) {
		fprintf(stderr, "no module named %s\n", obj->valuestring);
		return JC_ERR;
	}
	jtmn = tc_list_entry(hnode, struct jc_type_module_node, node);
	if (jtmn->oper.jc_type_init) {
		memset(&jtp, 0, sizeof(jtp));
		if (node_name)
			jtp.node_name = strdup(node_name);
		jtp.obj = obj;
		jtp.user_data = user_data;
		jtp.init_cb = jc_type_init;
		jcc->module_private = (unsigned long)&jtp;
		ret = jtmn->oper.jc_type_init(jcc);
		if (jtp.node_name)
			free(jtp.node_name);
	}
	if (ret == JC_OK)
		return jc_type_var_module_add(jcc->depth, node_name, jtmn->module);

	return ret;
}

static int
jc_type_module_execute(
	char *node_name,
	char *module,
	unsigned long user_data,
	struct json_config_comm *jcc
)
{
	int ret = JC_OK;
	struct hlist_node *hnode = NULL;
	struct jc_type_private jtp;
	struct jc_type_module_node *jtmn = NULL;

	hnode = tc_hash_get(global_type.module_hash, 
			    (unsigned long)module,
			    (unsigned long)module);
	if (!hnode) {
		fprintf(stderr, "no module named %s\n", module);
		return JC_ERR;
	}
	jtmn = tc_list_entry(hnode, struct jc_type_module_node, node);
	if (jtmn->oper.jc_type_execute) {
		memset(&jtp, 0, sizeof(jtp));
		if (node_name)
			jtp.node_name = strdup(node_name);
		jtp.user_data = user_data;
		jtp.execute_cb = jc_type_module_execute;
		jcc->module_private = (unsigned long)&jtp;
		ret = jtmn->oper.jc_type_execute(jcc);
		if (jtp.node_name)
			free(jtp.node_name);
	}

	return ret;
}

static int
jc_type_execute(
	char *node_name,
	unsigned long user_data,
	struct json_config_comm *jcc
)
{
	struct hlist_node *hnode = NULL;
	struct jc_type_var_node *jtvn = NULL;
	struct jc_type_var_param jtvp;

	memset(&jtvp, 0, sizeof(jtvp));
	if (node_name)
		jtvp.var = strdup(node_name);
	jtvp.depth = jcc->depth;
	hnode = tc_hash_get(global_type.var_hash, 
			    (unsigned long)node_name, 
			    (unsigned long)&jtvp);
	if (jtvp.var)
		free(jtvp.var);
	if (!hnode) {
		fprintf(stderr, "no variable named %s\n", node_name);
		return JC_ERR;
	}
	jtvn = tc_list_entry(hnode, struct jc_type_var_node, node);

	return jc_type_module_execute(node_name, jtvn->module, user_data, jcc);
}

static int
jc_type_module_copy(
	unsigned long user_data,
	struct hlist_node *hnode,
	int *flag
)
{
	struct jc_type_module_node *jtmn = NULL;

	(*flag) = 0;
	jtmn = tc_list_entry(hnode, struct jc_type_module_node, node);
	if (jtmn->oper.jc_type_copy)
		return jtmn->oper.jc_type_copy((unsigned int)user_data);

	return JC_OK;
}

static int
jc_type_copy(
	unsigned int data_num	
)
{
	return tc_hash_traversal(
			(unsigned long)data_num, 
			global_type.module_hash, 
			jc_type_module_copy);
}

static int
jc_type_module_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_type_module_node *jtmn = NULL;

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (!user_data)
		name = 0;
	else {
		jtmn = tc_list_entry(hnode, struct jc_type_module_node, node);
		if (jtmn->module)
			name = jtmn->module[0];
	}

	return (name % JC_TYPE_HASH_SIZE);
}

static int
jc_type_module_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_type_module_node *jtmn = NULL;

	jtmn = tc_list_entry(hnode, struct jc_type_module_node, node);
	if (!jtmn->module && !user_data)
		return JC_OK;
	if (!jtmn->module || !user_data)
		return JC_ERR;
	if (!strcmp(jtmn->module, (char*)user_data))
		return JC_OK;

	return JC_ERR;
}

static int
jc_type_module_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_module_node *jtmn = NULL;

	jtmn = tc_list_entry(hnode, struct jc_type_module_node, node);
	if (jtmn->module)
		free(jtmn->module);

	free(jtmn);

	return JC_OK;
}

static int
jc_type_var_hash(
	struct hlist_node *hnode,	
	unsigned long user_data
)
{
	char name = 0;
	struct jc_type_var_node *jtvn = NULL;

	if (!hnode && user_data)
		name = ((char*)user_data)[0];
	else if (!user_data)
		name = 0;
	else {
		jtvn = tc_list_entry(hnode, struct jc_type_var_node, node);
		if (jtvn->var)
			name = jtvn->var[0];
	}

	return (name % JC_TYPE_HASH_SIZE);
}

static int
jc_type_var_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int flag = 0;
	struct jc_type_var_node *jtvn = NULL;
	struct jc_type_var_param *jtvp = NULL;

	jtvp = (struct jc_type_var_param *)user_data;
	if (!jtvp)
		return JC_ERR;
	jtvn = tc_list_entry(hnode, struct jc_type_var_node, node);
	if (!jtvn->var && !jtvp->var) 
		flag = 1;
	else if (!jtvn->var || !jtvp->var) 
		flag = 0;
	else if (!strcmp(jtvn->var, jtvp->var))
		flag = 1;
	else
		flag = 0;
	if (flag) {
		if (jtvn->depth == jtvp->depth)
			return JC_OK;
	}

	return JC_ERR;
}

static int
jc_type_var_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_type_var_node *jtvn = NULL;

	jtvn = tc_list_entry(hnode, struct jc_type_var_node, node);
	if (jtvn->var)
		free(jtvn->var);
	if (jtvn->module)
		free(jtvn->module);
	free(jtvn);

	return JC_OK;
}

int
json_config_type_init()
{
	struct json_config_oper oper;

	global_type.module_hash = tc_hash_create(
					JC_TYPE_HASH_SIZE, 
					jc_type_module_hash, 
					jc_type_module_hash_get,
					jc_type_module_hash_destroy);
	if (global_type.module_hash == TC_HASH_ERR)
		return JC_ERR;

	global_type.var_hash = tc_hash_create(
					JC_TYPE_HASH_SIZE,
					jc_type_var_hash,
					jc_type_var_hash_get,
					jc_type_var_hash_destroy);
	if (global_type.var_hash == TC_HASH_ERR){
		tc_hash_destroy(global_type.var_hash);
		free(global_type.var_hash);
		return JC_ERR;
	}

	memset(&oper, 0, sizeof(oper));
	oper.jc_init = jc_type_init;
	oper.jc_execute = jc_type_execute;
	oper.jc_copy = jc_type_copy;

	return json_config_module_add(JC_TYPE, JC_TYPE_LEVEL, &oper);
}

int
json_config_type_uninit()
{
	if (global_type.module_hash) {
		tc_hash_destroy(global_type.module_hash);
		free(global_type.module_hash);
	} 

	if (global_type.var_hash) {
		tc_hash_destroy(global_type.var_hash);
		free(global_type.var_hash);
	}
}

