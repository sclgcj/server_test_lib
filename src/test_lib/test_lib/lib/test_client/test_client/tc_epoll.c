#include "tc_epoll_private.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_init.h"
#include "tc_create.h"
#include "tc_timer_private.h"
#include "tc_print.h"
#include "tc_thread.h"

struct tc_global_epoll_data {
	int duration;
	int epoll_fd;
	struct tc_epoll_oper oper;
};

#define TC_EPOLL_EVENT_MAX  1024
static struct tc_global_epoll_data global_epoll_data;

static int
tc_epoll_data_ctl(
	int	sock,
	int	cmd,
	int	event,	
	void	*ptr
)
{
	int ret = 0;
	struct epoll_event epoll_event, *tmp = NULL;

	if (ptr)
	{
		memset(&epoll_event, 0, sizeof(epoll_event));
		epoll_event.events = event;
		epoll_event.data.ptr = ptr;
		tmp = &epoll_event;
	}

	ret = epoll_ctl(
			global_epoll_data.epoll_fd, 
			cmd,
			sock, 
			tmp);
	if (ret < 0) {
		TC_ERRNO_SET(TC_CTL_EPOLL_ERR);
		return TC_ERR;
	}

	return TC_OK;
}

int
tc_epoll_data_add(
	int		sock,
	int		event,	
	unsigned long	epoll_data
)
{
	return tc_epoll_data_ctl(sock, EPOLL_CTL_ADD, event, (void*)epoll_data);
}

int
tc_epoll_data_mod(
	int		sock,
	int		event,	
	unsigned long	epoll_data
)
{
	return tc_epoll_data_ctl(sock, EPOLL_CTL_MOD, event, (void*)epoll_data);
}

int
tc_epoll_data_del(
	int sock
)
{
	return tc_epoll_data_ctl(sock, EPOLL_CTL_DEL, 0, NULL);
}

int
tc_epoll_config_set(
	int duration,
	struct tc_epoll_oper *oper
)
{
	global_epoll_data.duration = duration;
	memcpy(&global_epoll_data.oper, oper, sizeof(*oper));
	return TC_OK;
}

static int
tc_epoll_check_duration()
{
	int tick = 0;	

	tick = tc_timer_tick_get();
	if (tick == global_epoll_data.duration)
		return TC_OK;
	
	return TC_ERR;
}

int
tc_epoll_start()
{
	int i = 0;
	int ret = 0;
	int num_fds = 0;
	int event_size = 0;
	struct epoll_event event[TC_EPOLL_EVENT_MAX];	

	event_size = sizeof(event);
	for (; ;) {
		memset(event, 0, event_size);
		num_fds = epoll_wait(
				   global_epoll_data.epoll_fd,
				   event, 
				   TC_EPOLL_EVENT_MAX, 
				   1000);
		if (num_fds < 0)
			TC_PANIC("epoll wait error :%s\n", strerror(errno));
		ret = tc_epoll_check_duration();
		if (ret == TC_OK)
			break;
		//real timer end check
		//ret = tc_rt_end_check();
		for (i = 0; i < num_fds; i++) {
			if ((event[i].events & EPOLLIN) && 
					(global_epoll_data.oper.epoll_recv))
				global_epoll_data.oper.epoll_recv(event[i].data.ptr);
			else if ((event[i].events & EPOLLOUT) && 
					(global_epoll_data.oper.epoll_send))
				global_epoll_data.oper.epoll_send(event[i].data.ptr);
			else if ((event[i].events & EPOLLERR) && 
					(global_epoll_data.oper.epoll_err))
				global_epoll_data.oper.epoll_err(errno, event[i].data.ptr);
			else
				continue;
		}
	}
}

static int
tc_epoll_uninit()
{
	close(global_epoll_data.epoll_fd);
	
	return TC_OK;
}

int
tc_epoll_init()
{
	memset(&global_epoll_data, 0, sizeof(global_epoll_data));
	global_epoll_data.epoll_fd = epoll_create(1000000);
	if (global_epoll_data.epoll_fd < 0) 
		TC_PANIC("create epoll error : %s\n", strerror(errno));

	return tc_uninit_register(tc_epoll_uninit);
}

TC_MOD_INIT(tc_epoll_init);
