#include "tc_std_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init_private.h"
#include "tc_hash.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_config_private.h"
//#include "tc_interface_private.h"

#include "toml.h"

#if 0
/**
 * This module is used to handle the config file. 
 */
struct tc_config_node{
	char *config_name;
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

CONFIG_FUNC(ARRAY)
{
	char *tmp = NULL;
	char *end = NULL;
	char str[256] = { 0 };
	cJSON *node = NULL;
	cJSON **out = (cJSON **)user_data;
	
	if (!val)
		return;
	(*out) = cJSON_CreateArray();
	tmp = val;
	end = strchr(val, ',');
	while (end) {
		memset(str, 0, 256);
		memcpy(str, tmp, end - tmp);
		node = 	cJSON_CreateString(str);
		cJSON_AddItemToArray((*out), node);
		tmp = end + 1;
		end = strchr(val, ',');
	}
	if (tmp) {
		memset(str, 0, 256);
		memcpy(str, tmp, strlen(tmp));
		node = cJSON_CreateString(str);
		cJSON_AddItemToArray((*out), node);
	}
}

CONFIG_FUNC(INT)
{
	*((int*)user_data) = atoi(val);
	//PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(STR)
{
	memcpy((char*)user_data, val, strlen(val));
	//PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(SHORT)
{
	*((short*)user_data) = (short)atoi(val);
	//PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(USHORT)
{
	*((unsigned short*)user_data) = (unsigned short)atoi(val);
	//PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(IP)
{
	*((unsigned int*)user_data) = inet_addr(val);
	//PRINT("name = %s, val =%s\n", name, val);
}

CONFIG_FUNC(TABLE)
{
	PRINT("table\n");
}

CONFIG_FUNC(DURATION)
{
	int day = 0, hour = 0, min = 0, sec = 0;

	sscanf(val, "%d:%d:%d:%d", &day, &hour, &min, &sec);
	if (hour >=24 || min >= 60 || sec >= 60) 
		PRINT("Wrong duration time set : %s\n", val);

	*((int*)user_data) = day * 3600 * 24 + hour * 3600 + min * 60 + sec;
	//PRINT("duration = %d\n", *((int*)user_data));
}

CONFIG_FUNC(PROTO)
{
	if (!strcmp(val, "tcp")) 
		*((int*)user_data) = TC_PROTO_TCP;
	else if (!strcmp(val, "udp"))
		*((int*)user_data) = TC_PROTO_UDP;
	else if (!strcmp(val, "http"))
		*((int*)user_data) = TC_PROTO_HTTP;
	else if (!strcmp(val, "unix_tcp"))
		*((int*)user_data) = TC_PROTO_UNIX_TCP;
	else if (!strcmp(val, "unix_udp"))
		*((int*)user_data) = TC_PROTO_UNIX_UDP;
}

CONFIG_FUNC(DEV)
{
	if (!strcmp(val, "server"))
		*((int*)user_data) = TC_DEV_SERVER;
	else if (!strcmp(val, "client"))
		*((int*)user_data) = TC_DEV_CLIENT;
}

CONFIG_FUNC(SIZE)
{
	int size = 0;
	char *tmp = NULL;

	size = atoi(val);
	tmp = strchr(val, 'K');
	if (tmp) 
		size *= 1024;
	tmp = strchr(val, 'M');
	if (tmp)
		size *= (1024 * 1024);

	*((int*)user_data) = size;
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
		PRINT("fopen error: %s\n", strerror(errno));
		TC_ERRNO_SET(TC_EMPTY_FILE);
		return NULL;
	}
	fread(file_buf, sizeof(char), st_buf.st_size, fp);
	fclose(fp);

	(*file_size) = st_buf.st_size;
	return file_buf;
}

static char *
tc_escape_splash(
	char *val
)
{
	int len = strlen(val);
	int cnt = 0;
	char *tmp = NULL;

	tmp = (char*)calloc(1, len + 1);
	while(*val)
	{
		if( *val == '\\')
		{
			val++;
			continue;
		}
		else if(*val == '\n')
		{
			(*val) = '\0';
		}
		*(tmp + cnt) = *val;
		cnt++;
		val++;
	}
	PRINT("tmp =%s\n", tmp);

	return tmp;
}

struct tc_config_param {
	int toml_type;
	char *name;
	char *val;
};

static void
tc_config_node_traversal(
	struct hlist_node *hnode, 
	unsigned long user_data
)
{
	struct tc_config_node *conf = NULL;
	struct tc_config_param *param = NULL;

	param = (struct tc_config_param*)user_data;
	conf = tc_list_entry(hnode, struct tc_config_node, node);	
	//PRINT("conf->config_name = %s, %s\n", conf->config_name, (char*)param->name);
	if (strcmp(conf->config_name, param->name)) 
		return;

	//PRINT("name = %s\n", param->name);
	//PRINT("val = %s\n", param->val);
	if (conf->config_handle) 
		conf->config_handle(
				param->toml_type,
				param->name,
				param->val,
				conf->user_data);
}

static int
tc_config_node_handle(
	int toml_type,
	char *name, 
	char *val
)
{
	struct tc_config_param param;
	struct hlist_node *hnode = NULL;
	struct tc_config_node *conf= NULL;

	hnode = tc_hash_get(global_config.config_hash,
				(unsigned long)name, 
				(unsigned long)name);
	if (!hnode) {
		TC_ERRNO_SET(TC_NOT_REGITSTER_CONFIG_OPT);
		return TC_ERR;
	}

	param.toml_type = toml_type;
	param.name = name;
	param.val = val;
	tc_hash_head_traversal(
			global_config.config_hash, 
			(unsigned long)name, 
			(unsigned long)&param, 
			tc_config_node_traversal);

	/*conf = tc_list_entry(hnode, struct tc_config_node, node);
	if (conf->config_handle) {
		conf->config_handle(
				toml_type,
				name, 
				val,
				conf->user_data);

	}*/

	return TC_OK;
}

static void
tc_config_walk(
	struct toml_node *tnode,
	void		 *data
)
{
	int ret = 0;
	int flag = 0;
	char *tmp = NULL;
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
		ret = tc_config_node_handle(TC_CONFIG_TOML_TABLE, name, NULL);
		if (ret != TC_OK)
			tc_interface_param_set(name, NULL);
		TC_FREE(name);
		break;	
	default:
		val = toml_value_as_string(tnode);
		name = toml_name(tnode);
		if (!val || !strstr(val, "/")) 
			tmp = val;
		else {
			PRINT("\n");
			tmp = tc_escape_splash(val);
			flag = 1;
		}
		ret = tc_config_node_handle(TC_CONFIG_TOML_NORMAL, name, tmp);
		if (ret != TC_OK)
			tc_interface_param_set(name, tmp);
		TC_FREE(name);
		TC_FREE(val);
		if (flag == 1) {
			TC_FREE(tmp);
			flag = 0;
		}
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

	//PRINT("file_path = %s\n", file_path);
	file_content = tc_config_get_file_content(file_path, &file_size);
	if (!file_content) 
		return TC_ERR;

	//PRINT("file_content = %s\n", file_content);
	toml_init(&troot);
	toml_parse(troot, file_content, file_size);
	toml_walk(troot, tc_config_walk, NULL);

	TC_FREE(file_content);

	return TC_OK;
}

static int
tc_file_first_line_get(
	char *file,
	char *line
)
{
	FILE *fp = NULL;

	//PRINT("file = %s\n", file);
	fp = fopen(file, "r");
	if (!fp)  {
		TC_ERRNO_SET(TC_OPEN_FILE_ERR);
		return TC_ERR;
	}
	fgets(line, 128, fp);
	line[strlen(line) - 1] = '\0';
	fclose(fp);

	return TC_OK;
}

static int
tc_file_path_get(
	int  file_path_len,
	char *file_path,
	int  *real_len
)
{
	int len = 0, ret = 0;
	char line[128] = { 0 };
	char path[1024] = { 0 };
	FILE *fp = NULL;

	getcwd(path, 1024);
	len = strlen(path);
	strcat(path, "/config/comm/curr_dir.txt");

	ret = tc_file_first_line_get(path, line);
	if (ret != TC_OK)
		return ret;

	memset(&path[len], 0, 1024 - len);	
	strcat(path, "/");
	strcat(path, line);
	memset(line, 0, sizeof(line));
	ret = tc_file_first_line_get(path, line);
	if (ret != TC_OK)
		return ret;
	
	memset(&path[len], 0, 1024 - len);
	strcat(path, "/");
	strcat(path, line);

	len = strlen(path);
	PRINT("path = %s, len = %d\n", path, len);
	if (len >= file_path_len) {
		(*real_len) = len + 1;
		return TC_ERR;
	}
	(*real_len) = file_path_len;

	memcpy(file_path, path, len);

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

	/*if (!global_config_oper.get_config_file) {
		TC_ERRNO_SET(TC_NOT_REGISTER_CONFIG);
		return TC_ERR;
	}*/

	file_path = (char*)calloc(file_path_len, sizeof(char));
	if (!file_path) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	ret = TC_ERR;

	while (ret != TC_OK) {
		ret = tc_file_path_get(file_path_len, file_path, &real_len);
		if (ret != TC_OK) {
			if (real_len <= file_path_len) {
				TC_ERRNO_SET(TC_OPEN_FILE_ERR);
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

	/*if (global_config_oper.config_start)
		global_config_oper.config_start();*/

	ret = __tc_config_handle(file_path);
	if (ret != TC_OK)
		TC_ERRNO_SET(ret);

	/*if (global_config_oper.config_end)
		global_config_oper.config_end();*/

	TC_FREE(file_path);
	return ret;
}

int
tc_config_add( 	
	char *conf_name,
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

#endif
