#include "tc_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_recv_check.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"
#include "tc_interface_private.h"
#include "tc_transfer_proto_private.h"
#include "tc_transfer_proto_comm_private.h"

static int
tc_curl_connect(
	struct tc_create_link_data *cl_data
)
{
	return tc_interface_func_execute(cl_data);
}

static int
tc_curl_setup()
{
	int id = 0;	
	
	struct tc_transfer_proto_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.proto_connect = tc_curl_connect;

	tc_transfer_proto_add(&oper, &id);

	return tc_transfer_proto_config_add("curl_client", id);
}

int
tc_curl_init()
{
	return tc_init_register(tc_curl_setup);
}

TC_MOD_INIT(tc_curl_init);
