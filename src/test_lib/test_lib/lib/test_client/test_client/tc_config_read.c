#include "tc_comm.h"
#include "tc_init.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_config_read.h"

#include "toml.h"

/*
 * The implementation of configure reading at present is based on libtoml for v0.3. We hope 
 * we can use the newest version, but its compilation costs too much time. We will try to 
 * compile the newest libtoml later. 
 *
 * The configure file's archeture is blowing:
 * [general]    // we hope set this at the first line of every config file, we will provide 
 *		// a configure file model with this.
 * name = val
 * name2 = val2
 * ...
 *
 * [table1]
 * name3 = val3
 * name4 = val4
 * ...
 *
 * [table2]
 * name5 = val5
 * name6 = val6
 *
 * After configure file reading, we will get a json object like this:
 * {
 *	general: {
 *		name : val,
 *		name2 : val2
 *	},
 *	table1: {
 *		name3 : val3,
 *		name4 : val4
 *	},
 *	table2: {
 *		name5 : val5,
 *		name6 : val6
 *	}
 * }
 *
 * Then we can use tc_config_read_get function to get each object. Its code like this:
 * cJSON *obj = NULL;
 * obj = tc_config_read_get("general");
 * ...
 * obj = tc_config_read_get("table1");
 * then we can use obj to get its special element.
 *
 * We import this for more flexible configure. User don't need to register any disposition
 * function, just getting the config json object to access the configure element.
 */

struct tc_config_read_data {
	cJSON *cur_data;
	cJSON *config_data;
};

static struct tc_config_read_data global_read_data;

cJSON *
tc_config_read_get(
	char *table_name
)
{
	return cJSON_GetObjectItem(global_read_data.config_data, table_name);
}

void
tc_config_read_del(
	char *table_name
)
{
	cJSON_DeleteItemFromObject(global_read_data.config_data, table_name);
}

static int
tc_config_read_first_line_get(
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
tc_config_read_cur_file_path_get(
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

	ret = tc_config_read_first_line_get(path, line);
	if (ret != TC_OK)
		return ret;

	memset(&path[len], 0, 1024 - len);	
	strcat(path, "/");
	strcat(path, line);
	memset(line, 0, sizeof(line));
	ret = tc_config_read_first_line_get(path, line);
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

static char *
tc_config_read_file_path_get()
{
	int ret = 0;
	int real_len = 0;
	int file_path_len = 256;
	char *file_path = NULL;

	file_path = (char*)calloc(file_path_len, sizeof(char));
	if (!file_path) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	ret = TC_ERR;

	while (ret != TC_OK) {
		ret = tc_config_read_cur_file_path_get(file_path_len, file_path, &real_len);
		if (ret != TC_OK) {
			if (real_len <= file_path_len) {
				TC_ERRNO_SET(TC_OPEN_FILE_ERR);
				TC_FREE(file_path);
				return NULL;
			}
			file_path_len = real_len;
			real_len = 0;
			file_path = (char*)realloc(file_path, file_path_len);
			if (!file_path) {
				TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
				return NULL;
			}
		}
	}

	return file_path;
}

static char *
tc_config_read_file_content_get(
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
		return NULL;
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

static void
tc_config_read_walk(
	struct toml_node *tnode,
	void		 *data
)
{
	int ret = 0;
	int flag = 0;
	char *tmp = NULL;
	char *name = NULL, *val = NULL;
	enum toml_type ttype;
	cJSON *node = NULL;

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
		global_read_data.cur_data = cJSON_CreateObject();
		cJSON_AddItemToObject(global_read_data.config_data, 
				      name, 
				      global_read_data.cur_data);
		TC_FREE(name);
		break;	
	default:
		val = toml_value_as_string(tnode);
		name = toml_name(tnode);
		/*
		 * libtoml will add a '/' when it faces with a '/' in the configure file.
		 * But We always hope to use it directly, so we delete the new '/'
		 */
		if (!val || !strstr(val, "/")) 
			tmp = val;
		else {
			tmp = tc_escape_splash(val);
			flag = 1;
		}
		node = cJSON_CreateString(tmp);
		if (!global_read_data.cur_data) {
			global_read_data.cur_data = cJSON_CreateObject();
			cJSON_AddItemToObject(global_read_data.config_data, 
					      "anonymous", 
					      global_read_data.cur_data);
		}
		cJSON_AddItemToObject(global_read_data.cur_data, 
				      name, 
				      node);
		TC_FREE(name);
		TC_FREE(val);
		if (flag == 1) {
			TC_FREE(tmp);
			flag = 0;
		}
	}
}



static int
tc_config_file_read(
	char *file_path
)
{
	int  file_size = 0;
	char *file_content = NULL;
	struct toml_node *troot = NULL;

	file_content = tc_config_read_file_content_get(file_path, &file_size);
	if (!file_content) 
		return TC_ERR;

	toml_init(&troot);
	toml_parse(troot, file_content, file_size);
	toml_walk(troot, tc_config_read_walk, NULL);

	TC_FREE(file_content);

	return TC_OK;
}

int
tc_config_read_handle()
{
	int ret = 0;	
	int handle_result = 0;
	char *file_path = NULL;

	file_path = tc_config_read_file_path_get();
	ret = tc_config_file_read(file_path);
	if (ret != TC_OK)
		TC_ERRNO_SET(ret);

	TC_FREE(file_path);
	return ret;
}

static int
tc_config_read_uninit()
{
	cJSON_Delete(global_read_data.config_data);
}

int
tc_config_read_init()
{
	global_read_data.config_data = cJSON_CreateObject();
	if (!global_read_data.config_data)
		return TC_ERR;

	return tc_uninit_register(tc_config_read_uninit);
}

TC_MOD_INIT(tc_config_read_init);
