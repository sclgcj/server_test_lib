#include "tc_std_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init_private.h"
#include "tc_print.h"
#include "tc_recv_check.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"
#include "tc_transfer_proto_private.h"
#include "tc_transfer_proto_comm_private.h"

static int
tc_udp_server_config_set()
{
	int id = 0;
	struct tc_transfer_proto_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.proto_recv = tc_transfer_proto_comm_udp_data_recv;
	oper.proto_handle = tc_transfer_proto_comm_data_handle;
	oper.is_proto_server = tc_transfer_proto_client;

	tc_transfer_proto_add(&oper, &id);

	return tc_transfer_proto_config_add("udp_server", id);
}

static int
tc_udp_connect(
	struct tc_create_link_data *cl_data
)
{
	int sock = cl_data->private_link_data.sock;

	/*
	 * 在udp中，不需要进行connect，直接向服务器发包即可
	 */

	tc_epoll_data_add(sock, TC_EVENT_WRITE, (unsigned long)cl_data);
	tc_recv_check_start("connect", cl_data->config->connect_timeout, cl_data->user_data);

	return TC_OK;
}

static int
tc_udp_client_config_set()
{
	int id = 0;
	struct tc_transfer_proto_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.proto_recv = tc_transfer_proto_comm_udp_data_recv;
	oper.proto_send = tc_transfer_proto_comm_udp_data_send;
	oper.proto_handle = tc_transfer_proto_comm_data_handle;
	oper.proto_connect = tc_udp_connect;
	oper.is_proto_server = tc_transfer_proto_client;

	tc_transfer_proto_add(&oper, &id);

	return tc_transfer_proto_config_add("udp_client", id);
}

static int
tc_udp_config_set()
{
	tc_udp_client_config_set();
	tc_udp_server_config_set();
}

int
tc_udp_setup()
{
}

static int
tc_udp_uninit()
{
}

int
tc_udp_init()
{
	int ret = 0;

	ret = tc_user_cmd_add(tc_udp_config_set);
	if (ret != TC_OK)	
		return ret;

	return tc_uninit_register(tc_udp_uninit);
}

TC_MOD_INIT(tc_udp_init);
