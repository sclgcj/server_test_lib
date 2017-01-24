#include "tc_send_private.h"
#include "tc_std_comm.h"
#include "tc_init_private.h"
#include "tc_cmd.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_thread.h"
#include "tc_recv_check.h"
#include "tc_recv_private.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"

struct tc_send_data {
	int thread_num;
	int thread_stack;
	int send_thread_id;
};

static struct tc_send_data global_send_data;

/*static int	
tc_data_send(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	struct list_head *sl = NULL;
	struct tc_io_data *io_data = NULL;
	struct tc_link_private_data *data = NULL;

	data = &cl_data->private_link_data;
	while (1) {
		pthread_mutex_lock(&data->send_mutex);
		if (list_empty(&data->send_list))
			ret = TC_ERR;
		else {
			io_data = tc_list_entry(data->send_list.next, struct tc_io_data, node);
			list_del_init(&io_data->node);
			ret = TC_OK;
		}
		pthread_mutex_unlock(&data->send_mutex);
		if (ret == TC_ERR) {
			ret = TC_OK;
			break;
		}
		//PRINT("data = %s, data_len = %d\n\n", io_data->data, io_data->data_len);
		ret = send(data->sock, io_data->data, io_data->data_len, 0);
		if (ret > 0) {
			TC_FREE(io_data->data);
			TC_FREE(io_data);
			ret = TC_OK;
			continue;
		} 
		else {
			PRINT("err = %s\n", strerror(errno));
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EPIPE) {
				ret = TC_WOULDBLOCK;
				break;
			}
			TC_FREE(io_data->data);
			TC_FREE(io_data);
			if (errno == ECONNRESET) {
				ret = TC_PEER_RESET;
				break;
			}
			ret = TC_SEND_ERR;
		}
		break;
	}

	return ret;
}*/

int
tc_send_node_add(
	unsigned long epoll_data
)
{
	struct tc_recv_node *recv_node = NULL;

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

	tc_recv_check_stop("connect", cl_data->user_data);
	memset(&addr, 0, sizeof(addr));
	if (cl_data->epoll_oper->connected_func) {
		/*
		 * just do inet first, others later, because of time limit
		 */
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = cl_data->link_data.peer_addr.s_addr;
		addr.sin_port = htons(cl_data->link_data.peer_port);
		ret = cl_data->epoll_oper->connected_func(
							cl_data->user_data);
		//PRINT("ret = %d\n", ret);
		//tc_epoll_data_mod(sock, event, (unsigned long)cl_data);
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
	struct tc_recv_node *send_node = NULL;
	struct tc_create_link_data *cl_data = NULL;

	send_node = tc_list_entry(list_node, struct tc_recv_node, node);
	cl_data = (struct tc_create_link_data*)send_node->user_data;
	
	if (cl_data->private_link_data.status == TC_STATUS_CONNECT)	{
		tc_send_connect_handle(cl_data);
		goto out;
	}
	if (!cl_data->proto_oper || !cl_data->proto_oper->proto_send)
		goto out;
	ret = cl_data->proto_oper->proto_send(cl_data);

	if (ret == TC_WOULDBLOCK) {
		tc_epoll_data_send_mod( cl_data->private_link_data.sock, 
					(unsigned long)cl_data);
		ret = TC_OK;
	}
	else if (ret == TC_OK)
		tc_epoll_data_recv_mod( cl_data->private_link_data.sock,
					(unsigned long)cl_data);

	if (ret != TC_OK && cl_data->epoll_oper->err_handle)  {
		ret = cl_data->epoll_oper->err_handle(
						ret, 
						cl_data->user_data);
		cl_data->private_link_data.err_flag = ret;
	}
		
out:
	TC_FREE(send_node);
	return TC_OK;
}

static void
tc_send_free(
	struct list_head *list_node
)
{
	struct tc_recv_node *send_node = NULL;

	send_node = tc_list_entry(list_node, struct tc_recv_node, node);
	TC_FREE(send_node);
}

static int
tc_send_create()
{
	if (global_send_data.thread_num <= 1) 
		return tc_thread_pool_create(
					1, 
					global_send_data.thread_stack,
					"send_data", 
					tc_send_free,
					tc_send, 
					NULL, 
					&global_send_data.send_thread_id);
	else
		return tc_thread_pool_create(
					global_send_data.thread_num,
					global_send_data.thread_stack,
					"send_data",
					tc_send_free,
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

	return tc_local_init_register(tc_send_create);
}

TC_MOD_INIT(tc_send_init);
