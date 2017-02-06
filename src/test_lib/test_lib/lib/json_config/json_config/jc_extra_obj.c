#include "jc_extra_private.h"
#include "jc_extra_obj_private.h"
#include "jc_comm_func_private.h"
#include "tc_hash.h"
#include <dlfcn.h>

#define JC_EXTRA_OBJ "object"
#define JC_EXTRA_OBJ_HASH_SIZE 26

struct jc_extra_obj_node {
	int  depth;
	char *var_name;
	jc_extra_func func;
	void *dlhandle;
	struct hlist_node node;
};

struct jc_extra_obj_param {
	int depth;
	char *var;
};

struct jc_extra_obj {
	tc_hash_handle_t var_hash;
};

static struct jc_extra_obj global_obj;

static int
jc_extra_obj_add(
	int  depth,
	char *var_name,
	void *func,
	void *handle
)
{
	struct jc_extra_obj_node *evn = NULL;
	
	evn = (struct jc_extra_obj_node *)calloc(1, sizeof(*evn));
	if (!evn) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", 
				sizeof(*evn), strerror(errno));
		exit(0);
	}
	if (var_name)
		evn->var_name = strdup(var_name);
	evn->func = func;
	evn->dlhandle = handle;
	evn->depth = depth;

	return tc_hash_add(global_obj.var_hash, &evn->node, (unsigned long)var_name);	
}

static char *
jc_extra_lib_path_get(
	char *suffix
)
{

}

static int
jc_extra_obj_init(
	struct jc_comm *jcc
) 
{
	char *path = NULL, *real_path = NULL;
	void *handle = NULL;
	jc_extra_func func = NULL;
	cJSON *obj = NULL;
	struct jc_extra_private *jep = NULL;

	jep = (struct jc_extra_private*)jcc->module_private;
	obj = cJSON_GetObjectItem(jep->obj, "lib_path");
	if (!obj) {
		fprintf(stderr, "no lib_path set\n");
		exit(0);
	}
	
	if (obj->valuestring)
		path = jc_escape_splash(obj->valuestring);
	else {
		path = (char*)calloc(1, 3);
		path[0] = '.';
		path[1] = '/';
	}
	if (path[0] == '/') {
		real_path = strdup(path);
		handle = dlopen(real_path, RTLD_LAZY);
	}
	else {
		real_path = jc_extra_lib_path_get(path);
		handle = dlopen(real_path, RTLD_LAZY);
	}
	if (!handle) {
		fprintf(stderr, "open lib file %s error: %s\n", real_path, dlerror());
		exit(0);
	}
	obj = cJSON_GetObjectItem(jep->obj, "func");
	if (!obj) {
		fprintf(stderr, "no func name set\n");
		exit(0);
	}
	func = (jc_extra_func)dlsym(handle, obj->valuestring);

	if (path)
		free(path);
	if (real_path)
		free(path);

	return jc_extra_obj_add(jcc->depth, jep->var_name, func, handle);
}

static int
jc_extra_obj_execute(
	struct jc_comm *jcc
)
{
	struct hlist_node *hnode = NULL;
	struct jc_extra_obj_node *eon = NULL;
	struct jc_extra_private *jep = NULL;
	struct jc_extra_obj_param eop;

	jep = (struct jc_extra_private *)jcc->module_private;
	memset(&eop, 0, sizeof(eop));
	if (jep->var_name)
		eop.var = strdup(jep->var_name);
	eop.depth = jcc->depth;
	hnode = tc_hash_get(
			global_obj.var_hash, 
			(unsigned long)jep->var_name, 
			(unsigned long)&eop);
	if (eop.var)
		free(eop.var);
	if (!hnode) {
		fprintf(stderr, "no variable named %s\n", jep->var_name);
		return JC_ERR;
	}
	eon = tc_list_entry(hnode, struct jc_extra_obj_node, node);
	jcc->end = 1;
	if (eon->func)	
		return eon->func(jcc);

	return JC_OK;
}
	
static int
jc_extra_obj_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	char name = 0;
	struct jc_extra_obj_node *evn = NULL;

	if (!hnode) 
		name = ((char*)user_data)[0];
	else if (!user_data)
		name = 0;
	else {
		evn = tc_list_entry(hnode, struct jc_extra_obj_node, node);
		if (evn->var_name)
			name = evn->var_name[0];
	}

	return (name % JC_EXTRA_OBJ_HASH_SIZE);
}

static int
jc_extra_obj_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int flag = 0;
	struct jc_extra_obj_node *evn = NULL;
	struct jc_extra_obj_param *eop = NULL;

	eop = (struct jc_extra_obj_param *)user_data;
	if (!eop)
		return JC_ERR;
	
	evn = tc_list_entry(hnode, struct jc_extra_obj_node, node);
	if (!evn->var_name && !user_data)
		flag = 1;
	else if (!evn->var_name || !user_data)
		flag = 0;
	else if (!strcmp(evn->var_name, eop->var))
		flag = 1;
	else 
		flag = 0;

	if (flag) {
		if (evn->depth == eop->depth)
			return JC_OK;
	}

	return JC_ERR;
}

static int
jc_extra_obj_hash_destroy(
	struct hlist_node *hnode
)
{
	struct jc_extra_obj_node *evn = NULL;

	evn = tc_list_entry(hnode, struct jc_extra_obj_node, node);
	if (evn->var_name)
		free(evn->var_name);
	if (evn->dlhandle)
		dlclose(evn->dlhandle);
	free(evn);

	return JC_OK;
}

int
json_config_extra_obj_uninit()
{
	return tc_hash_destroy(global_obj.var_hash);
}

int
json_config_extra_obj_init()
{
	global_obj.var_hash = tc_hash_create(
					JC_EXTRA_OBJ_HASH_SIZE, 
					jc_extra_obj_hash, 
					jc_extra_obj_hash_get,
					jc_extra_obj_hash_destroy);
	if (global_obj.var_hash == TC_HASH_ERR)
		return JC_ERR;

	return JC_OK;
}

