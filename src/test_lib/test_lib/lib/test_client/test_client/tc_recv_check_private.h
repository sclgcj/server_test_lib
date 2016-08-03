#ifndef TC_RECV_CHECK_PRIVATE_H
#define TC_RECV_CHECK_PRIVATE_H

#include "tc_epoll_private.h"

struct tc_recv_check_handle {
	struct tc_timer_list_handle *list_handle;
};

struct tc_recv_check_handle *
tc_recv_check_create(
	int recv_timeout
);

int
tc_recv_check_add(
		struct tc_recv_check_handle *handle,
		struct tc_create_link_data *epoll_data);

#endif
