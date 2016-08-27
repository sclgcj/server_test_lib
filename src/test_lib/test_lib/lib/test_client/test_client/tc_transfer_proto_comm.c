#include "tc_comm.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_thread.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"
#include "tc_transfer_proto_comm_private.h"


int
tc_transfer_proto_comm_accept(
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

	create_data = tc_create_data_calloc(
					NULL, 0, 0, 
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
			tc_create_link_data_destroy(cl_data);
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
tc_transfer_proto_comm_recv_multi_packet_handle(
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
			memset(data->recv_data + recv_size, 0, ret);
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
tc_transfer_proto_comm_recv_err_handle(
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
		if (errcode == ECONNREFUSED)
			return TC_PEER_REFUSED;
		if (errcode == ENOTCONN)
			return TC_NOT_CONNECT;

		return TC_RECV_ERR;
	}
}

int
tc_transfer_proto_comm_data_recv(
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
			ret = tc_transfer_proto_comm_recv_multi_packet_handle(
								recv_size, data, cl_data);
			if (ret <= 0)
				break;
			continue;
		}
		break;
	}
	if (recv_size <= 0) {
		ret = tc_transfer_proto_comm_recv_err_handle(recv_size, errno);
		if (ret == TC_WOULDBLOCK)
			goto out;
	}

	TC_FREE(data->recv_data);
	data->recv_data = NULL;
	data->recv_cnt = TC_DEFAULT_RECV_BUF;

out:
	return ret;
}

int	
tc_transfer_proto_comm_data_send(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	struct list_head *sl = NULL;
	struct tc_io_data *io_data = NULL;
	struct tc_link_private_data *data = NULL;

	PRINT("\n");
	data = &cl_data->private_link_data;
	while (1) {
		if (tc_thread_test_exit() == TC_OK)
			break;
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
}

int
tc_transfer_proto_comm_data_send_add(
	char *send_data,
	int  send_len,
	unsigned long user_data
)
{
	int ret = 0;
	struct tc_io_data *io_data = NULL;
	struct tc_create_link_data *cl_data = NULL;

	cl_data = tc_create_link_data_get(user_data);
	if (!cl_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	io_data = (struct tc_io_data *)calloc(1, sizeof(*io_data));
	if (!io_data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	io_data->data = (char*)calloc(send_len, 1);
	if (!io_data->data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	memcpy(io_data->data, send_data, send_len);
	io_data->data_len = send_len;
	pthread_mutex_lock(&cl_data->private_link_data.send_mutex);
	list_add_tail(&io_data->node, &cl_data->private_link_data.send_list);
	pthread_mutex_unlock(&cl_data->private_link_data.send_mutex);

	PRINT("\n");
	tc_epoll_data_send_mod(cl_data->private_link_data.sock, 
				(unsigned long)cl_data);

	return TC_OK;
}

int
tc_transfer_proto_comm_data_handle(
	struct tc_create_link_data *cl_data
)
{
	int ret = TC_OK;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = cl_data->link_data.peer_addr.s_addr;
	addr.sin_port = htons(cl_data->link_data.peer_port);
	if (cl_data->epoll_oper->handle_data) {
		ret = cl_data->epoll_oper->handle_data(
					cl_data->user_data);
	}

	return ret;
}

