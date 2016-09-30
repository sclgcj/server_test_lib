#include "tc_std_comm.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_init_private.h"
#include "tc_print.h"
#include "tc_recv_check.h"
#include "tc_epoll_private.h"
#include "tc_create_private.h"
//#include "tc_interface_private.h"
#include "tc_transfer_proto_private.h"
#include "tc_transfer_proto_comm_private.h"

static int
tc_user_def_connect(
	struct tc_create_link_data *cl_data
)
{
	if (cl_data->epoll_oper && 
			cl_data->epoll_oper->create_link)
		return cl_data->epoll_oper->create_link(cl_data->user_data);
}

static int
tc_user_def_setup()
{
	int id = 0;	
	
	struct tc_transfer_proto_oper oper;

	memset(&oper, 0, sizeof(oper));
	oper.proto_connect = tc_user_def_connect;

	tc_transfer_proto_add(&oper, &id);

	return tc_transfer_proto_config_add("user_define", id);
}

int
tc_user_def_init()
{
	return tc_local_init_register(tc_user_def_setup);
}

TC_MOD_INIT(tc_user_def_init);
