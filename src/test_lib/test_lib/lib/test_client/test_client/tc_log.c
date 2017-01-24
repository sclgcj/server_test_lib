#include "tc_std_comm.h"
#include "tc_log.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_init_private.h"
#include "tc_log_private.h"
#include "tc_config_read.h"
#include "tc_print.h"
#include <stdarg.h>

#define TC_LOG_HASH_SIZE 26
#define TC_DEFAULT_LOG_SIZE 512

struct tc_log_data {
	tc_hash_handle_t log_hash;
};


struct tc_log_node {
//	char *name;
	char *data;
	char *file;
	int  id;
	int  direct;
	int  data_pos;
	int  data_size;
	pthread_mutex_t mutex;
	struct hlist_node node;
};

//static struct tc_log_data global_log_data;

static int
tc_log_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	int name = 0;
	struct tc_log_node *log_node;

	if (!hnode) 
		name = (int)user_data;
	else if (hnode) {
		log_node = tc_list_entry(hnode, struct tc_log_node, node);
		name = log_node->id;
	}
	
	return (name);
}

static int
tc_log_hash_get(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	struct tc_log_node *log_node = NULL;

	log_node = tc_list_entry(hnode, struct tc_log_node, node);
	if ((int)user_data != log_node->id)
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
	TC_FREE(log_node->data);
	TC_FREE(log_node);

	return TC_OK;
}

struct tc_log_node *
tc_log_node_create(
	int id,
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
	if (file && file[0]) {
		len = strlen(file);
		log_node->file = (char*)calloc(len + 1, sizeof(char));
		if (!log_node->file) {
			TC_FREE(log_node);
			TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
			return NULL;
		}
		memcpy(log_node->file, file, len);
	}
	log_node->id	= id;
	log_node->data_pos = 0;
	log_node->data_size = TC_DEFAULT_LOG_SIZE;
	pthread_mutex_init(&log_node->mutex, NULL);
	log_node->data = (char*)calloc(log_node->data_size, sizeof(char));
	if (!log_node->data) {
		TC_FREE(log_node->file);
		TC_FREE(log_node);
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}

	return log_node;
}

void
tc_log_data_address(
	tc_log_data_t data
)
{
	struct tc_log_data *ldata = NULL;

	ldata = (struct tc_log_data*)data;
}

int
tc_log_data_start(
	tc_log_data_t data,
	int  id,
	char *file
)
{
	int len = 0;
	struct hlist_node *hnode = NULL;
	struct tc_log_node *log_node = NULL;
	struct tc_log_data *log_data = NULL;

	if (!data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	log_data = (struct tc_log_data *)data;
	hnode = tc_hash_get(
			log_data->log_hash, 
			(unsigned long)id, 
			(unsigned long)id);
	if (hnode) {
		log_node = tc_list_entry(hnode, struct tc_log_node, node);
		log_node->data_pos = 0;
		memset(log_node->data, 0, log_node->data_size);
	}
	else 
		log_node = tc_log_node_create(id, file);

	return tc_hash_add(log_data->log_hash, &log_node->node, id);
}

static int
tc_log_data_string_set(
	struct tc_log_node	*log_node, 
	char			*fmt,
	va_list			arg_ptr
)
{
	int ret = 0;
	int vs_ret = 0;
	va_list save;

	pthread_mutex_lock(&log_node->mutex);
	while (1) {
		va_copy(save, arg_ptr);
		vs_ret += vsnprintf(
				log_node->data + log_node->data_pos , 
				log_node->data_size - log_node->data_pos, 
				fmt, 
				save);
		va_end(save);
		ret = strlen(log_node->data);
		if (ret + 1 < log_node->data_size) 
			break;
		//log_node->data_pos += vs_ret;
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
	pthread_mutex_unlock(&log_node->mutex);

	return TC_OK;
}

static int
tc_log_data_string_write(
	struct tc_log_node *log_node
)
{
	FILE *fp = NULL;

	pthread_mutex_lock(&log_node->mutex);
	if (!log_node->file) 
		fprintf(stderr, "%s", log_node->data);
	else {
		fp = fopen(log_node->file, "w+");
		if (!fp) {
			pthread_mutex_unlock(&log_node->mutex);
			TC_ERRNO_SET(TC_OPEN_FILE_ERR);
			return TC_ERR;
		}
		fwrite(log_node->data, sizeof(char), log_node->data_pos, fp);
		fclose(fp);
	} 
	pthread_mutex_unlock(&log_node->mutex);

	return TC_OK;
}

int
tc_log_data_write(
	tc_log_data_t	data,
	int		id,
	char		*fmt,
	va_list		arg_ptr
)
{
	int ret = 0;
	struct hlist_node  *hnode = NULL;
	struct tc_log_node *log_node;
	struct tc_log_data *log_data;

	if (!data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	log_data = (struct tc_log_data *)data;
	hnode = tc_hash_get(
			log_data->log_hash, 
			(unsigned long)id, 
			(unsigned long)id);
	if (!hnode) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	log_node = tc_list_entry(hnode, struct tc_log_node, node);

	ret = tc_log_data_string_set(log_node, fmt, arg_ptr);
	if (ret != TC_OK)
		return ret;

	log_node->data_pos = strlen(log_node->data);
	//return tc_log_data_string_write(log_node);

	return TC_OK;
}

int
tc_log_data_end(
	tc_log_data_t data,
	int id
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;
	struct tc_log_node *log_node = NULL;
	struct tc_log_data *log_data = NULL;

	log_data = (struct tc_log_data *)data;
	if (!log_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	hnode = tc_hash_get(
			log_data->log_hash, 
			(unsigned long)id, 
			(unsigned long)id);
	if (!hnode){
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	log_node = tc_list_entry(hnode, struct tc_log_node, node);	

	ret = tc_log_data_string_write(log_node);

	memset(log_node->data, 0, log_node->data_pos);
	log_node->data_pos = 0;

	return ret;
}

int
tc_log_data_destroy(
	tc_log_data_t data
)
{
	struct tc_log_data *log_data = (struct tc_log_data*)data;

	return tc_hash_destroy(log_data->log_hash);
}

static int
tc_log_data_init(
	int total_link,
	struct tc_log_data *data
)
{
	data->log_hash = tc_hash_create(
					total_link,
					tc_log_hash, 
					tc_log_hash_get, 
					tc_log_hash_destroy);
	if (data->log_hash == TC_HASH_ERR)
		TC_PANIC("create log hash error\n");

	return TC_OK;
	//return tc_uninit_register(tc_log_uninit);
}

tc_log_data_t
tc_log_data_create(
	int total_link	
)
{
	struct tc_log_data *log_data = NULL;

	log_data = (struct tc_log_data *)calloc(1, sizeof(struct tc_log_data));
	if (!log_data) {
		TC_PANIC("not enough memroy for %d bytes: %s\n", 
				sizeof(struct tc_log_data));
	}
	
	tc_log_data_init(total_link, log_data);
	return (tc_log_data_t)log_data;
}

