#include "tc_interface.h"
#include "tc_interface_private.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init.h"
#include "tc_comm.h"
#include "tc_json.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_thread.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"
#include "tc_rendezvous_private.h"

struct tc_interface_node {
	char *group_name;
	char *interface_name;
	cJSON *json_param;
	struct tc_interface_oper oper;
	struct tc_interface_node *next;
};

struct tc_interface_head {
	int circle_run;
	struct tc_interface_node *cur;
	struct tc_interface_node *last;
	struct tc_interface_node *start;
};

struct tc_curl_param {
	struct tc_interface_node *interface_node;
	void *user_data;
	struct list_head node;
};

struct tc_interface_data {
	int circle_run;
	int total_link;
	int recv_timeout;
	int connect_timeout;
	int open_rendezvous;
	int thread_id;
	unsigned short http_server_port;
	unsigned short res;
	char http_type[8];
	char http_server_ip[16];
	tc_rendezvous_t rend;
	pthread_mutex_t handle_mutex;
	pthread_cond_t handle_cond;
	struct tc_interface_node *cur_config;
	struct tc_interface_head interface_head;
};

static struct tc_interface_data global_interface_data;

static void
tc_mobile_curlopt_default_set(
	char *url,
	char *param,
	int  param_size,
	struct tc_curl_param *curl_param,
	CURL *curl
);

int
tc_interface_register(
	char *group_name,
	char *api_name,
	struct tc_interface_oper *oper
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
	memcpy(&interface_node->oper, oper, sizeof(*oper));
	if (group_name)
		interface_node->group_name = strdup(group_name);
	if (api_name)
		interface_node->interface_name = strdup(api_name);

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
	struct tc_interface_node *interface = NULL;
	struct tc_create_link_data *cl_data = NULL;

	curl_param = (struct tc_curl_param *)user_data;
	cl_data = (struct tc_create_link_data *)curl_param->user_data; 
	interface = curl_param->interface_node;

	pthread_mutex_lock(&cl_data->data_mutex);
	if (cl_data->first_recv == 0) {
		cl_data->first_recv = 1;
		if (interface->oper.first_recv) {
			ret = interface->oper.first_recv(interface->interface_name, 
							 cl_data->user_data);
			if (ret != TC_OK) {
				pthread_mutex_unlock(&cl_data->data_mutex);
				goto out;
			}
		}
	}
	pthread_mutex_unlock(&cl_data->data_mutex);
	if (interface->oper.interface_recv) {
		ret = interface->oper.interface_recv(
						interface->interface_name, 
						ptr, 
						size,
						nmemb,
						cl_data->user_data);
		if (ret != TC_OK)
			goto out;
	}
	tc_thread_pool_node_add(global_interface_data.thread_id, &curl_param->node);

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

static int
tc_mobile_data_send(
	char *url,
	char *param,
	int  param_size,
	struct tc_interface_node *interface_node,
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	CURL *curl = NULL;
	CURLcode curl_code;
	struct tc_curl_param *curl_param = NULL;

	curl = curl_easy_init();
	if (!curl) {
		TC_ERRNO_SET(TC_CURL_INIT_ERR);
		return TC_ERR;
	}

	curl_param = (struct tc_curl_param *)calloc(1, sizeof(*curl_param));
	if (!curl_param) 
		TC_PANIC("not enough memory for %d bytes\n", sizeof(*curl_param));
	curl_param->user_data  = (void*)cl_data;
	curl_param->interface_node = interface_node;
	tc_mobile_curlopt_default_set(
				url, param, param_size, 
				curl_param, curl);

	//可能需要其他的curl选项，找时间弄一个
	if (interface_node->oper.curlopt_set) {
		ret = interface_node->oper.curlopt_set(
						interface_node->interface_name, 
						curl, cl_data->user_data);
		if (ret != TC_OK)
			goto out;
	}
	if (global_interface_data.open_rendezvous)
		tc_rendezvous_set(global_interface_data.rend);

	if (cl_data->epoll_oper->interface_before_send)
		cl_data->epoll_oper->interface_before_send(cl_data->user_data);

	curl_code = curl_easy_perform(curl);
	//wait all thread receiving over, and send the disposition signal to the 
	//disposition thread
	if (global_interface_data.open_rendezvous)
		tc_rendezvous_set(global_interface_data.rend);
	pthread_cond_signal(&global_interface_data.handle_cond);
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
	//wait for the handle completion
	pthread_mutex_lock(&cl_data->interface_mutex);
	pthread_cond_wait(&cl_data->interface_cond, 
			  &cl_data->interface_mutex);
	pthread_mutex_unlock(&cl_data->interface_mutex);
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

int
tc_interface_func_execute(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	int len = 0, url_len = 0;
	char *url = NULL;
	char *param = NULL, *tmp = NULL;
	struct tc_interface_node *interface_node = NULL;
	
	interface_node = global_interface_data.interface_head.cur;
	while (interface_node) {
		if (interface_node->oper.interface_param) {
			ret = interface_node->oper.interface_param(
							interface_node->interface_name, 
							cl_data->user_data, 
							(unsigned long*)&param);
			if (ret != TC_OK)
				goto next;
		}
		if (!param)
			len = 0;
		else
			len = strlen(param);
		url_len = strlen(interface_node->group_name) + 
			  strlen(interface_node->interface_name) + 16 + 32;
		url = (char*)calloc(url_len, sizeof(char));
		snprintf(url, url_len, "%s://%s:%d/%s/%s", 
				global_interface_data.http_type, 
				global_interface_data.http_server_ip, 
				global_interface_data.http_server_port, 
				interface_node->group_name, 
				interface_node->interface_name);
		tc_mobile_data_send(url, param, len, interface_node, cl_data);

next:
		len = 0;
		url_len = 0;
		TC_FREE(param);
		TC_FREE(url);
		interface_node = interface_node->next;
	}

	return TC_OK;
}

char *
tc_interface_url_encode(
	char *data
)
{
	int len = 0;
	char *tmp= NULL, *url_encode = NULL;
	CURL *curl = NULL;

	if (!data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return NULL;
	}

	curl = curl_easy_init();
	if (!curl) {
		TC_ERRNO_SET(TC_CURL_INIT_ERR);
		return NULL;
	}
	url_encode = curl_easy_escape(curl, data, 0);
	len = strlen(url_encode);
	tmp = (char*)calloc(len + 1, sizeof(char));
	if (!tmp)
		TC_PANIC("not enough memory for %d bytes\n", len + 1);
	memcpy(tmp, url_encode, len);

	curl_easy_cleanup(curl);
	curl_free(url_encode);

	return tmp;
}

int
tc_interface_cmd_add(
	
	unsigned long user_data
)
{
	struct tc_create_link_data *lc_data = NULL;

	lc_data = tc_create_link_data_get(user_data);
}

static void 
tc_interface_curl_param_destroy(
	struct list_head *list_node
)
{
	struct tc_curl_param *param = NULL;

	param = tc_list_entry(list_node, struct tc_curl_param, node);
	TC_FREE(param);
}

static int
tc_interface_curl_handle(
	struct list_head *list_node
)
{
	int ret = 0;
	struct tc_curl_param *param = NULL;
	struct tc_interface_node *interface = NULL;
	struct tc_create_link_data *cl_data = NULL;

	pthread_mutex_lock(&global_interface_data.handle_mutex);
	pthread_cond_wait(&global_interface_data.handle_cond, 
			  &global_interface_data.handle_mutex);
	pthread_mutex_unlock(&global_interface_data.handle_mutex);

	param = tc_list_entry(list_node, struct tc_curl_param, node);
	cl_data = (struct tc_create_link_data*)param->user_data;
	interface = param->interface_node;
	
	if (interface->oper.interface_check) {
		ret = interface->oper.interface_check(interface->interface_name, 
						      cl_data->user_data);
		if (ret != TC_OK) 
			if (cl_data->epoll_oper->err_handle){
				ret = cl_data->epoll_oper->err_handle(ret, cl_data->user_data);
				cl_data->private_link_data.err_flag = ret;
				tc_create_link_err_handle(cl_data);
			}
	}

	pthread_cond_signal(&cl_data->interface_cond);

//	if (param->interface_node->interface_func)
//		param->interface_node->interface_func(cl_data->user_data);

out:
	return TC_OK;
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

	return tc_thread_pool_create(
				TC_THREAD_DEFAULT_NUM, 
				TC_THREAD_DEFALUT_STACK, 
				"interface_handle", 
				tc_interface_curl_param_destroy, 
				tc_interface_curl_handle, 
				NULL,
				&global_interface_data.thread_id);
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
	TC_CONFIG_ADD(
		"http_server_port", 
		&global_interface_data.http_server_port, 
		FUNC_NAME(USHORT));
	TC_CONFIG_ADD(
		"http_server_ip", 
		global_interface_data.http_server_ip, 
		FUNC_NAME(STR));
	TC_CONFIG_ADD(
		"http_type",
		global_interface_data.http_type, 
		FUNC_NAME(STR));

	return TC_OK;
}

static int
tc_interface_destroy()
{
	struct tc_interface_node *node = NULL, *save = NULL;

	node = global_interface_data.interface_head.start;
	while (node) {
		save = node->next;
		TC_FREE(node->group_name);
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

	pthread_cond_init(&global_interface_data.handle_cond, NULL);
	pthread_mutex_init(&global_interface_data.handle_mutex, NULL);

	ret = tc_user_cmd_add(tc_interface_config_setup);
	if (ret != TC_OK)
		return ret;

	ret = tc_init_register(tc_interface_create);
	if (ret != TC_OK)
		return ret;

	return tc_uninit_register(tc_interface_destroy);
}

TC_MOD_INIT(tc_interface_init);
