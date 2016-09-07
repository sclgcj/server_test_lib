#include "tc_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_recv_check.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"
#include "tc_transfer_proto_private.h"
#include "tc_transfer_proto_comm_private.h"

static int
tc_tcp_server_config_set()
{
	int id = 0;
	struct tc_transfer_proto_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.proto_recv = tc_transfer_proto_comm_accept;
	oper.is_proto_server = tc_transfer_proto_server;
	oper.proto_handle =  tc_transfer_proto_accept_handle;

	tc_transfer_proto_add(&oper, &id);

	return tc_transfer_proto_config_add("tcp_server", id);
}

static int
tc_tcp_connect(
	struct tc_create_link_data *cl_data
)
{
	int ret = 0;
	int sock = 0;
	int addr_size = sizeof(struct sockaddr_in);
	struct sockaddr_in server_addr;
	
	sock = cl_data->private_link_data.sock;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family	= AF_INET;
	server_addr.sin_port	= htons(cl_data->link_data.peer_port);
	server_addr.sin_addr	= cl_data->link_data.peer_addr;	

	PRINT("sever_ip = %s, server_port = %d\n", 
			inet_ntoa(server_addr.sin_addr), 
			ntohs(server_addr.sin_port));
	while (1) {
		ret = connect(sock, (struct sockaddr*)&server_addr, addr_size);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			else if (errno == EISCONN)
				break;
			else if (errno != EINPROGRESS && 
					errno != EALREADY && errno != EWOULDBLOCK) 
				TC_PANIC("connect error :%s\n", strerror(errno));
			break;
		}
		break;
	}

	tc_epoll_data_add(sock, TC_EVENT_WRITE, (unsigned long)cl_data);
	tc_recv_check_start("connect", cl_data->config->connect_timeout, cl_data->user_data);
	return TC_OK;
}

static int
tc_tcp_client_config_set()
{
	int id = 0;
	struct tc_transfer_proto_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.proto_recv = tc_transfer_proto_comm_data_recv;
	oper.proto_send = tc_transfer_proto_comm_data_send;
	oper.proto_handle = tc_transfer_proto_comm_data_handle;
	oper.proto_connect = tc_tcp_connect;
	oper.is_proto_server = tc_transfer_proto_client;

	tc_transfer_proto_add(&oper, &id);

	return tc_transfer_proto_config_add("tcp_client", id);
}

static int
tc_tcp_config_set()
{
	tc_tcp_client_config_set();
	tc_tcp_server_config_set();
}

int
tc_tcp_setup()
{
}

static int
tc_tcp_uninit()
{
}

int
tc_tcp_init()
{
	int ret = 0;

	ret = tc_user_cmd_add(tc_tcp_config_set);
	if (ret != TC_OK)	
		return ret;

	return tc_uninit_register(tc_tcp_uninit);
}

TC_MOD_INIT(tc_tcp_init);
