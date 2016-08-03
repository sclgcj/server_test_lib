#include "tc_recv_private.h"
#include "tc_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_thread.h"
#include "tc_config.h"
#include "tc_handle_private.h"
#include "tc_epoll_private.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"

struct tc_recv_data {
	int thread_num;
	int thread_stack;
	int recv_thread_id;
	int (*handle_func)(struct list_head *node);
};

static struct tc_recv_data global_recv_data;

int 
tc_recv_node_add(
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

	return tc_thread_pool_node_add(global_recv_data.recv_thread_id, &recv_node->node);
}

static int
tc_recv_unix_tcp_accept(
	struct tc_create_link_data *cl_data 
)
{
	return TC_OK;
}

static int
tc_recv_unix_udp_accept( 
	struct tc_recv_node *recv_node,
	struct tc_create_link_data *cl_data 
)
{
	return TC_OK;
}

static int
tc_recv_udp_accept(
	struct tc_recv_node		*recv_node,
	struct sockaddr_in		*in_addr,
	struct tc_create_link_data	*cl_data 
)
{
	int ret = TC_OK;
	int event = 0;
	unsigned long user_data = 0;
	struct tc_create_link_data *epoll_data = NULL;

	if (cl_data->epoll_oper->recv_data)
		ret = cl_data->epoll_oper->recv_data(
					cl_data->private_link_data.sock, 
					cl_data->user_data,
					in_addr);
	if (ret != TC_OK) 
		return ret;

	if (cl_data->epoll_oper->accept_func)
		ret = cl_data->epoll_oper->accept_func(
					(unsigned long)cl_data,
					in_addr, 
					&event, 
					&user_data);
	if (ret != TC_OK)
		return ret;

	epoll_data = tc_create_link_data_alloc(
					cl_data->private_link_data.sock,
					NULL,
					user_data,
					cl_data->link_data.local_addr,
					in_addr->sin_addr,
					cl_data->link_data.local_port,
					ntohs(in_addr->sin_port));
	if (!epoll_data) 
		TC_PANIC("no enough memory\n");
	
	return tc_sock_event_add(cl_data->private_link_data.sock, event, cl_data);
}

static int 
tc_recv_tcp_accept(
	struct tc_create_link_data *cl_data
)
{
	int sock = 0, ret = 0;
	int addr_size = 0, event = 0;
	unsigned long user_data = 0;
	struct sockaddr_in addr;
	struct tc_create_link_data *epoll_data = NULL;

	sock = accept(cl_data->private_link_data.sock, (struct sockaddr*)&addr, &addr_size);
	if (sock < 0) 
		TC_PANIC("accept error: %s\n", strerror(errno));

	if (cl_data->epoll_oper->accept_func) {
		ret = cl_data->epoll_oper->accept_func(
						(unsigned long)cl_data, 
						&addr, 
						&event, 
						&user_data);
		if (ret != TC_OK) {
			close(sock);
			return ret;
		}
	}

	epoll_data = tc_create_link_data_alloc(
					sock, 
					NULL,
					user_data,
					cl_data->link_data.local_addr,
					addr.sin_addr,
					cl_data->link_data.local_port,
					ntohs(addr.sin_port));
	if (!cl_data) 
		TC_PANIC("no enough memory\n");

	tc_epoll_data_mod(
			cl_data->private_link_data.sock, 
			TC_EVENT_READ,
			(unsigned long )cl_data);
	return tc_sock_event_add(sock, event, epoll_data);
}

static int
tc_recv(
	struct list_head *list_node
)
{
	int ret = 0, event = 0;
	int tcp_server = 0, udp_server = 0;
	struct sockaddr_in in_addr;
	struct sockaddr_un un_addr;
	struct tc_recv_node *recv_node = NULL;
	struct tc_create_link_data *cl_data = NULL;

	recv_node = tc_list_entry(list_node, struct tc_recv_node, node);
	cl_data = (struct tc_create_link_data *)recv_node->user_data;
	if (!cl_data)
		return TC_OK;

	memset(&in_addr, 0, sizeof(in_addr));
	in_addr.sin_family = AF_INET;
	in_addr.sin_addr.s_addr = cl_data->link_data.peer_addr.s_addr;
	in_addr.sin_port = htons(cl_data->link_data.peer_port);
	memset(&un_addr, 0, sizeof(un_addr));
	un_addr.sun_family = AF_UNIX;
	switch (cl_data->private_link_data.link_type) {
	case TC_LINK_TCP_CLIENT:
	case TC_LINK_UDP_CLIENT:
		if (cl_data->epoll_oper->recv_data)
			ret = cl_data->epoll_oper->recv_data(
						cl_data->private_link_data.sock, 
						cl_data->user_data,
						&in_addr);
		event = TC_EVENT_READ;
		break;	
	case TC_LINK_TRASVERSAL_TCP_SERVER:
	case TC_LINK_TCP_SERVER:
		ret = tc_recv_tcp_accept(cl_data);
		tcp_server = 1;
		event = TC_EVENT_READ;
		break;
	case TC_LINK_UNIX_TCP_SERVER:
		ret = tc_recv_unix_tcp_accept(cl_data);
		tcp_server = 1;
		event = TC_EVENT_READ;
		break;
	case TC_LINK_TRASVERSAL_UDP_SERVER:
	case TC_LINK_UDP_SERVER:
		ret = tc_recv_udp_accept(recv_node, &in_addr, cl_data);
		udp_server = 1;
		event = TC_EVENT_READ;
		break;
	case TC_LINK_UNIX_UDP_SERVER:
		ret = tc_recv_unix_udp_accept(recv_node, cl_data);
		udp_server = 1;
		event = TC_EVENT_READ;
		break;
	case TC_LINK_TRASVERSAL_TCP_CLIENT:
	case TC_LINK_TRASVERSAL_UDP_CLIENT:
	case TC_LINK_UNIX_TCP_CLIENT:
	case TC_LINK_UNIX_UDP_CLIENT:
		//memcpy(un_addr.sun_path, cl_data->rc_node)
		if (cl_data->epoll_oper->transfer_recv)
			ret = cl_data->epoll_oper->transfer_recv(
						cl_data->private_link_data.sock,
						cl_data->user_data,
						(struct sockaddr*)&un_addr);
		event = TC_EVENT_READ;
		break;
	default:
		break;
	}

	tc_epoll_data_mod(cl_data->private_link_data.sock, event, (unsigned long)cl_data);
	if (ret != TC_OK && ret != TC_PEER_CLOSED && ret != TC_WOULDBLOCK) 
		goto err;

	if (ret == TC_PEER_CLOSED) {
		ret = tc_create_check_duration();
		goto err;
	}
	
	if (tcp_server != 1 && ret != TC_WOULDBLOCK) {
		tc_handle_node_add(&recv_node->node);
		return TC_OK;
	}
	ret = TC_OK;

err:
	if (ret != TC_OK && cl_data->epoll_oper->err_handle)
		cl_data->epoll_oper->err_handle(
					ret, 
					cl_data->user_data,
					&cl_data->link_data);
	TC_FREE(recv_node);
	return ret;
}

static int
tc_recv_create()
{
	if (global_recv_data.thread_num <= 1) {
		return tc_thread_pool_create(
				1,
				global_recv_data.thread_stack,
				"recv_data",
				NULL,
				tc_recv,
				NULL,
				&global_recv_data.recv_thread_id);
	}
	else
		return tc_thread_pool_create(
				global_recv_data.thread_num,
				global_recv_data.thread_stack,
				"recv_data", 
				NULL, 
				NULL,
				tc_recv,
				&global_recv_data.recv_thread_id);
	
}

static int
tc_recv_config_setup()
{
	int ret = 0;	

	global_recv_data.thread_num = TC_THREAD_DEFAULT_NUM;
	global_recv_data.thread_stack = TC_THREAD_DEFALUT_STACK;

	TC_CONFIG_ADD("recv_thread_num", &global_recv_data.thread_num, FUNC_NAME(INT));
	TC_CONFIG_ADD("recv_stack_size", &global_recv_data.thread_stack, FUNC_NAME(INT));

	return TC_OK;
}

int
tc_recv_init()
{
	int ret = 0;

	memset(&global_recv_data, 0, sizeof(global_recv_data));

	ret = tc_user_cmd_add(tc_recv_config_setup);
	if (ret != TC_OK)
		return ret;

	return tc_init_register(tc_recv_create);
}

TC_MOD_INIT(tc_recv_init);
