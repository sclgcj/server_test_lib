#include "tc_interface.h"
#include "tc_interface_private.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init.h"
#include "tc_comm.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"
#include "tc_rendezvous_private.h"

struct tc_interface_node {
	char *interface_name;
	cJSON *json_param;
	int (*interface_func)(unsigned long user_data);
	struct tc_interface_node *next;
};

struct tc_interface_head {
	int circle_run;
	struct tc_interface_node *cur;
	struct tc_interface_node *last;
	struct tc_interface_node *start;
};

struct tc_curl_param {
	int (*write_callback)(char *ptr, size_t size, size_t nmemb, void *user_data);
	void *user_data;
};

struct tc_interface_data {
	int circle_run;
	int total_link;
	int recv_timeout;
	int connect_timeout;
	int open_rendezvous;
	tc_rendezvous_t rend;
	struct tc_interface_node *cur_config;
	struct tc_interface_head interface_head;
};

static struct tc_interface_data global_interface_data;

int
tc_interface_register(
	char *interface_name,
	int (*interface_func)(unsigned long user_data)
)
{
	int len = 0;
	struct tc_interface_head *interface_head = NULL;
	struct tc_interface_node *interface_node = NULL;	

	interface_node = (struct tc_interface_node*)
				calloc(1, sizeof(*interface_node));
	if (!interface_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	interface_node->interface_func = interface_func;
	if (interface_name) {
		len = strlen(interface_name);
		interface_node->interface_name = (char *)calloc(1, len + 1);
		if (!interface_node->interface_name) {
			TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
			TC_FREE(interface_node);
			return TC_ERR;
		}
		memcpy(interface_node->interface_name, interface_name, len);
	}

	interface_head = &global_interface_data.interface_head;
	if (!interface_head->cur) {
		interface_node->next = NULL;
		interface_head->cur  = interface_node;
		interface_head->last = interface_node;
		interface_head->start = interface_node;
	} else {
		interface_node->next = interface_head->last->next;
		interface_head->last->next = interface_node;
		interface_head->last = interface_node;
	}

	return TC_OK;
}

static struct tc_interface_node *
tc_interface_node_get(
	char *name
)
{
	struct tc_interface_node *node = NULL;

	node = global_interface_data.interface_head.start;
	while (node) {
		if (!strcmp(name, node->interface_name))
			return node;
		node = node->next;
	}

	return NULL;
}

void
tc_interface_param_set(
	char *name,
	char *val
)
{
	cJSON *json = NULL;
	struct tc_interface_node *node = NULL;

	node = tc_interface_node_get(name);
	if (node)  {
		global_interface_data.cur_config = node;
		node->json_param = cJSON_CreateObject();
		return;
	}

	if (global_interface_data.cur_config && global_interface_data.cur_config->json_param) {
		if (strchr(val, ',')) 
			FUNC_NAME(ARRAY)(0, name, val, (unsigned long)&json);
		else
			json = cJSON_CreateString(val);
		cJSON_AddItemToObject(
				global_interface_data.cur_config->json_param, 
				name, 
				json);
	}
}

cJSON *
tc_interface_param_get(
	char *interface
)
{
	struct tc_interface_node *node = NULL;

	node = tc_interface_node_get(interface);
	if (!node) 
		return NULL;
	
	return node->json_param;
}

void
tc_interface_func_execute(
	unsigned long user_data
)
{
	struct tc_interface_node *interface_node = NULL;
	
	if (!global_interface_data.interface_head.cur)
		return;

	if (!global_interface_data.interface_head.cur->interface_func)
		return;

	interface_node = global_interface_data.interface_head.cur;
	global_interface_data.interface_head.cur = interface_node->next;

	interface_node->interface_func(user_data);
}

int
tc_interface_url_encode(
	char *data,
	int  *encode_len,
	char *encode_str
)
{
	int len = 0;
	char *url_encode = NULL;
	CURL *curl = NULL;

	if (!data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	curl = curl_easy_init();
	if (!curl) {
		TC_ERRNO_SET(TC_CURL_INIT_ERR);
		return TC_ERR;
	}
	url_encode = curl_easy_escape(curl, data, 0);
	len = strlen(url_encode);
	if (len >= *encode_len) {
		TC_FREE(url_encode);
		curl_easy_cleanup(curl);
		(*encode_len) = len;
		return TC_ERR;
	}
	memcpy(encode_str, url_encode, len);
	
	TC_FREE(url_encode);
	curl_easy_cleanup(curl);
	return TC_OK;
}

static int
tc_curl_response_handle(
	char *ptr,
	size_t size,
	size_t nmemb,
	void   *user_data
)
{
	int ret = 0;
	struct tc_curl_param *curl_param = NULL;
	struct tc_create_link_data *cl_data = NULL;

	curl_param = (struct tc_curl_param *)user_data;
	cl_data = (struct tc_create_link_data *)curl_param->user_data;

	if (cl_data->epoll_oper->interface_recv) {
		ret = cl_data->epoll_oper->interface_recv(
						ptr, 
						size, 
						nmemb, 
						cl_data->user_data);
		if (ret != TC_OK) 
			goto out;
	} 

	ret = curl_param->write_callback(ptr, size, nmemb, (void*)cl_data->user_data);
	
	if (ret != TC_OK)
		return ret;

out:
	return nmemb * size;
}

static void
tc_mobile_curlopt_default_set(
	char *url,
	char *param,
	int  param_size,
	struct tc_curl_param *curl_param,
	CURL *curl
)
{
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)curl_param);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, tc_curl_response_handle);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, param_size);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, param);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, global_interface_data.connect_timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, global_interface_data.recv_timeout);
	curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0L);
	curl_easy_setopt(curl, CURLOPT_CAINFO, "");
	curl_easy_setopt(curl, CURLOPT_CAPATH, "");
}

int
tc_mobile_data_send(
	char *url,
	char *param,
	int  param_size,
	unsigned long user_data,
	void (*curlopt_set)(unsigned long user_data, CURL *curl),
	int (*write_callback)(char *ptr, size_t size, size_t nmemb, void *user_data)
)
{
	int ret = 0;
	CURL *curl = NULL;
	CURLcode curl_code;
	struct tc_curl_param curl_param;
	struct tc_create_link_data *cl_data = NULL;

	//cl_data = (struct tc_create_link_data *)extra_data;
	cl_data = tc_create_link_data_get(user_data);
	if (!cl_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	curl = curl_easy_init();
	if (!curl) {
		TC_ERRNO_SET(TC_CURL_INIT_ERR);
		return TC_ERR;
	}

	memset(&curl_param, 0, sizeof(curl_param));
	curl_param.write_callback = write_callback;
	curl_param.user_data  = (void*)cl_data;
	tc_mobile_curlopt_default_set(
				url, param, param_size, 
				&curl_param, curl);
	if (curlopt_set) 
		curlopt_set(user_data, curl);

	if (global_interface_data.open_rendezvous)
		tc_rendezvous_set(global_interface_data.rend);

	if (cl_data->epoll_oper->interface_before_send)
		cl_data->epoll_oper->interface_before_send(cl_data->user_data);

	curl_code = curl_easy_perform(curl);
	if (curl_code != CURLE_OK) {
		if (curl_code == CURLE_OPERATION_TIMEDOUT ) {
			PRINT("time out\n");
			ret = TC_TIMEOUT;
			goto out;	
		}
		PRINT("error = %d\n", curl_code);
		ret = TC_CURL_PERFORM_ERR;
		goto out;
	}
	ret = TC_OK;
out:
	if (ret != TC_OK)
		if (cl_data->epoll_oper->err_handle) {
			ret = cl_data->epoll_oper->err_handle(
						TC_TIMEOUT, 
						cl_data->user_data);
			cl_data->private_link_data.err_flag = ret;
			tc_create_link_err_handle(cl_data);
		}
	curl_easy_cleanup(curl);
	return ret;
}

static int
tc_interface_create()
{
	if (global_interface_data.open_rendezvous == 1)
		global_interface_data.rend = tc_rendezvous_create(
						global_interface_data.total_link, 
						"api_test");
	if (global_interface_data.rend == TC_RENDEVOUS_ERR) 
		return TC_ERR;

	return TC_OK;
}

static int
tc_interface_config_setup()
{
	TC_CONFIG_ADD(
		"recv_timeout", 
		&global_interface_data.recv_timeout, 
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"total_link",
		&global_interface_data.total_link,
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"connect_timeout", 
		&global_interface_data.connect_timeout, 
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"rendevous_enable", 
		&global_interface_data.open_rendezvous,
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"circle_run", 
		&global_interface_data.circle_run, 
		FUNC_NAME(INT));

	return TC_OK;
}

static int
tc_interface_destroy()
{
	struct tc_interface_node *node = NULL, *save = NULL;

	node = global_interface_data.interface_head.start;
	while (node) {
		save = node->next;
		TC_FREE(node->interface_name);
		if (node->json_param)
			cJSON_Delete(node->json_param);
		TC_FREE(node);
		node = save;	
	}

	global_interface_data.interface_head.start = NULL;

	return TC_OK;
}

int
tc_interface_init()
{
	int ret = 0;

	ret = tc_user_cmd_add(tc_interface_config_setup);
	if (ret != TC_OK)
		return ret;

	ret = tc_init_register(tc_interface_create);
	if (ret != TC_OK)
		return ret;

	return tc_uninit_register(tc_interface_destroy);
}

TC_MOD_INIT(tc_interface_init);
