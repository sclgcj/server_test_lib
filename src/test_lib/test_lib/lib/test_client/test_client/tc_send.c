#include "tc_send_private.h"
#include "tc_comm.h"
#include "tc_init.h"
#include "tc_cmd.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_thread.h"
#include "tc_recv_private.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"

struct tc_send_data {
	int thread_num;
	int thread_stack;
	int send_thread_id;
};

static struct tc_send_data global_send_data;

int
tc_send_node_add(
	unsigned long epoll_data
)
{
	struct tc_recv_node *recv_node = NULL;

	PRINT("\n");
	recv_node = (struct tc_recv_node *)calloc(1, sizeof(*recv_node));
	if (!recv_node) 
		TC_PANIC("not enough memory\n");

	recv_node->user_data = epoll_data;

	return tc_thread_pool_node_add(global_send_data.send_thread_id, &recv_node->node);
}

static int
tc_send_connect_handle(
	struct tc_create_link_data *cl_data
)
{
	int ret = TC_OK;
	int sock = cl_data->private_link_data.sock;
	int event = 0;
	int result = 0;	
	socklen_t result_len = sizeof(result);
	struct sockaddr_in addr;

	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) 
		return TC_ERR;

	if (result != 0) {
		tc_epoll_data_mod(sock, EPOLLONESHOT | EPOLLOUT, (unsigned long)cl_data);
		return TC_ERR;
	}

	memset(&addr, 0, sizeof(addr));
	if (cl_data->epoll_oper->connected_func) {
		/*
		 * just do inet first, others later, because of time limit
		 */
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = cl_data->link_data.peer_addr.s_addr;
		addr.sin_port = htons(cl_data->link_data.peer_port);
		ret = cl_data->epoll_oper->connected_func(
							cl_data->user_data, 
							&event,
							&addr);
		tc_epoll_data_mod(sock, event, (unsigned long)cl_data);
	}
	cl_data->private_link_data.status = TC_STATUS_SEND_DATA;

	return ret;
}

static int
tc_send(
	struct list_head *list_node
)
{
	int ret = 0;
	struct sockaddr_in in_addr;
	struct sockaddr_un un_addr;
	struct tc_recv_node *send_node = NULL;
	struct tc_create_link_data *cl_data = NULL;

	send_node = tc_list_entry(list_node, struct tc_recv_node, node);
	cl_data = (struct tc_create_link_data*)send_node->user_data;
	
	PRINT("\n");
	if (cl_data->private_link_data.status == TC_STATUS_CONNECT)	{
		tc_send_connect_handle(cl_data);
		goto out;
	}

	PRINT("\n");
	memset(&in_addr, 0, sizeof(in_addr));
	memset(&un_addr, 0, sizeof(un_addr));
	switch (cl_data->private_link_data.link_type) {
	case TC_LINK_TCP_CLIENT:
	case TC_LINK_UDP_CLIENT:
	case TC_LINK_TCP_SERVER:
	case TC_LINK_UDP_SERVER:
		in_addr.sin_family = AF_INET;
		in_addr.sin_addr.s_addr = cl_data->link_data.peer_addr.s_addr;
		in_addr.sin_port = htons(cl_data->link_data.peer_port);
		if (cl_data->epoll_oper->send_data)  {
			ret = cl_data->epoll_oper->send_data(
						cl_data->private_link_data.sock, 
						cl_data->user_data, 
						&in_addr);
			if (ret == TC_WOULDBLOCK) {
				tc_epoll_data_mod(
						cl_data->private_link_data.sock, 
						TC_EVENT_WRITE, 
						(unsigned long)cl_data);
			}
		}
		break;
	default:
		/*
		 * just do it later
		 */
		break;
	}
out:
	TC_FREE(send_node);
	return TC_OK;
}

static int
tc_send_create()
{
	if (global_send_data.thread_num <= 1) 
		return tc_thread_pool_create(
					1, 
					global_send_data.thread_stack,
					"send_data", 
					NULL,
					tc_send, 
					NULL, 
					&global_send_data.send_thread_id);
	else
		return tc_thread_pool_create(
					global_send_data.thread_num,
					global_send_data.thread_stack,
					"send_data",
					NULL, 
					NULL,
					tc_send, 
					&global_send_data.send_thread_id);
}

int
tc_send_config_setup()
{
	global_send_data.thread_num   = TC_THREAD_DEFAULT_NUM;
	global_send_data.thread_stack = TC_THREAD_DEFALUT_STACK;	

	TC_CONFIG_ADD("send_thread_num", &global_send_data.thread_num, FUNC_NAME(INT));
	TC_CONFIG_ADD("send_stack_size", &global_send_data.thread_stack, FUNC_NAME(INT));

	return TC_OK;
}

int
tc_send_init()
{
	int ret = 0;

	memset(&global_send_data, 0, sizeof(global_send_data));
	ret = tc_user_cmd_add(tc_send_config_setup);
	if (ret != TC_OK)
		return ret;

	return tc_init_register(tc_send_create);
}

TC_MOD_INIT(tc_send_init);
