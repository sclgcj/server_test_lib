#ifndef TC_EPOLL_DATA_H
#define TC_EPOLL_DATA_H

#include "tc_std_comm.h"
#include "tc_epoll.h"
#include "tc_create_private.h"

struct tc_epoll_oper {
	int (*epoll_recv)(unsigned long epoll_data);
	int (*epoll_send)(unsigned long epoll_data);
	int (*epoll_err)(int error_num, unsigned long epoll_data);
};

int
tc_epoll_data_add(
	int		sock,
	int		event,	
	unsigned long	user_data
);

int
tc_epoll_config_set(
	int duration,
	struct tc_epoll_oper *oper
);

int
tc_epoll_start();

int
tc_epoll_data_recv_mod(
	int		sock,
	unsigned long	data
);

int
tc_epoll_data_mod(
	int		sock,
	int		event,	
	unsigned long	user_data
);

int
tc_epoll_data_del(
	int sock
);

int
tc_epoll_data_send_mod(
	int		sock,
	unsigned long	data
);

#endif
