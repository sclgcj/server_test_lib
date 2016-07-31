#include "tc_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init.h"
#include "tc_hash.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_config_private.h"

#include "toml.h"

/**
 * This module is used to handle the config file. 
 */
struct tc_config_node{
	char *config_name;
	int  toml_type;
	unsigned long user_data;
	void (*config_handle)(int toml_type, char *name, char *val, unsigned long user_data);
	struct hlist_node node;
};

struct tc_config{	
	int config_num;
	tc_hash_handle_t config_hash;
};

static struct tc_config_oper global_config_oper;

#define TC_CONFIG_TABLE_SIZE 26
static struct tc_config global_config;

int
tc_config_oper_register(
	struct tc_config_oper *config_oper
)
{
	if (!config_oper) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
		
	memset(&global_config_oper, 0, sizeof(global_config_oper));
	memcpy(&global_config_oper, config_oper, sizeof(global_config_oper));

	return TC_OK;
}

static char *
tc_config_get_file_content(
	char *file,
	int  *file_size
)
{
	char *file_buf = NULL;
	FILE *fp = NULL;
	struct stat st_buf;

	memset(&st_buf, 0, sizeof(st_buf));
	stat(file, &st_buf);
	if (st_buf.st_size <= 0) {
		TC_ERRNO_SET(TC_EMPTY_FILE);
		return NULL;
	}
	file_buf = (char*)calloc(st_buf.st_size, sizeof(char));
	if (!file_buf) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	fp = fopen(file, "r");
	if (!fp) {
		TC_ERRNO_SET(TC_EMPTY_FILE);
		return NULL;
	}
	fread(file_buf, sizeof(char), st_buf.st_size, fp);
	fclose(fp);

	(*file_size) = st_buf.st_size;
	return file_buf;
}

static void
tc_config_node_handle(
	char *name, 
	char *val
)
{
	struct hlist_node *hnode = NULL;
	struct tc_config_node *conf= NULL;

	hnode = tc_hash_get(global_config.config_hash,
				(unsigned long)name, 
				(unsigned long)name);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGITSTER_CONFIG_OPT);
		return;
	}
	conf = tc_list_entry(hnode, struct tc_config_node, node);
	if (conf->config_handle) {
		conf->config_handle(
				TC_CONFIG_TOML_TABLE, 
				name, 
				val,
				conf->user_data);
	}
}

static void
tc_config_walk(
	struct toml_node *tnode,
	void		 *data
)
{
	char *name = NULL, *val = NULL;
	enum toml_type ttype;

	ttype = toml_type(tnode);

	switch (ttype) {
	case TOML_ROOT:
		break;				
	case TOML_LIST:
		break;
	case TOML_TABLE_ARRAY:
		break;
	case TOML_TABLE:
		name = toml_name(tnode);
		tc_config_node_handle(name, NULL);
		TC_FREE(name);
		break;	
	default:
		val = toml_value_as_string(tnode);
		name = toml_name(tnode);
		tc_config_node_handle(name, val);
		TC_FREE(name);
		TC_FREE(val);
	}
}

static int
__tc_config_handle(
	char *file_path
)
{
	int  file_size = 0;
	char *file_content = NULL;
	struct toml_node *troot = NULL;

	file_content = tc_config_get_file_content(file_path, &file_size);
	if (!file_content) 
		return TC_ERR;

	PRINT("file_content = %s\n", file_content);
	toml_init(&troot);
	toml_parse(troot, file_content, file_size);
	toml_walk(troot, tc_config_walk, NULL);

	TC_FREE(file_content);

	return TC_OK;
}

int
tc_config_handle()
{
	int ret = 0;	
	int real_len = 0;
	int handle_result = 0;
	int file_path_len = 256;
	char *file_path = NULL;

	if (!global_config_oper.get_config_file) {
		TC_ERRNO_SET(TC_NOT_REGISTER_CONFIG);
		return TC_ERR;
	}

	file_path = (char*)calloc(file_path_len, sizeof(char));
	if (!file_path) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	ret = TC_ERR;
	while (ret != TC_OK) {
		ret = global_config_oper.get_config_file(file_path_len, file_path, &real_len);
		if (ret != TC_OK) {
			if (real_len <= file_path_len) {
				TC_ERRNO_SET(TC_GET_CONFIG_FILE_ERR);
				TC_FREE(file_path);
				return TC_ERR;
			}
			file_path_len = real_len;
			real_len = 0;
			file_path = (char*)realloc(file_path, file_path_len);
			if (!file_path) {
				TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
				return TC_ERR;
			}
		}
	}

	if (global_config_oper.config_start)
		global_config_oper.config_start();

	ret = __tc_config_handle(file_path);
	if (ret != TC_OK)
		TC_ERRNO_SET(handle_result);

	if (global_config_oper.config_end)
		global_config_oper.config_end();

	TC_FREE(file_path);
	return ret;
}


int
tc_config_add( 	
	char *conf_name,
	int  type,
	unsigned long user_data,
	void (*config_handle)(int toml_type, char *name, char *val, unsigned long user_data)
)
{
	int name_len = 0;
	struct tc_config_node *conf_node = NULL;

	if (!conf_name) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	conf_node = (struct tc_config_node*)calloc(1, sizeof(*conf_node));
	if (!conf_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	name_len = strlen(conf_name);
	conf_node->config_name = (char*)calloc(1, name_len + 1);
	if (!conf_node->config_name) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	memcpy(conf_node->config_name, conf_name, name_len);
	conf_node->config_handle = config_handle;
	conf_node->user_data	 = user_data;
	conf_node->toml_type	 = type;

	global_config.config_num++;
	return tc_hash_add(global_config.config_hash, &conf_node->node, 0);
}

static int
tc_config_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	char ch = 0;
	struct tc_config_node *conf_node = NULL;

	if (!hnode && !user_data) {
		return TC_ERR;
	}
	if (!hnode) {
		ch = ((char*)user_data)[0];
	} else {
		conf_node = tc_list_entry(hnode, struct tc_config_node, node);
		ch = conf_node->config_name[0];
	}



	return (ch % TC_CONFIG_TABLE_SIZE);
}

static int
tc_config_hash_get(
	struct hlist_node	*hnode,	
	unsigned long		user_data
)
{
	struct tc_config_node *conf_node = NULL;

	if (!hnode || !user_data) 
		return TC_ERR;
	conf_node = tc_list_entry(hnode, struct tc_config_node, node);
	if (strcmp(conf_node->config_name, (char*)user_data)) 
		return TC_ERR;

	return TC_OK;
}

static int
tc_config_hash_destroy(
	struct hlist_node *hnode
)
{
	struct tc_config_node *conf_node = NULL;

	if (!hnode)
		return TC_ERR;

	conf_node = tc_list_entry(hnode, struct tc_config_node, node);
	TC_FREE(conf_node->config_name);
	TC_FREE(conf_node);

	return TC_OK;
}

static int
tc_config_setup()
{
	memset(&global_config, 0, sizeof(global_config));
	global_config.config_hash = tc_hash_create(
						TC_CONFIG_TABLE_SIZE,
						tc_config_hash,
						tc_config_hash_get,
						tc_config_hash_destroy);
	if (global_config.config_hash == (TC_HASH_ERR))
		return TC_ERR;
	return TC_OK;
}

static int
tc_config_uninit()
{
	return tc_hash_destroy(global_config.config_hash);
}

int
tc_config_init()
{
	int ret = 0;

	ret = tc_config_setup();
	if (ret != TC_OK)
		return ret;

	return tc_uninit_register(tc_config_uninit);
}

TC_MOD_INIT(tc_config_init);


