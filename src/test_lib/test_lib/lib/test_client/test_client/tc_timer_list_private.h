#ifndef TC_TIMER_LIST_PRIVATE_H
#define TC_TIMER_LIST_PRIVATE_H 1

#include "tc_timer_private.h"

struct tc_timer_list_node {
	int		 count;
	int		 timer_id;
	struct list_head head;
	pthread_mutex_t  count_mutex;
	pthread_mutex_t  mutex;
	int (*handle_func)(unsigned long data);
	struct list_head list_node;
};

struct tc_timer_list_timespec {
	struct timespec ts;
	pthread_mutex_t mutex;
};

struct tc_timer_list_handle {
	int timer_sec;
	int timer_flag;
	int (*handle_func)(unsigned long data);
	pthread_mutex_t timer_node_mutex;
	struct tc_timer_list_node *timer_node;
	struct tc_timer_list_timespec list_timespec;	
};

struct tc_timer_list_handle *
tc_timer_list_start(
	int timer_sec,
	int timer_flag,
	int (*list_func)(unsigned long data)
);

int
tc_timer_list_add(
	struct tc_timer_list_handle *handle,
	unsigned long  user_data,
	unsigned long *timer_data
);

void
tc_timer_list_del(
	unsigned long data
);

#endif
