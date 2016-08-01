#include "tc_timer_list_private.h"
#include "tc_err.h"
#include "tc_comm.h"
#include "tc_print.h"
#include "tc_timer.h"

void
tc_timer_list_del(
	unsigned long data
)
{
	int ret = 0;
	struct tc_timer_data_node *data_node = NULL;
	struct tc_timer_list_node *timer_node = NULL;

	if (!data)
		return;

	PRINT("del data \n");
	data_node = (struct tc_timer_data_node *)data;
	timer_node = (struct tc_timer_list_node*)data_node->parent;
	PRINT("eeeeee\n");
	pthread_mutex_lock(&timer_node->mutex);
	timer_node->count--;
	list_del_init(&data_node->list_node);
	pthread_mutex_unlock(&timer_node->mutex);
}

static int
tc_timer_list_check(
	unsigned long data
)
{
	int ret = 0;
	struct list_head *sl = NULL;
	struct tc_timer_list_node *timer_node = NULL;
	struct tc_timer_data_node *data_node = NULL;

	if (!data)
		return TC_OK;

	timer_node = (struct tc_timer_list_node *)data;
	ret = pthread_mutex_trylock(&timer_node->mutex);
	if (ret != 0) {
		if (ret == EBUSY)  {
			PRINT("busy \n");
			return TC_OK;
		}
		TC_PANIC("pthread_mutex_trylock : %s\n", strerror(errno));
	}
	sl = timer_node->head.next;
	PRINT("dfdfdfdf\n");
	while (sl != &timer_node->head) {
		data_node = tc_list_entry(sl, struct tc_timer_data_node, list_node);
		sl = sl->next;
		if (timer_node->handle_func) {
			ret = timer_node->handle_func(data_node->data);
			if (ret != TC_OK) {
				timer_node->count--;
				PRINT("id = %d, count =============== %d\n", timer_node->timer_id, timer_node->count);
				list_del_init(&data_node->list_node);
			}
		}
	}
	pthread_mutex_lock(&timer_node->count_mutex);
	PRINT("id = %d, count = %d\n", timer_node->timer_id, timer_node->count);
	if (timer_node->count == 0)
		ret = TC_ERR;
	else
		ret = TC_OK;
	pthread_mutex_unlock(&timer_node->count_mutex);
	/*list_for_each_entry(data_node, &timer_node->head, list_node) {
		PRINT("data_node pointer = %p\n", (char*)data_node->data);
		if (timer_node->handle_func) {
			ret = timer_node->handle_func(data_node->data);
			if (ret != TC_OK) {

			}
		}
	}*/
	pthread_mutex_unlock(&timer_node->mutex);

	return ret;
}

static struct tc_timer_list_node *
tc_timer_list_node_create(
	int (*handle_func)(unsigned long data)
)
{
	struct tc_timer_list_node *node = NULL;

	node = (struct tc_timer_list_node *)calloc(1, sizeof(*node));
	if (!node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	node->handle_func = handle_func;
	INIT_LIST_HEAD(&node->head);
	pthread_mutex_init(&node->mutex, NULL);
	pthread_mutex_init(&node->count_mutex, NULL);

	return node;
}

static int
tc_timer_list_end(
	unsigned long data
)
{
	struct timespec t2;
	struct tc_timer_list_handle *handle = NULL;

	handle = (struct tc_timer_list_handle*)data;
	if (!handle)
		return TC_OK;

	clock_gettime(CLOCK_MONOTONIC, &t2);
	pthread_mutex_lock(&handle->timer_node_mutex);
	if (handle->timer_node && handle->list_timespec.ts.tv_sec > 0) {
		if (t2.tv_sec - handle->list_timespec.ts.tv_sec > 1 || 
			(t2.tv_sec - handle->list_timespec.ts.tv_sec == 1 && 
			 t2.tv_nsec - handle->list_timespec.ts.tv_nsec >= 0)) {
			PRINT("=====count = %d\n", handle->timer_node->count);
			clock_gettime(CLOCK_MONOTONIC, &handle->list_timespec.ts);
			tc_timer_create(
				handle->timer_sec,
				handle->timer_flag,
				(unsigned long)handle->timer_node,
				tc_timer_list_check,
				&handle->timer_node->timer_id);
			PRINT("check_timer---------------------------> %d\n", handle->timer_node->timer_id);
			handle->timer_node = NULL;
		}
	}
	pthread_mutex_unlock(&handle->timer_node_mutex);
}

struct tc_timer_list_handle *
tc_timer_list_start(
	int timer_sec,
	int timer_flag,
	int (*list_func)(unsigned long data)
)
{
	int end_timer = 0;
	struct tc_timer_list_handle *handle = NULL;

	handle = (struct tc_timer_list_handle *)calloc(1, sizeof(*handle));
	if (!handle) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	/*handle->timer_node = tc_timer_list_node_create();
	if (!handle->timer_node) {
		TC_FREE(handle);
		return NULL;
	}
	INIT_LIST_HEAD(&handle->timer_node->head);*/
	pthread_mutex_init(&handle->list_timespec.mutex, NULL);
	pthread_mutex_init(&handle->timer_node_mutex, NULL);
	handle->timer_sec = timer_sec;
	handle->timer_flag = timer_flag;
	handle->handle_func = list_func;

	tc_timer_create(
			1, 
			TC_TIMER_FLAG_CONSTANT, 
			(unsigned long)handle, 
			tc_timer_list_end,
			&end_timer);
	PRINT("end_timer---------------------------> %d\n", end_timer);

	return handle;
}

int
tc_timer_list_add(
	struct tc_timer_list_handle *handle,
	unsigned long  user_data,
	unsigned long *timer_data
)
{
	int flag = 0;
	struct timespec t2;
	struct tc_timer_list_node *tmp = NULL;
	struct tc_timer_data_node *data_node = NULL;

	//PRINT("\n");
	data_node = (struct tc_timer_data_node *)calloc(1, sizeof(*data_node));
	if (!data_node) {
		pthread_mutex_unlock(&handle->timer_node_mutex);
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	data_node->data = user_data;
	*(timer_data) = (unsigned long)data_node;
	//PRINT("dat_node->data = %p\n", (char*)data_node->data);

	pthread_mutex_lock(&handle->timer_node_mutex);
	if (!handle->timer_node) {
		handle->timer_node = tc_timer_list_node_create(handle->handle_func);
		if (!handle->timer_node) {
			pthread_mutex_unlock(&handle->timer_node_mutex);
			return TC_ERR;
		}
		clock_gettime(CLOCK_MONOTONIC, &handle->list_timespec.ts);
	}
	data_node->parent = (unsigned long)handle->timer_node;
	//*(timer_data) = (unsigned long)handle->timer_node;
	//pthread_mutex_lock(&handle->list_timespec.mutex);
	clock_gettime(CLOCK_MONOTONIC, &t2);

	/*
	 * check if over 1 second
	 */
	//PRINT("sec diff =%d, %lu\n", t2.tv_sec - handle->list_timespec.ts.tv_sec, handle->list_timespec.ts.tv_sec);
	tmp = handle->timer_node;
	if (t2.tv_sec - handle->list_timespec.ts.tv_sec > 1 || 
		(t2.tv_sec - handle->list_timespec.ts.tv_sec == 1 && 
		 t2.tv_nsec - handle->list_timespec.ts.tv_nsec >= 0)) {
		flag = 1;
		handle->timer_node = NULL;
	}	
	pthread_mutex_unlock(&handle->timer_node_mutex);
	//pthread_mutex_unlock(&handle->list_timespec.mutex);

	pthread_mutex_lock(&tmp->mutex);
	list_add_tail(&data_node->list_node, &tmp->head);
	tmp->count++;
	pthread_mutex_unlock(&tmp->mutex);

	if (flag == 1) {
		PRINT("timer node count = %d\n", tmp->count);
		tc_timer_create(
				handle->timer_sec, 
				handle->timer_flag, 
				(unsigned long)tmp,
				tc_timer_list_check,
				&tmp->timer_id);
			PRINT("check_timer--------------------------222-> %d\n", tmp->timer_id);
	}
	
	return TC_OK;
}


