#include "tc_comm.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_thread.h"
#include "tc_epoll_private.h"
#include "tc_handle_private.h"
#include "tc_create_private.h"
#include "tc_addr_inet_private.h"
#include "tc_addr_manage_private.h"
#include "tc_transfer_proto_comm.h"
#include "tc_transfer_proto_comm_private.h"

static struct tc_io_data*
tc_io_data_create(
	char *data,
	int  data_len,
	struct tc_address *ta
)
{
	struct tc_io_data *io_data = NULL;

	io_data = (struct tc_io_data *)calloc(1, sizeof(*io_data));
	if (!io_data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	io_data->data = (char*)calloc(data_len, sizeof(char));
	if (!io_data->data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	memcpy(io_data->data, data, data_len);
	io_data->data_len = data_len;
	if (ta) {
		io_data->addr = tc_address_create(ta);
		io_data->addr_type = ta->addr_type;
	}

	return io_data;
}

static void
tc_io_data_destroy(
	struct tc_io_data *io_data
)
{
	if (!io_data)
		return;

	TC_FREE(io_data->addr);
	TC_FREE(io_data->data);
	TC_FREE(io_data);
}

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

	//tc_create_user_data_get(create_data->link_id, &create_data->user_data);

	epoll_data = tc_create_link_data_alloc(
					sock, 
					cl_data->app_proto,
					addr.sin_addr.s_addr,
					ntohs(addr.sin_port),
					cl_data->config,
					create_data);
	if (!epoll_data) 
		TC_PANIC("no enough memory\n");

	tc_create_link_create_data_destroy(create_data);

	tc_handle_node_add((unsigned long)epoll_data);

	return TC_OK;
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
tc_transfer_proto_comm_udp_data_recv(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	int addr_len = 0;
	int recv_size = 0;
	struct tc_address *ta = NULL;
	struct sockaddr_in addr;
	struct tc_link_private_data *data = &cl_data->private_link_data;

	data->recv_cnt = 65535;
	data->recv_data = (char *)calloc(1, data->recv_cnt);
	if (!data->recv_data) 
			TC_PANIC("Not enough memory for %u\n", data->recv_cnt);
	addr_len = sizeof(addr);
	memset(&addr, 0, addr_len);
	while (1) {
		recv_size = recvfrom(cl_data->private_link_data.sock, 
				 data->recv_data, 
				 data->recv_cnt, 0,
				 (struct sockaddr*)&addr, 
				 &addr_len);
		if (!cl_data->epoll_oper->recv_data)
			break;
		if (recv_size > 0) {
			(ta) = tc_address_encode(tc_addr_inet_id_get(), (struct sockaddr*)&addr);
			ret = cl_data->epoll_oper->udp_recv_data(
						data->recv_data, 
						recv_size, 
						(ta),
						cl_data->user_data);	
			if (ret == TC_ERR) 
				goto out;
		}
		break;
	}
	if (recv_size <= 0) {
		ret = tc_transfer_proto_comm_recv_err_handle(recv_size, errno);
	}

out:
	TC_FREE(data->recv_data);
	data->recv_data = NULL;
	data->recv_cnt = 65535;

	return ret;
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
tc_transfer_proto_comm_udp_data_send(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	int len = 0;
	struct list_head *sl = NULL;
	struct tc_io_data *io_data = NULL;
	struct tc_link_private_data *data = NULL;

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
		len = tc_address_length(io_data->addr_type);
		//PRINT("data = %s, data_len = %d\n\n", io_data->data, io_data->data_len);
		ret = sendto(data->sock, io_data->data, io_data->data_len, 
			     0, io_data->addr, len);
		if (ret > 0) {
			tc_io_data_destroy(io_data);
			ret = TC_OK;
			continue;
		} else {
			PRINT("err = %s\n", strerror(errno));
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EPIPE) {
				ret = TC_WOULDBLOCK;
				break;
			}
			tc_io_data_destroy(io_data);
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
tc_transfer_proto_comm_data_send(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	struct list_head *sl = NULL;
	struct tc_io_data *io_data = NULL;
	struct tc_link_private_data *data = NULL;

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
			tc_io_data_destroy(io_data);
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
			tc_io_data_destroy(io_data);
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
	
	io_data = tc_io_data_create(send_data, send_len, NULL);
	if (!io_data)
		return TC_ERR;

	pthread_mutex_lock(&cl_data->private_link_data.send_mutex);
	list_add_tail(&io_data->node, &cl_data->private_link_data.send_list);
	pthread_mutex_unlock(&cl_data->private_link_data.send_mutex);

	tc_epoll_data_send_mod(cl_data->private_link_data.sock, 
				(unsigned long)cl_data);

	return TC_OK;
}

int
tc_transfer_proto_accept_handle(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	int sock = cl_data->private_link_data.sock;
	struct tc_address *ta = NULL;
	struct sockaddr_in addr;

	tc_sock_event_add(sock, TC_EVENT_READ, cl_data);

	if (cl_data->epoll_oper->accept_func) {
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr = cl_data->link_data.peer_addr;
		addr.sin_port = htons(cl_data->link_data.peer_port);
		ta = tc_address_encode(tc_addr_inet_id_get(), (struct sockaddr*)&addr);
		ret = cl_data->epoll_oper->accept_func(
						ta, 
						cl_data->user_data);
		tc_address_destroy(ta);
		if (ret != TC_OK) {
			close(sock);
			tc_create_link_data_destroy(cl_data);
			return ret;
		}
	}

	cl_data->proto_oper->proto_handle = tc_transfer_proto_comm_data_handle;

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
	if (cl_data->epoll_oper->handle_data) 
		ret = cl_data->epoll_oper->handle_data(cl_data->user_data);

	return ret;
}

int
tc_transfer_proto_server()
{
	return 1;
}

int
tc_transfer_proto_client()
{
	return 0;
}

int
tc_transfer_proto_comm_udp_data_send_add(
	char *send_data,
	int  send_len,
	struct tc_address *ta,
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
	
	io_data = tc_io_data_create(send_data, send_len, ta);
	if (!io_data)
		return TC_ERR;

	pthread_mutex_lock(&cl_data->private_link_data.send_mutex);
	list_add_tail(&io_data->node, &cl_data->private_link_data.send_list);
	pthread_mutex_unlock(&cl_data->private_link_data.send_mutex);

	tc_epoll_data_send_mod(cl_data->private_link_data.sock, 
				(unsigned long)cl_data);

	return TC_OK;
}
