#include <pthread.h>
#include "json_config_private.h"

struct json_config_rename{
	char *orignal_name;
	char *new_name;
	struct list_head node;
};

struct json_config_module {
	char *name;
	int level;
	struct list_head node;
	struct json_config_oper oper;
};

struct json_config_data {
	unsigned int	 vuser_num;
	pthread_mutex_t  json_list_mutex;
	struct list_head json_module_list;
	struct list_head json_rename_list;
};

struct json_config {
	cJSON *json_root;
	struct list_head node;
};

static struct json_config_data global_json_config = {
	.json_module_list = LIST_HEAD_INIT(global_json_config.json_module_list),
	.json_rename_list = LIST_HEAD_INIT(global_json_config.json_rename_list),
	.json_list_mutex = PTHREAD_MUTEX_INITIALIZER
};

static struct json_config_rename *
json_config_rename_get_by_new_name(
	char *name
)
{
	struct list_head *sl = NULL;
	struct json_config_rename *jcr = NULL;

	list_for_each_entry(jcr, &global_json_config.json_rename_list, node) {
		jcr = list_entry(sl, struct json_config_rename, node);
		if (!strcmp(jcr->new_name, name))
			return jcr;
		sl = sl->next;
	}

	return NULL;
}

static struct json_config_rename*
json_config_rename_get_by_orignal_name(
	char *name
)
{
	struct list_head *sl = NULL;
	struct json_config_rename *jcr = NULL;

	list_for_each_entry(jcr, &global_json_config.json_rename_list, node) {
		jcr = list_entry(sl, struct json_config_rename, node);
		if (!strcmp(jcr->orignal_name, name))
			return jcr;
		sl = sl->next;
	}

	return NULL;
}

int
json_config_rename_add(
	char *orignal_name,
	char *new_name
)
{
	struct json_config_rename *jc_rename = NULL;

	jc_rename = json_config_rename_get_by_orignal_name(orignal_name);
	if (jc_rename) {
		free(jc_rename->new_name);
		jc_rename->new_name = strdup(new_name);
		return JC_OK;
	}
	jc_rename = (struct json_config_rename*)calloc(1, sizeof(*jc_rename));
	if (!jc_rename) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", sizeof(*jc_rename), strerror(errno));
		exit(0);
	}
	jc_rename->orignal_name = strdup(orignal_name);
	jc_rename->new_name = strdup(orignal_name);
	list_add_tail(&global_json_config.json_rename_list, &jc_rename->node);
	
	return JC_OK;
}

static void
json_config_level_add(
	struct json_config_module *new_jm
)
{
	struct list_head *sl = NULL;
	struct json_config_module *jm = NULL;

	list_for_each_entry(jm, &global_json_config.json_module_list, node) {
		jm = list_entry(sl, struct json_config_module, node);
		if (jm->level < new_jm->level) {
			list_add_tail(jm->node.prev, &new_jm->node);
			break;
		}
	}	
}

int
json_config_module_add(
	char *name,
	int  level,
	struct json_config_oper *js_oper
)
{
	struct json_config_module *js_mod = NULL;

	js_mod = (struct json_config_module*)calloc(1, sizeof(*js_mod));
	if (!js_mod) {
		fprintf(stderr, "can't calloc %d bytes : %s\n", sizeof(*js_mod), strerror(errno));
		exit(0);
	}
	if (name)
		js_mod->name = strdup(name);
	memcpy(&js_mod->oper, js_oper, sizeof(*js_oper));
	json_config_level_add(js_mod);
	//list_add_tail(&global_json_config.json_module_list, &js_mod->node);

	return JC_OK;
}

static struct json_config_module *
json_config_module_get(
	char *module
)
{
	struct list_head *sl = NULL;
	struct json_config_module *jm = NULL;
	
	list_for_each_entry(jm, &global_json_config.json_module_list, node) {
		jm = list_entry(sl, struct json_config_module, node);
		if (!strcmp(module, jm->name))
			return jm;
		sl = sl->next;
	}

	return NULL;
}

int
json_config_init(
	int vuser_num
)
{
	global_json_config.vuser_num = vuser_num;
	//json_config_mode_init();
	//json_config_type_init();
}

int 
json_config_uninit()
{
}

static int
json_config_node_handle(
	cJSON *root,
	unsigned long user_data,
	struct json_config_comm *jcc
)
{
	int ret = JC_OK;
	return ret;
}

static void
json_config_comm_free(
	struct json_config_comm *jcc
)
{
	if (jcc->retval)
		free(jcc->retval);
	if (jcc->conf_val)
		cJSON_Delete(jcc->conf_val);
	if (jcc->mode_type)
		free(jcc->mode_type);
}

/*
 * 该处理方式只根据
 */
static cJSON *
json_config_to_param_walk(
	int id,
	int depth,
	unsigned long user_data,
	cJSON *root
)
{
	int ret = 0;
	char *mod = NULL; 
	cJSON *obj = NULL; 
	struct json_config_rename *jr = NULL; 
	struct json_config_module *jm = NULL; 
	struct json_config_comm jcc;

	memset(&jcc, 0, sizeof(jcc));

/*	jcc.walk_cb = json_config_to_param_walk;
	jcc.id = id;
	jcc.depth = depth;
	list_for_each_entry(jm, &global_json_config.json_module_list, node) {
		jr = json_config_rename_get_by_orignal_name(jm->name); 
		if (jr) 
			mod = jr->new_name; 
		else 
			mod = jm->name; 
		obj = cJSON_GetObjectItem(root, mod); 
		if (!obj) { 
			fprintf(stderr, "no object named %s whose orignal name is %sn", mod, jm->name); 
			continue;
		} 
		if (jm->oper.jc_execute) { 
			ret = jm->oper.jc_execute(root->string, id, &jcc);
			if (ret != JC_OK || jcc.end != 0) 
				break; 
		} 
	} */
	obj = (cJSON*)jcc.out_data;
	jcc.out_data = 0;
	
	json_config_comm_free(&jcc);

	if (ret != JC_OK) {
		if (obj)
			cJSON_Delete(obj);
		obj = NULL;
	}

	return obj;
}

char *
json_config_to_param(
	int id,
	unsigned long user_data,
	cJSON *root
)
{
	char *ret = NULL;
	cJSON *obj = NULL;

	obj = json_config_to_param_walk(id, 0, user_data, root);
	if (!obj)
		return NULL;

	ret = cJSON_Print(obj);

	cJSON_Delete(obj);

	return ret;
}

static cJSON *
json_config_param_init_walk(
	int id,
	int depth,
	unsigned long user_data,
	cJSON *root
)
{
	int ret = 0;
	char *mod = NULL; 
	cJSON *obj = NULL; 
	struct json_config_rename *jr = NULL; 
	struct json_config_module *jm = NULL; 
	struct json_config_comm jcc;

	memset(&jcc, 0, sizeof(jcc));
	jcc.walk_cb = json_config_param_init_walk;
	list_for_each_entry(jm, &global_json_config.json_module_list, node) { 
		jr = json_config_rename_get_by_orignal_name(jm->name); 
		if (jr) 
			mod = jr->new_name; 
		else 
			mod = jm->name; 
		obj = cJSON_GetObjectItem(root, mod); 
		if (!obj) { 
			fprintf(stderr, "no object named %s whose orignal name is %sn", mod, jm->name); 
			continue;
		} 
		if (jm->oper.jc_init) { 
			ret = jm->oper.jc_init(root->string, obj, user_data, &jcc);
			if (ret != JC_OK || jcc.end != 0) 
				break; 
		} 
	} 
	obj = (cJSON*)jcc.out_data;
	jcc.out_data = 0;
	
	json_config_comm_free(&jcc);

	if (ret != JC_OK) {
		if (obj)
			cJSON_Delete(obj);
		obj = NULL;
	}

	return obj;
}

static int
json_config_vuser_param_init()
{
	int ret = JC_OK;
	struct json_config_module *jcm = NULL;

	list_for_each_entry(jcm, &global_json_config.json_module_list, node) {
		if (jcm->oper.jc_copy) {
			ret = jcm->oper.jc_copy(global_json_config.vuser_num);
			if (ret != JC_OK)
				break;
		}
	}

	return ret;
}

cJSON *
json_config_param_init(
	int id,
	unsigned long user_data,
	cJSON *root
)
{
	cJSON *obj = NULL;

	obj = json_config_param_init_walk(id, 0, user_data, root);	

	json_config_vuser_param_init();

	return obj;
}

