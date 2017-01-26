#include "json_config_private.h"
#include "json_config_mode_private.h"
#include "json_config_mode_each_occurence_private.h"
#include "json_config_mode_each_occurence_private.h"
#include "tc_hash.h"

#define JC_MODE_HASH_SIZE	26
#define JC_MODE			"mode"	
#define JC_MODE_LEVEL		100

struct json_mode_module {
	char *name;
	struct json_mode_oper oper;
	struct hlist_node node;
};

struct json_mode_var {
	char *var;
	int depth;
	char *module;
	struct hlist_node node;
};

struct json_mode_var_param {
	char *var;
	int depth;
};

struct json_mode {
	tc_hash_handle_t mode_hash; //用于存放mode的子模块
	tc_hash_handle_t mode_var_hash;	//用于存放变量与子模块的对应关系
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
	if (name) 
		jmm->name = strdup(name);
	if (oper)
		memcpy(&jmm->oper, oper, sizeof(*oper));

	tc_hash_add(global_mode.mode_hash, &jmm->node, (unsigned long)name);

	return JC_OK;
}

static int
json_mode_var_add(
	int depth,
	char *var,
	char *module
)
{
	struct json_mode_var *jmv = NULL;

	jmv = (struct json_mode_var*)calloc(1, sizeof(*jmv));
	if (!jmv) {
		fprintf(stderr, "can't alloc %d bytes for %s\n", sizeof(*jmv), strerror(errno));
		return JC_ERR;
	}

	if (module)
		jmv->module = strdup(module);
	if (var)
		jmv->var = strdup(var);
	jmv->depth = depth;

	return tc_hash_add(global_mode.mode_var_hash, 
			   &jmv->node, 
			   (unsigned long)var);
}

static int
json_mode_init(
	char *node_name,
	cJSON *obj,
	unsigned long user_data,
	struct json_config_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;
	struct json_mode_private jmp;
	struct json_mode_module *jmm = NULL;

	hnode = tc_hash_get(global_mode.mode_hash, 
			    (unsigned long)obj->valuestring, 
			    (unsigned long)obj->valuestring);
	if (!hnode) {
		fprintf(stderr, "No mode module named %s\n", obj->valuestring);
		return JC_ERR;
	}
	jmm = tc_list_entry(hnode, struct json_mode_module, node);
	
	memset(&jmp, 0, sizeof(jmp));
	jmp.obj = obj;
	jmp.node_name = strdup(node_name);
	jcc->module_private = (unsigned long)&jmp;

	if (jmm->oper.json_mode_init)
		ret = jmm->oper.json_mode_init(jcc);
	if (ret == JC_OK && node_name)
		ret = json_mode_var_add(jcc->depth, node_name, obj->valuestring);

	if (jmp.node_name)
		free(jmp.node_name);

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
	struct json_config_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;
	struct json_mode_module *jmm = NULL;
	struct json_mode_private jmp;

	hnode = tc_hash_get(global_mode.mode_hash, 
			    (unsigned long)module, 
			    (unsigned long)module);
	if (!hnode) {
		fprintf(stderr, "not module named %s\n", module);
		return JC_ERR;
	}
	jmm = tc_list_entry(hnode, struct json_mode_module, node);

	memset(&jmp, 0, sizeof(jmp));
	jmp.other_mode_judge = json_mode_module_execute;
	jmp.data = user_data;
	jcc->module_private = (unsigned long)&jmp;
	if (jmm->oper.json_mode_execute)
		ret = jmm->oper.json_mode_execute(jcc);

	json_mode_private_free(&jmp);

	return ret;
}

static struct json_mode_var *
jc_mode_var_get(
	int  depth,
	char *var 
)
{
	struct hlist_node *hnode = NULL;
	struct json_mode_var *jmv = NULL;
	struct json_mode_var_param jmvp;

	memset(&jmvp, 0, sizeof(jmvp));
	if (var)
		jmvp.var = strdup(var);
	jmvp.depth = depth;
	hnode = tc_hash_get(global_mode.mode_var_hash, 
			    (unsigned long)var,
			    (unsigned long)&jmvp);
	if (jmvp.var)
		free(jmvp.var);
	if (!hnode) {
		fprintf(stderr, "no module named %s in depth %d\n",
				jmvp.var, jmvp.depth);
		return NULL;
	}
	jmv = tc_list_entry(hnode, struct json_mode_var, node);

	return jmv;
}

static int
json_mode_execute(
	char *node_name,
	unsigned long user_data,
	struct json_config_comm *jcc
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;
	struct json_mode_var *jmv = NULL;

	jmv = jc_mode_var_get(jcc->depth, node_name);
	if (!jmv)
		return JC_ERR;

	return json_mode_module_execute(jmv->module, user_data, jcc);
}

static int
jc_mode_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char mname = 0;
	struct json_mode_module *jmm;

	if (!hnode) 
		mname = ((char*)user_data)[0];
	else {
		jmm = tc_list_entry(hnode, struct json_mode_module, node);
		if (jmm->name)
			mname = jmm->name[0];
	}

	return (mname % JC_MODE_HASH_SIZE);
}

static int
jc_mode_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct json_mode_module *jmm = NULL;

	jmm = tc_list_entry(hnode, struct json_mode_module, node);
	if (!strncmp((char*)user_data, jmm->name, strlen(jmm->name)))
		return JC_OK;

	return JC_ERR;
}

static int
jc_mode_hash_destroy(
	struct hlist_node *hnode
)
{
	struct json_mode_module *jmm = NULL;

	jmm = tc_list_entry(hnode, struct json_mode_module, node);
	if (jmm->name)
		free(jmm->name);
	free(jmm);

	return JC_OK;
}

static int
jc_mode_var_hash(
	struct hlist_node *hnode,
	unsigned long	  user_data
)
{
	char var = 0;
	struct json_mode_var *jmv = NULL;

	if (!hnode)
		var = ((char*)user_data)[0];
	else {
		jmv = tc_list_entry(hnode, struct json_mode_var, node);
		if (jmv->var) 
			var = jmv->var[0];
	}

	return (var % JC_MODE_HASH_SIZE);
}

static int
jc_mode_var_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int flag = 0;
	struct json_mode_var *jmv = NULL;
	struct json_mode_var_param *jmvp = NULL;

	jmvp = (struct json_mode_var_param *)user_data;
	if (!jmvp)
		return JC_ERR;
	jmv = tc_list_entry(hnode, struct json_mode_var, node);
	if (!user_data && !jmvp->var) 
		flag = 1;
	else if (!user_data || !jmvp->var) 
		flag = 0;
	else if (!strcmp(jmvp->var, jmv->var))
		flag = 1;
	else
		flag = 0;

	if (flag) {
		if (jmvp->depth == jmv->depth)
			return JC_OK;
	}

	return JC_ERR;
}

static int
jc_mode_var_hash_destroy(
	struct hlist_node *hnode
)
{
	struct json_mode_var *jmv = NULL;

	jmv = tc_list_entry(hnode, struct json_mode_var, node);
	if (jmv->module)
		free(jmv->module);
	if (jmv->var)
		free(jmv->var);

	free(jmv);

	return JC_OK;
}

int
json_config_mode_init()
{
	struct json_config_oper oper;

	global_mode.mode_hash = tc_hash_create(
				JC_MODE_HASH_SIZE, 
				jc_mode_hash, 
				jc_mode_hash_get, 
				jc_mode_hash_destroy);
	if (global_mode.mode_hash == TC_HASH_ERR)
		return JC_ERR;

	global_mode.mode_var_hash = tc_hash_create(
				JC_MODE_HASH_SIZE, 
				jc_mode_var_hash, 
				jc_mode_var_hash_get, 
				jc_mode_var_hash_destroy);
	if (global_mode.mode_var_hash == TC_HASH_ERR) {
		tc_hash_destroy(global_mode.mode_hash);
		free(global_mode.mode_hash);
		global_mode.mode_hash = NULL;
		return JC_ERR;
	}	

	memset(&oper, 0, sizeof(oper));
	oper.jc_init = json_mode_init;
	oper.jc_execute = json_mode_execute;
	return json_config_module_add(JC_MODE, JC_MODE_LEVEL, &oper);

	//json_mode_each_iteration_init();
	//json_mode_each_occurence_init();
	//json_mode_each_once_init();
	//json_mode_unique_init();
}

int
json_config_mode_uninit()
{
	if (global_mode.mode_hash) {
		tc_hash_destroy(global_mode.mode_hash);
		global_mode.mode_hash = NULL;
	}
	if (global_mode.mode_var_hash) {
		tc_hash_destroy(global_mode.mode_var_hash);
		global_mode.mode_var_hash = NULL;
	}

	return JC_OK;
}
