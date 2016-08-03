#include "tc_handle_private.h"
#include "tc_init.h"
#include "tc_cmd.h"
#include "tc_err.h"
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
	struct list_head *list_node
)
{
	return tc_thread_pool_node_add(
				global_handle_data.handle_thread_id, 
				list_node);
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

	memset(&addr, 0, sizeof(addr));
	memset(&un_addr, 0, sizeof(un_addr));
	switch (cl_data->private_link_data.link_type) {	
	case TC_LINK_TCP_CLIENT:
	case TC_LINK_UDP_CLIENT:
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = cl_data->link_data.peer_addr.s_addr;
		addr.sin_port = htons(cl_data->link_data.peer_port);
		if (cl_data->epoll_oper->handle_data) {
			ret = cl_data->epoll_oper->handle_data(
						cl_data->private_link_data.sock,
						cl_data->user_data, 
						&cl_data->link_data,
						&addr);
		}
		break;
	case TC_LINK_UNIX_TCP_CLIENT:	
	case TC_LINK_UNIX_UDP_CLIENT:
	case TC_LINK_TRASVERSAL_TCP_CLIENT:
	case TC_LINK_TRASVERSAL_UDP_CLIENT:
		un_addr.sun_family = AF_UNIX;
		if (cl_data->link_data.unix_path)
			memcpy(
				un_addr.sun_path, 
				cl_data->link_data.unix_path, 
				strlen(cl_data->link_data.unix_path));
		if (cl_data->epoll_oper->transfer_handle) 
			ret = cl_data->epoll_oper->transfer_handle(
						cl_data->private_link_data.sock,
						cl_data->user_data,
						&cl_data->link_data,
						(struct sockaddr*)&un_addr);
	}
	if (ret != TC_OK && cl_data->epoll_oper->err_handle)
		cl_data->epoll_oper->err_handle(
						ret, 
						cl_data->user_data, 
						&cl_data->link_data);

	TC_FREE(recv_node);
	return TC_OK;
}

static int
tc_handle_create()
{
	if (global_handle_data.thread_num <= 0) 
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
