#ifndef TC_EPOLL_H
#define TC_EPOLL_H

#define TC_EVENT_READ  (EPOLLONESHOT | EPOLLIN)
#define TC_EVENT_WRITE (EPOLLONESHOT | EPOLLOUT)

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

#endif
