#include "tc_epoll_private.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_init.h"
#include "tc_create.h"
#include "tc_print.h"
#include "tc_thread.h"
#include "tc_heap_timer_private.h"

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
	//	PRINT("epoll_ctl err: %s\n", strerror(errno));
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
tc_epoll_data_recv_mod(
	int		sock,
	unsigned long	data
)
{
	return tc_epoll_data_ctl(sock, EPOLL_CTL_MOD, TC_EVENT_READ, (void*)data);
}

int
tc_epoll_data_send_mod(
	int		sock,
	unsigned long	data
)
{
	return tc_epoll_data_ctl(sock, EPOLL_CTL_MOD, TC_EVENT_WRITE, (void*)data);
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

	tick = tc_heap_timer_tick_get();
	//PRINT("tick = %d, duration = %d\n", tick, global_epoll_data.duration);
	if (global_epoll_data.duration == 0)
		return TC_ERR;
	if (tick >= global_epoll_data.duration) {
		tc_thread_exit_wait();
		return TC_OK;
	}
	
	return TC_ERR;
}

int
tc_epoll_start()
{
	int i = 0;
	int ret = 0;
	int num_fds = 0;
	int event_size = 0;
	unsigned long data = 0;
	struct epoll_event event[TC_EPOLL_EVENT_MAX];	

	event_size = sizeof(event);
	for (; event_size != 0;) {
		memset(event, 0, event_size);
		num_fds = epoll_wait(
				   global_epoll_data.epoll_fd,
				   event, 
				   TC_EPOLL_EVENT_MAX, 
				   1000);
		PRINT("num_fds = %d\n", num_fds);
		if (num_fds < 0) {
			if (errno == EINTR)
				break;
			TC_PANIC("epoll wait error :%s\n", strerror(errno));
		}
		ret = tc_epoll_check_duration();
		if (ret == TC_OK)
			break;
		//real timer end check
		//ret = tc_rt_end_check();
		for (i = 0; i < num_fds; i++) {
			if (tc_thread_test_exit() == TC_OK)  {
				event_size = 0;
				break;	
			}
			data = (unsigned long)event[i].data.ptr;
			if ((event[i].events & EPOLLIN || event[i].events & EPOLLERR) && 
					(global_epoll_data.oper.epoll_recv)){
				PRINT("\n");
				global_epoll_data.oper.epoll_recv(data);
			}
			else if ((event[i].events & EPOLLOUT) && 
					(global_epoll_data.oper.epoll_send)) 
				global_epoll_data.oper.epoll_send(data);
			/*else if ((event[i].events & EPOLLERR) && 
					(global_epoll_data.oper.epoll_err))  {
				PRINT("epoll error:%s\n", strerror(errno));
				global_epoll_data.oper.epoll_err(errno, data);
			}*/
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
