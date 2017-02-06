#include "jc_private.h"
#include "jc_extra_private.h"
#include "tc_hash.h"

#define JC_EXTRA "extra"
#define JC_EXTRA_LEVEL 400
#define JC_EXTRA_HASH_SIZE 26

struct jc_extra_module_node {
	char *module_name;
	struct jc_extra_oper oper;
	struct hlist_node node;
};

struct jc_extra_var_module_node {
	int  depth;
	char *var_name;
	char *module_name;
	struct hlist_node node;
};

struct jc_extra_var_module_param {
	int depth;
	char *var;
};

struct jc_extra {
	tc_hash_handle_t var_module;
	tc_hash_handle_t module_handle;
}; 
	
static struct jc_extra global_extra;

int
jc_extra_module_add(
	char *module_name,
	struct jc_extra_oper *oper
)
{
	struct jc_extra_module_node *emn = NULL;

	emn = (struct jc_extra_module_node*)calloc(1, sizeof(*emn));
	if (!emn) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				sizeof(*emn), strerror(errno));
		exit(0);
	}
	if (module_name)
		emn->module_name = strdup(module_name);
	if (oper)
		memcpy(&emn->oper, oper, sizeof(*oper));

	return tc_hash_add(global_extra.module_handle, 
			   &emn->node, 
			   (unsigned long)module_name);
}

static int
jc_extra_var_module_add(
	int  depth,
	char *var,
	char *module
)
{
	struct jc_extra_var_module_node *vmn = NULL;

	vmn = (struct jc_extra_var_module_node*)calloc(1, sizeof(*vmn));
	if (!vmn) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				sizeof(*vmn), strerror(errno));
		exit(0);
	}
	if (var) 
		vmn->var_name = strdup(var);
	if (module)
		vmn->module_name = strdup(module);
	vmn->depth = depth;

	return tc_hash_add(global_extra.var_module, 
			   &vmn->node, 
			   (unsigned long)vmn->var_name);
}

static int
jc_extra_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;		
	struct jc_extra_private jep;
	struct jc_extra_module_node *emn = NULL;

	hnode = tc_hash_get(global_extra.module_handle, 
			    (unsigned long)obj->valuestring, 
			    (unsigned long)obj->valuestring);
	if (!hnode) {
		fprintf(stderr, "no module named %s\n", node_name);
		return JC_ERR;
	}
	emn = tc_list_entry(hnode, struct jc_extra_module_node, node);
	memset(&jep, 0, sizeof(jep));
	if (node_name)
		jep.var_name = strdup(node_name);
	jep.user_data = user_data;
	jep.obj = obj;
	jcc->module_private = (unsigned long)&jep;
	if (emn->oper.extra_init)
		ret = emn->oper.extra_init(jcc);
	if (jep.var_name)
		free(jep.var_name);

	if (ret == JC_OK)
		return jc_extra_var_module_add(jcc->depth, node_name, obj->valuestring);

	return ret;
}

static int
jc_extra_module_execute(
	char *node_name,
	char *module_name,
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;		
	struct jc_extra_private jep;
	struct jc_extra_module_node *emn = NULL;

	hnode = tc_hash_get(global_extra.module_handle, 
			    (unsigned long)module_name, 
			    (unsigned long)module_name);
	if (!hnode) {
		fprintf(stderr, "no module named %s\n", module_name);
		return JC_ERR;
	}
	emn = tc_list_entry(hnode, struct jc_extra_module_node, node);

	memset(&jep, 0, sizeof(jep));
	if (node_name)
		jep.var_name = strdup(node_name);
	jep.user_data = user_data;
	jep.other_extra_module = jc_extra_module_execute;
	jcc->module_private = (unsigned long)&jep;
	if (emn->oper.extra_execute)
		ret = emn->oper.extra_execute(jcc);
	if (jep.var_name)
		free(jep.var_name);

	return ret;
}

static int
jc_extra_execute(
	char *node_name,	
	unsigned long user_data,
	struct jc_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;		
	struct jc_extra_var_module_node *vmn = NULL;
	struct jc_extra_var_module_param vmp;

	memset(&vmp, 0, sizeof(vmp));
	if (node_name)
		vmp.var = strdup(node_name);
	vmp.depth = jcc->depth;
	hnode = tc_hash_get(global_extra.var_module, 
			    (unsigned long)node_name, 
			    (unsigned long)&vmp);
	if (vmp.var)
		free(vmp.var);
	if (!hnode) {
		fprintf(stderr, "no variable named %s in depth %d\n", 
						node_name, jcc->depth);
		return JC_ERR;
	}
	vmn = tc_list_entry(hnode, struct jc_extra_var_module_node, node);

	return jc_extra_module_execute(node_name, vmn->module_name, user_data, jcc);
}

int
json_config_extra_uninit()
{
	tc_hash_destroy(global_extra.module_handle);
	return JC_OK;
}

static int
jc_extra_module_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_extra_module_node *emn = NULL;	

	if (!hnode)
		name = ((char *)user_data)[0];
	else {
		emn = tc_list_entry(hnode, struct jc_extra_module_node, node);
		if (emn->module_name)	
			name = emn->module_name[0];
	}

	return (name % JC_EXTRA_HASH_SIZE);
}

static int
jc_extra_module_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct jc_extra_module_node *emn = NULL;

	emn = tc_list_entry(hnode, struct jc_extra_module_node, node);
	if (!emn->module_name && !user_data)
		return JC_OK;
	if (!emn->module_name || !user_data)
		return JC_ERR;
	if (!strncmp(emn->module_name, (char*)user_data, strlen(emn->module_name)))
		return JC_OK;

	return JC_ERR;
}

static int
jc_extra_module_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_extra_module_node *emn = NULL;

	emn = tc_list_entry(hnode, struct jc_extra_module_node, node);
	if (emn->module_name)
		free(emn->module_name);

	free(emn);

	return JC_OK;
}

static int
jc_extra_var_module_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_extra_var_module_node *vmn = NULL;

	if (!hnode) 
		name = ((char*)user_data)[0];
	else {
		vmn = tc_list_entry(hnode, struct jc_extra_var_module_node, node);
		if (vmn->var_name)
			name = vmn->var_name[0];
	}

	return (name % JC_EXTRA_HASH_SIZE);

}

static int
jc_extra_var_module_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int flag = 0;
	struct jc_extra_var_module_node *vmn = NULL;
	struct jc_extra_var_module_param *vmp = NULL;

	vmp = (struct jc_extra_var_module_param *)user_data;
	if (!vmp)
		return JC_ERR;
	vmn = tc_list_entry(hnode, struct jc_extra_var_module_node, node);
	if (!vmn->var_name && !vmp->var)
		flag = 1;
	else if (!vmn->var_name || !vmp->var)
		flag = 0;
	else if (!strcmp(vmn->var_name, vmp->var))
		flag = 1;
	else
		flag = 0;

	if (flag) {
		if (vmn->depth == vmp->depth)
			return JC_OK;
	}

	return JC_ERR;
}

static int
jc_extra_var_module_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_extra_var_module_node *vmn = NULL;

	vmn = tc_list_entry(hnode, struct jc_extra_var_module_node, node);
	if (vmn->var_name)
		free(vmn->var_name);
	if (vmn->module_name)
		free(vmn->module_name);

	free(vmn);

	return JC_OK;
}

int
json_config_extra_init()
{
	struct jc_oper oper;

	global_extra.module_handle = tc_hash_create(
					JC_EXTRA_HASH_SIZE, 
					jc_extra_module_hash, 
					jc_extra_module_hash_get, 
					jc_extra_module_hash_destroy);
	if (global_extra.module_handle == TC_HASH_ERR) 
		return JC_ERR;

	global_extra.var_module = tc_hash_create(
					JC_EXTRA_HASH_SIZE, 
					jc_extra_var_module_hash, 
					jc_extra_var_module_hash_get, 
					jc_extra_var_module_hash_destroy);
	if (global_extra.var_module == TC_HASH_ERR) {
		tc_hash_destroy(global_extra.module_handle);
		free(global_extra.var_module);
		return JC_ERR;
	}

	memset(&oper, 0, sizeof(oper));
	oper.jc_init = jc_extra_init;
	oper.jc_execute = jc_extra_execute;

	return jc_module_add(JC_EXTRA, JC_EXTRA_LEVEL, &oper);
}

