#ifndef TC_HEAP_TIMER_PRIVATE_H
#define TC_HEAP_TIMER_PRIVATE_H

#include "tc_comm.h"
#include "tc_heap_timer.h"

struct tc_heap_timer_data_node {
	unsigned long data;
	unsigned long parent;
	pthread_mutex_t mutex;
	struct list_head list_node;
};


#endif
