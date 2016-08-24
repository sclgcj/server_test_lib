#ifndef TC_HEAP_TIMER_H 
#define TC_HEAP_TIMER_H

enum {
	TC_HEAP_TIMER_FLAG_INSTANT, 
	TC_HEAP_TIMER_FLAG_CONSTANT,
	TC_HEAP_TIMER_FLAG_MAX
};

int
tc_heap_timer_create(
	int alarm_sec,		
	int timer_flag,
	unsigned long user_data,
	int (*timer_func)(unsigned long user_data),
	void (*timer_free)(unsigned long user_data),
	unsigned long *timer_id
);

unsigned long
tc_heap_timer_tick_get();

void
tc_heap_timer_destroy( 
	unsigned long id
);



#endif
