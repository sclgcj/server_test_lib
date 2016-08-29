#include "tc_handle_private.h"
#include "tc_init.h"
#include "tc_cmd.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_thread.h"
#include "tc_recv_private.h"
#include "tc_create_private.h"

struct tc_handle_data {
	int thread_num;
	int thread_stack;
	int handle_thread_id;	
};

static struct tc_handle_data global_handle_data;

int 
tc_handle_node_add(
	unsigned long user_data
)
{
	struct tc_recv_node *recv_node = NULL;

	recv_node = (struct tc_recv_node*)calloc(1, sizeof(*recv_node));
	if (!recv_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	recv_node->user_data = user_data;

	return tc_thread_pool_node_add(global_handle_data.handle_thread_id, &recv_node->node);
}

static int
tc_handle(
	struct list_head *list_node
)
{
	int ret = 0;
	struct sockaddr_in addr;
	struct sockaddr_un un_addr;
	struct tc_recv_node *recv_node = NULL;
	struct tc_create_link_data *cl_data = NULL;

	recv_node = tc_list_entry(list_node, struct tc_recv_node, node);
	cl_data = (struct tc_create_link_data*)recv_node->user_data;

	if (!cl_data->proto_oper || !cl_data->proto_oper->proto_handle)
		goto out;
	ret = cl_data->proto_oper->proto_handle(cl_data);

	if (ret != TC_OK && cl_data->epoll_oper->err_handle) {
		ret = cl_data->epoll_oper->err_handle(
						tc_cur_errno_get(),
						cl_data->user_data);
		cl_data->private_link_data.err_flag = ret;
		tc_create_link_err_handle(cl_data);
	}

out:
	TC_FREE(recv_node);
	return TC_OK;
}

static void
tc_handle_free(
	struct list_head *list_node
)
{
	struct tc_recv_node *recv_node = NULL;	
	
	recv_node = tc_list_entry(list_node, struct tc_recv_node, node);
	TC_FREE(recv_node);
}

static int
tc_handle_create()
{
	if (global_handle_data.thread_num <= 1) 
		return tc_thread_pool_create(
				1, 
				global_handle_data.thread_stack,
				"handle_data", 
				NULL,  
				tc_handle, 
				NULL, 
				&global_handle_data.handle_thread_id);
	else 
		return tc_thread_pool_create(
				global_handle_data.thread_num, 
				global_handle_data.thread_stack,
				"handle_data", 
				NULL, 
				NULL, 
				tc_handle,
				&global_handle_data.handle_thread_id);
}

static int
tc_handle_config_setup()
{
	global_handle_data.thread_num = TC_THREAD_DEFAULT_NUM;
	global_handle_data.thread_stack = TC_THREAD_DEFALUT_STACK;

	TC_CONFIG_ADD(
		"handle_thread_num", 
		&global_handle_data.thread_num, 
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"handle_stack_size",
		&global_handle_data.thread_stack,
		FUNC_NAME(INT));
	return TC_OK;
}

int
tc_handle_init()
{
	int ret = 0;

	memset(&global_handle_data, 0, sizeof(global_handle_data));
	ret = tc_user_cmd_add(tc_handle_config_setup);
	if (ret != TC_OK)
		return ret;

	return tc_init_register(tc_handle_create);
}

TC_MOD_INIT(tc_handle_init);
