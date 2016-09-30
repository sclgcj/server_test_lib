#ifndef TC_TIMER_PRIVATE_H
#define TC_TIMER_PRIVATE_H 1

#include "tc_std_comm.h"
#include "tc_timer.h"

struct tc_timer_data_node {
	unsigned long data;
	unsigned long parent;
	pthread_mutex_t mutex;
	struct list_head list_node;
};

#endif
