#ifndef TC_TIMER_H
#define TC_TIMER_H

enum {
	TC_TIMER_FLAG_INSTANT, 
	TC_TIMER_FLAG_CONSTANT,
	TC_TIMER_FLAG_MAX
};

int
tc_timer_create(
	int alarm_sec,		
	int timer_flag,
	unsigned long user_data,
	int (*timer_func)(unsigned long user_data),
	void (*timer_free)(unsigned long user_data),
	int *timer_id
);

int
tc_timer_tick_get();

void
tc_timer_destroy( 
	int id
);

#endif
