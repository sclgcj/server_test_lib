#include "tc_comm.h"
#include "tc_log.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_init.h"
#include "tc_print.h"
#include <stdarg.h>

#define TC_LOG_HASH_SIZE 26
#define TC_DEFAULT_LOG_SIZE 128
struct tc_log_node {
	char *name;
	char *data;
	char *file;
	int  direct;
	int  data_pos;
	int  data_size;
	struct hlist_node node;
};

struct tc_log_data {
	tc_hash_handle_t log_hash;
};

static struct tc_log_data global_log_data;


static int
tc_log_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	char name = 0;
	struct tc_log_node *log_node;

	if (!hnode && user_data) 
		name = ((char*)user_data)[0];
	else if (hnode) {
		log_node = tc_list_entry(hnode, struct tc_log_node, node);
		name = log_node->name[0];
	}

	return (name % TC_LOG_HASH_SIZE);
}

static int
tc_log_hash_get(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	struct tc_log_node *log_node = NULL;

	log_node = tc_list_entry(hnode, struct tc_log_node, node);
	if (!user_data && !log_node->name)
		return TC_OK;
	if ((!user_data && log_node->name) || 
			(!log_node->name && user_data))
		return TC_ERR;
	if (strcmp((char*)user_data, log_node->name)) 
		return TC_ERR;

	return TC_OK;
}

static int
tc_log_hash_destroy(
	struct hlist_node	*hnode
)
{
	struct tc_log_node *log_node = NULL;

	log_node = tc_list_entry(hnode, struct tc_log_node, node);
	TC_FREE(log_node->name);
	TC_FREE(log_node->data);
	TC_FREE(log_node);

	return TC_OK;
}

struct tc_log_node *
tc_log_node_create(
	int direct,
	char *name,
	char *file
)
{
	int len = 0;
	struct tc_log_node *log_node = NULL;
	
	log_node = (struct tc_log_node*)calloc(1, sizeof(*log_node));
	if (!log_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	if (name) {
		len = strlen(name);
		log_node->name = (char*)calloc(len + 1, sizeof(char));
		if (!log_node->name) {
			TC_FREE(log_node);
			TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
			return NULL;
		}
		memcpy(log_node->name, name, len);
	}
	if (file) {
		len = strlen(file);
		log_node->file = (char*)calloc(len + 1, sizeof(char));
		if (!log_node->file) {
			TC_FREE(log_node->name);
			TC_FREE(log_node);
			TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
			return NULL;
		}
		memcpy(log_node->file, file, len);
	}
	log_node->direct = direct;
	log_node->data_pos = 0;
	log_node->data_size = TC_DEFAULT_LOG_SIZE;
	log_node->data = (char*)calloc(log_node->data_size, sizeof(char));
	if (!log_node->data) {
		TC_FREE(log_node->name);
		TC_FREE(log_node->file);
		TC_FREE(log_node);
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}

	return log_node;
}

int
tc_log_start(
	int  direct,
	char *name,
	char *file
)
{
	int len = 0;
	struct hlist_node *hnode = NULL;
	struct tc_log_node *log_node = NULL;

	hnode = tc_hash_get(
			global_log_data.log_hash, 
			(unsigned long)name, 
			(unsigned long)name);
	if (hnode) {
		log_node = tc_list_entry(hnode, struct tc_log_node, node);
		log_node->data_pos = 0;
		memset(log_node->data, 0, log_node->data_size);
	}
	else 
		log_node = tc_log_node_create(direct, name, file);

	return tc_hash_add(global_log_data.log_hash, &log_node->node, 0);
}

static int
tc_log_string_set(
	struct tc_log_node	*log_node, 
	char			*fmt,
	va_list			arg_ptr
)
{
	int ret = 0;

	while (1) {
		ret = vsnprintf(
				log_node->data + log_node->data_pos, 
				log_node->data_size - log_node->data_pos, 
				fmt, 
				arg_ptr);
		if (ret < log_node->data_size) 
			break;
		log_node->data_size *= 2;
		log_node->data = (char*)realloc(
						log_node->data, 
						log_node->data_size);
		if (!log_node->data) {
			TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
			return TC_ERR;
		}
		memset(
			log_node->data + log_node->data_pos, 
			0, 
			log_node->data_size - log_node->data_pos);
	}

	return TC_OK;
}

static int
tc_log_string_write(
	struct tc_log_node *log_node
)
{
	FILE *fp = NULL;

	if (!log_node->file) 
		fprintf(stderr, "%s", log_node->data);
	else {
		fp = fopen(log_node->file, "w+");
		if (!fp) {
			TC_ERRNO_SET(TC_OPEN_FILE_ERR);
			return TC_ERR;
		}
		fwrite(log_node->data, sizeof(char), log_node->data_pos, fp);
		fclose(fp);
	} 

	return TC_OK;
}

int
tc_log_write(
	char *name,
	char *fmt,
	...
)
{
	int ret = 0;
	va_list arg_ptr;
	struct hlist_node *hnode = NULL;
	struct tc_log_node *log_node;

	hnode = tc_hash_get(
			global_log_data.log_hash, 
			(unsigned long)name, 
			(unsigned long)name);
	if (!hnode) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	log_node = tc_list_entry(hnode, struct tc_log_node, node);

	va_start(arg_ptr, fmt);		
	ret = tc_log_string_set(log_node, fmt, arg_ptr);
	va_end(arg_ptr);
	if (ret != TC_OK)
		return ret;

	if (log_node->direct) 
		return tc_log_string_write(log_node);
	else
		log_node->data_pos += strlen(log_node->data);

	return TC_OK;
}

int
tc_log_end(
	char *name
)
{
	struct hlist_node *hnode = NULL;
	struct tc_log_node *log_node = NULL;

	hnode = tc_hash_get(
			global_log_data.log_hash, 
			(unsigned long)name, 
			(unsigned long)name);
	if (!hnode){
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	log_node = tc_list_entry(hnode, struct tc_log_node, node);	

	return tc_log_string_write(log_node);
}

static int
tc_log_uninit()
{
	return tc_hash_destroy(global_log_data.log_hash);
}

int
tc_log_init()
{
	global_log_data.log_hash = tc_hash_create(
						TC_LOG_HASH_SIZE,
						tc_log_hash, 
						tc_log_hash_get, 
						tc_log_hash_destroy);
	if (global_log_data.log_hash == TC_HASH_ERR)
		TC_PANIC("create log hash error\n");

	return tc_uninit_register(tc_log_uninit);
}

