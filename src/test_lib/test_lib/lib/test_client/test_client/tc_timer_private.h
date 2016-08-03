#ifndef TC_TIMER_PRIVATE_H
#define TC_TIMER_PRIVATE_H 1

#include "tc_comm.h"

struct tc_timer_data_node {
	unsigned long data;
	unsigned long parent;
	pthread_mutex_t mutex;
	struct list_head list_node;
};

int
tc_timer_create(
	int alarm_sec,		
	int timer_flag,
	unsigned long user_data,
	int (*timer_func)(unsigned long user_data),
	int *timer_id
);

int
tc_timer_tick_get();

void
tc_timer_destroy( 
	int id
);

#endif
