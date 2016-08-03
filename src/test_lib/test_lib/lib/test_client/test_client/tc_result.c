#include "tc_result.h"
#include "tc_comm.h"
#include "tc_err.h"
#include "tc_print.h"
#include "tc_create_private.h"

static int
tc_result_traversal(
	unsigned long user_data,
	struct hlist_node *hnode,
	int *flag
)
{
	struct tc_create_link_data *cl_data = NULL;

	cl_data = tc_list_entry(hnode, struct tc_create_link_data, node);		

	if (cl_data->epoll_oper->result_func) 
		cl_data->epoll_oper->result_func(cl_data->user_data, &cl_data->link_data);
	(*flag) = 1;

	return TC_OK;
}

int
tc_result ()
{
	return tc_create_link_data_traversal(0, tc_result_traversal);
}
