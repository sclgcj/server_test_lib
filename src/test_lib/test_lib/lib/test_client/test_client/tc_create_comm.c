#include "tc_comm.h"
#include "tc_err.h"
#include "tc_epoll_private.h"
#include "tc_create_comm.h"
#include "tc_create_private.h"


int
tc_event_set(
	int event, 
	unsigned long user_data
)
{
	struct tc_create_link_data *cl_data = NULL;

	cl_data = tc_create_link_data_get(user_data);
	if (!cl_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	return tc_epoll_data_mod(cl_data->private_link_data.sock, 
				 event,
				 (unsigned long)cl_data);
}

int
tc_peer_info_get(
	unsigned long user_data,
	unsigned int  *peer_addr,
	unsigned short *peer_port
)
{
	struct tc_create_link_data *cl_data = NULL;

	if (!user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	cl_data = tc_create_link_data_get(user_data);

	if (peer_addr)
		(*peer_addr) = cl_data->link_data.peer_addr.s_addr;
	if (peer_port)
		(*peer_port) = cl_data->link_data.peer_port;

	return TC_OK;
}

int
tc_local_info_get(
	unsigned long user_data,
	unsigned int  *local_addr,
	unsigned short *local_port
)
{
	struct tc_create_link_data *cl_data = NULL;
	if (!user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	cl_data = tc_create_link_data_get(user_data);

	if (local_addr)
		(*local_addr) = cl_data->link_data.local_addr.s_addr;
	if (local_port)
		(*local_port) = cl_data->link_data.local_port;

	return TC_OK;
}

int
tc_close_link(
	unsigned long user_data
)
{
	struct tc_create_link_data *cl_data = NULL;

	if (!user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	cl_data = tc_create_link_data_get(user_data);

	return tc_create_link_data_destroy(cl_data);
}

