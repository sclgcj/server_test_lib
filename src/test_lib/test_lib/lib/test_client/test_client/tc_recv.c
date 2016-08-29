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

#if 0
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
	struct tc_create_data *create_data = NULL;
	struct tc_create_link_data *epoll_data = NULL;

	/*if (cl_data->epoll_oper->recv_data)
		ret = cl_data->epoll_oper->recv_data(
					cl_data->private_link_data.sock, 
					cl_data->user_data,
					in_addr);*/
	if (ret != TC_OK) 
		return ret;

	create_data = tc_create_data_calloc(
					NULL, 0, 0, 
					cl_data->link_data.local_addr, 
					in_addr->sin_addr,
					ntohs(in_addr->sin_port),
					cl_data->link_data.local_port,
					0);
	/*memset(&create_data, 0, sizeof(create_data));
	create_data.port = cl_data->link_data.local_port;
	create_data.addr = cl_data->link_data.local_addr;
	create_data.user_data = user_data;*/
	epoll_data = tc_create_link_data_alloc(
					cl_data->private_link_data.sock,
					NULL,
					in_addr->sin_addr,
					ntohs(in_addr->sin_port),
					create_data);
	if (!epoll_data) 
		TC_PANIC("no enough memory\n");
	

	if (cl_data->epoll_oper->accept_func)
		ret = cl_data->epoll_oper->udp_accept_func(
					(unsigned long)cl_data,
					in_addr, 
					&event, 
					user_data);
	if (ret != TC_OK) {
		TC_FREE(create_data);
		return ret;
	}

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
	struct tc_create_data *create_data = NULL;
	struct tc_create_link_data *epoll_data = NULL;

	sock = accept(cl_data->private_link_data.sock, (struct sockaddr*)&addr, &addr_size);
	if (sock < 0) 
		TC_PANIC("accept error: %s\n", strerror(errno));

	/*memset(&create_data, 0, sizeof(create_data));
	create_data.port = cl_data->link_data.local_port;
	create_data.addr = cl_data->link_data.local_addr;
	create_data.user_data = user_data;*/
	create_data = tc_create_data_calloc(
					0, 0, 0, 0, 
					cl_data->link_data.local_addr, 
					addr.sin_addr, 
					ntohs(addr.sin_port),
					cl_data->link_data.local_port, 
					0);
	if (!create_data) 
		TC_PANIC("Not enough memory");

	tc_create_user_data_get(create_data->link_id, &create_data->user_data);

	epoll_data = tc_create_link_data_alloc(
					sock, 
					NULL,
					addr.sin_addr,
					ntohs(addr.sin_port),
					create_data);
	if (!cl_data) 
		TC_PANIC("no enough memory\n");

	if (cl_data->epoll_oper->accept_func) {
		ret = cl_data->epoll_oper->accept_func(
						(unsigned long)cl_data, 
						&addr, 
						&event, 
						user_data);
		if (ret != TC_OK) {
			close(sock);
			return ret;
		}
	}

	tc_epoll_data_mod(
			cl_data->private_link_data.sock, 
			TC_EVENT_READ,
			(unsigned long )cl_data);
	return tc_sock_event_add(sock, event, epoll_data);
}

static int
tc_recv_err_handle(
	int ret,
	int errcode
)
{
	if (ret == 0) {
		if (errcode == ECONNRESET)
			return TC_PEER_RESET;
		if (errcode == ETIMEDOUT)
			return TC_WOULDBLOCK;
		return TC_PEER_CLOSED;
	} else {
		if (errcode == EAGAIN || errcode == EWOULDBLOCK || 
				errcode == ETIMEDOUT || errcode == EBADF)
			return TC_WOULDBLOCK;
		if (errcode == ECONNRESET)
			return TC_PEER_RESET;

		return TC_RECV_ERR;
	}
}

static int
tc_recv_multi_packet_handle(
	int recv_size,
	struct tc_link_private_data *data,
	struct tc_create_link_data  *cl_data
)
{
	int ret = 0;

	while (1) {
		ret = cl_data->epoll_oper->recv_data(
					data->recv_data, 
					recv_size, 
					cl_data->user_data);	
		if (ret == TC_ERR) 
			return ret;
		if (ret != TC_OK) {	//成功接收到一个包
			if (ret == recv_size) //正常接收完成	
				return TC_OK;	
			recv_size -= ret;
			if (recv_size < 0)    //返回值有问题
				return TC_WRONG_RECV_RETURN_VALUE;
			//实际需要的数据比实际获取的数据少
			memmove(data->recv_data, 
				data->recv_data + ret, 
				recv_size);
			continue;
		}
		//包不完整
		data->recv_cnt *= 2;
		data->recv_data = (char*)realloc(
						data->recv_data, 
						data->recv_cnt);
		if (!data->recv_data)
			TC_PANIC("Not enough memory\n");
		memset(data->recv_data, 0, data->recv_cnt);
		break;
	}
	return recv_size;
}

static int
tc_recv_data(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	int recv_size = 0;
	struct tc_link_private_data *data = &cl_data->private_link_data;

	if (!data->recv_data) {
		data->recv_data = (char *)calloc(1, data->recv_cnt);
		if (!data->recv_data) 
			TC_PANIC("Not enough memory for %u\n", data->recv_cnt);
	}
	while (1) {
		recv_size = recv(cl_data->private_link_data.sock, 
				 data->recv_data, 
				 data->recv_cnt, 0);
		if (!cl_data->epoll_oper->recv_data)
			break;
		if (recv_size > 0) {
			ret = tc_recv_multi_packet_handle(recv_size, data, cl_data);
			if (ret <= 0)
				break;
			continue;
		}
		break;
	}
	if (recv_size <= 0) {
		ret = tc_recv_err_handle(recv_size, errno);
		if (ret == TC_WOULDBLOCK)
			goto out;
	}

	TC_FREE(data->recv_data);
	data->recv_cnt = TC_DEFAULT_RECV_BUF;

out:
	return ret;
}
#endif
static int
tc_recv(
	struct list_head *list_node
)
{
	int ret = 0, result = 0;
	struct tc_recv_node *recv_node = NULL;
	struct tc_create_link_data *cl_data = NULL, *new_data = NULL;

	recv_node = tc_list_entry(list_node, struct tc_recv_node, node);
	cl_data = (struct tc_create_link_data *)recv_node->user_data;
	if (!cl_data)
		goto out;

	if (!cl_data->proto_oper || !cl_data->proto_oper->proto_recv)
		goto out;
	ret = cl_data->proto_oper->proto_recv(cl_data);

	result = ret;
	tc_epoll_data_recv_mod(cl_data->private_link_data.sock, (unsigned long)cl_data);
	if (ret != TC_OK && ret != TC_PEER_CLOSED && ret != TC_WOULDBLOCK) 
		goto err;

	if (ret == TC_PEER_CLOSED) {
		ret = tc_create_check_duration();
		goto err;
	}

	if (!cl_data->proto_oper->is_proto_server() && ret != TC_WOULDBLOCK) 
		tc_handle_node_add((unsigned long)cl_data);
	
	goto out;
err:
	if (ret != TC_OK && cl_data->epoll_oper->err_handle) {
		ret = cl_data->epoll_oper->err_handle(
					result, 
					cl_data->user_data);
		cl_data->private_link_data.err_flag = ret;
		tc_create_link_err_handle(cl_data);
	}
out:
	TC_FREE(recv_node);
	return ret;
}

static void
tc_recv_free(
	struct list_head *sl
)
{
	struct tc_recv_node *rnode = NULL;

	rnode = tc_list_entry(sl, struct tc_recv_node, node);
	TC_FREE(rnode);
}

static int
tc_recv_create()
{
	if (global_recv_data.thread_num <= 1) {
		return tc_thread_pool_create(
				1,
				global_recv_data.thread_stack,
				"recv_data",
				tc_recv_free,
				tc_recv,
				NULL,
				&global_recv_data.recv_thread_id);
	}
	else
		return tc_thread_pool_create(
				global_recv_data.thread_num,
				global_recv_data.thread_stack,
				"recv_data", 
				tc_recv_free, 
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
