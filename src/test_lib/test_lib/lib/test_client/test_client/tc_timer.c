#include "tc_timer.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_hash.h"
#include "tc_init.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_thread.h"
#include "tc_timer_private.h"

struct tc_timer_node {
	int			id;			//在hash链表中的位置
	int			tick;			//定时滴答数
	int			status;			//运行状态
	int			check_list_id;		//检查链表的id
	int			timer_flag;		//定时器标志，持续存在，还是只运行一次
	int			timer_sec;		//定时时长
	unsigned long		user_data;		//用户数据
	int (*timer_func)(unsigned long user_data);	
	pthread_mutex_t		tick_mutex;
	struct hlist_node	node;
	struct list_head	list_node;	
	struct list_head	check_list_node;
};

struct tc_timer_check_node {
	int		 status;
	struct list_head head;
	struct list_head check_head;
	unsigned long	end_num;
	unsigned long	start_num;
	pthread_mutex_t mutex;
	pthread_mutex_t check_mutex;
	struct list_head list_node;
};

struct tc_timer_count_data {
	unsigned long count;
	pthread_mutex_t mutex;
};

struct tc_timer_list_data {
	struct list_head head;
	pthread_mutex_t mutex;
};

struct tc_timer_list_config {
	int timer_sec;
	int timer_flag;
	int (*list_func)(unsigned long data);
};

#define TC_TIMER_TABLE_SIZE  86400
#define TC_TIMER_SPLICE	     8
struct tc_timer_data {
	int		 thread_num;
	int		 thread_stack;
	int		 check_list_cnt;
	int		 timer_count_id;
	int		 timer_check_id;
	int		 timer_handle_id;
	unsigned long	 max_id;
	tc_hash_handle_t timer_hash;
	pthread_mutex_t  check_list_mutex;
	struct tc_timer_list_data  timer_list;
	struct tc_timer_list_data  free_id_list;
	struct tc_timer_count_data timer_tick;
	struct tc_timer_count_data timer_count;
	struct tc_timer_check_node check_list[TC_TIMER_SPLICE];
};

enum {
	TC_TIMER_STATUS_IDLE,
	TC_TIMER_STATUS_RUNNING,
	TC_TIMER_STATUS_MAX
};

static struct tc_timer_data global_timer_data;
static int tc_timer_id_node_add(int id);
static int tc_timer_id_node_get();

static void
tc_timer_list_add(
	struct tc_timer_node *timer_node
)
{
	int id = 0;

	pthread_mutex_lock(&global_timer_data.check_list_mutex);
	id = (global_timer_data.check_list_cnt % TC_TIMER_SPLICE);
	global_timer_data.check_list_cnt++;
	timer_node->check_list_id = id;
	PRINT("id = %d\n", id);
	pthread_mutex_unlock(&global_timer_data.check_list_mutex);
	pthread_mutex_lock(&global_timer_data.check_list[id].mutex);
	list_add_tail(&timer_node->check_list_node, &global_timer_data.check_list[id].head);
	pthread_mutex_unlock(&global_timer_data.check_list[id].mutex);
}

static void
tc_timer_list_del(
	struct tc_timer_node *timer_node
)
{
	int id = timer_node->check_list_id;

	pthread_mutex_lock(&global_timer_data.check_list[id].check_mutex);
	list_del_init(&timer_node->check_list_node);
	pthread_mutex_unlock(&global_timer_data.check_list[id].check_mutex);
}

int
tc_timer_create(
	int timer_sec,		
	int timer_flag,
	unsigned long user_data,
	int (*timer_func)(unsigned long user_data),
	int *timer_id
)
{
	int ret = 0;
	struct timespec ts;
	struct tc_timer_node *timer_node = NULL;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	timer_node = (struct tc_timer_node *)calloc(1, sizeof(*timer_node));
	if (!timer_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	ret = tc_timer_id_node_get();
	pthread_mutex_lock(&global_timer_data.timer_count.mutex);
	if (ret != TC_ERR) {
		timer_node->id = ret;
		global_timer_data.timer_count.count++;
	}
	else
		timer_node->id = global_timer_data.timer_count.count++;
	/*if (global_timer_data.max_id < global_timer_data.timer_count.count)
		global_timer_data.max_id = global_timer_data.timer_count.count;*/
	pthread_mutex_unlock(&global_timer_data.timer_count.mutex);
	timer_node->user_data	= user_data;
	timer_node->timer_flag  = timer_flag;
	timer_node->timer_func  = timer_func;
	timer_node->timer_sec	= timer_sec;
	if (timer_id)
		*timer_id = timer_node->id;
	pthread_mutex_init(&timer_node->tick_mutex, NULL);
		
	PRINT("timer_id = %d\n", timer_node->id);
	tc_timer_list_add(timer_node);
	PRINT(".....\n");

	return TC_OK;
//	return tc_hash_add(global_timer_data.timer_hash, &timer_node->node, timer_node->id);
}

static int
tc_timer_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	unsigned long hash_val = 0;
	struct tc_timer_node *timer_node = NULL;

	if (!hnode) 
		hash_val = user_data;
	else {
		timer_node = tc_list_entry(hnode, struct tc_timer_node, node);
		hash_val = timer_node->id;
	}

	return (int)(hash_val % TC_TIMER_TABLE_SIZE);
}

static int
tc_timer_hash_get(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	struct tc_timer_node *timer_node = NULL;	

	timer_node = tc_list_entry(hnode, struct tc_timer_node, node);
	if (timer_node->id == (int)user_data) 
		return TC_OK;
	return TC_ERR;
}

static int
tc_timer_id_node_add(
	int id
)
{
	struct tc_timer_data_node *id_node = NULL;
	id_node = (struct tc_timer_data_node *)calloc(1, sizeof(*id_node));
	if (!id_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	id_node->data = id;
	pthread_mutex_lock(&global_timer_data.free_id_list.mutex);
	list_add_tail(&id_node->list_node, &global_timer_data.free_id_list.head);
	pthread_mutex_unlock(&global_timer_data.free_id_list.mutex);
	
	return TC_OK;
}

static int
tc_timer_id_node_get()
{
	int ret = 0;
	struct list_head *sl = NULL;
	struct tc_timer_data_node *id_node = NULL; 

	pthread_mutex_lock(&global_timer_data.free_id_list.mutex);
	if (list_empty(&global_timer_data.free_id_list.head)) {
		ret = TC_ERR;
		goto out;
	}
	sl = global_timer_data.free_id_list.head.next;
	id_node = tc_list_entry(sl, struct tc_timer_data_node, list_node);
	if (!id_node)
		PRINT("ererer\n");
	ret = id_node->data;
	list_del_init(sl);
	TC_FREE(id_node);
out:
	pthread_mutex_unlock(&global_timer_data.free_id_list.mutex);

	return ret;
}

static int
tc_timer_node_destroy(
	struct tc_timer_node *timer_node
)
{
	tc_timer_id_node_add(timer_node->id);

	tc_timer_list_del(timer_node);

	TC_FREE(timer_node);

	//pthread_mutex_lock(&global_timer_data.timer_count.mutex);
	//global_timer_data.timer_count.count--;
	//PRINT("global_timer_data.timer_count.cout = %ld\n",global_timer_data.timer_count.count);
	//pthread_mutex_unlock(&global_timer_data.timer_count.mutex);
}

static int
tc_timer_hash_destroy(
	struct hlist_node	*hnode
)
{
	struct tc_timer_node *timer_node = NULL;

	timer_node = tc_list_entry(hnode, struct tc_timer_node, node);
	tc_timer_node_destroy(timer_node);

/*	tc_timer_id_node_add(timer_node->id);

	TC_FREE(timer_node);

	pthread_mutex_lock(&global_timer_data.timer_count.mutex);
	global_timer_data.timer_count.count--;
	PRINT("global_timer_data.timer_count.cout = %ld\n",global_timer_data.timer_count.count);
	pthread_mutex_unlock(&global_timer_data.timer_count.mutex);*/
	return TC_OK;
}

int
tc_timer_tick_get()
{
	int ret = 0;
	
	pthread_mutex_lock(&global_timer_data.timer_tick.mutex);
	ret = global_timer_data.timer_tick.count;
	pthread_mutex_unlock(&global_timer_data.timer_tick.mutex);

	return ret;
}

static int 
tc_timer_check_node_add(
	int count
)
{
	int ret = 0;
	struct tc_timer_check_node *check_node = NULL;

	check_node = &global_timer_data.check_list[count];
//	count = (count + 1) % (TC_TIMER_SPLICE + 1);
	//PRINT("=============> count = %d\n", count);
	ret = pthread_mutex_trylock(&check_node->check_mutex);
	if (ret == EBUSY) {
		PRINT("busy\n");
		return TC_OK;
	}
	if (check_node->status == TC_TIMER_STATUS_RUNNING) {
		ret = TC_ERR;
		goto out;
	}
	pthread_mutex_lock(&check_node->mutex);
	if (!list_empty(&check_node->head)) 
		list_splice_tail_init(&check_node->head, &check_node->check_head);
	pthread_mutex_unlock(&check_node->mutex);
	if (list_empty(&check_node->check_head)) 
		ret = TC_ERR;
	else {
		check_node->status = TC_TIMER_STATUS_RUNNING;
		ret = TC_OK;
	}
out:
	pthread_mutex_unlock(&check_node->check_mutex);

	if (ret == TC_OK)
		return tc_thread_pool_node_add(
					global_timer_data.timer_check_id, 
					&check_node->list_node);

	return ret;
}

static int
tc_timer_count(
	struct list_head *node
)
{		
	int i = 0;
	int ret = 0, max = 0;
	int left = 0, div = 0;
	int timer_count = 0;
	struct hlist_node *hnode = NULL;
	struct tc_timer_node *timer_node = NULL;

	timer_node = tc_list_entry(node, struct tc_timer_node, list_node);
	TC_FREE(timer_node);
	while (1) {
		ret = tc_thread_test_exit();
		if (ret == TC_OK)
			break;
		sleep(1);
		ret = tc_thread_test_exit();
		if (ret == TC_OK)
			break;
		pthread_mutex_lock(&global_timer_data.timer_tick.mutex);
		global_timer_data.timer_tick.count++;
		PRINT("tick = %ld\n", global_timer_data.timer_tick.count);
		pthread_mutex_unlock(&global_timer_data.timer_tick.mutex);

		/* 
		 * we hope that we can use multi-thread to handle many 
		 * timers, so we decide to splice the total timer count 
		 * into pieces. Here we just consider that this will work 
		 * fine, more tests are required.
		 */
		/*pthread_mutex_lock(&global_timer_data.timer_count.mutex);
		if (timer_count != global_timer_data.timer_count.count) {
			timer_count = global_timer_data.timer_count.count;
			max = global_timer_data.max_id;
			div = global_timer_data.max_id / TC_TIMER_SPLICE;
			left = global_timer_data.max_id % TC_TIMER_SPLICE;
		}
		pthread_mutex_unlock(&global_timer_data.timer_count.mutex);*/
				
		for (i = 0; i < global_timer_data.thread_num; i++) 
			tc_timer_check_node_add(i);
	}

	return TC_OK;
}

void
tc_timer_destroy(
	int id
)
{
	struct hlist_node *hnode = NULL;

	hnode = tc_hash_get(global_timer_data.timer_hash, id, id);
	if (!hnode)
		return;

	PRINT(" id = %d\n", id);
		
	tc_hash_del(global_timer_data.timer_hash, hnode, id);
	tc_timer_hash_destroy(hnode);
}

static int
tc_timer_handle(
	struct list_head *node
)
{
	int ret = 0;
	struct tc_timer_node *timer_node = NULL;

	timer_node = tc_list_entry(node, struct tc_timer_node, list_node);

	if (timer_node->timer_func) {
		//PRINT("id = %d----\n", timer_node->id);
		ret = timer_node->timer_func(timer_node->user_data);
		//PRINT("id = %d, ret = %d\n", timer_node->id, ret);
//		tc_timer_destroy(timer_node->id);
		if (ret != TC_OK) 
			tc_timer_node_destroy(timer_node);
		else {
			pthread_mutex_lock(&timer_node->tick_mutex);
			timer_node->tick = 0;
			timer_node->status = TC_TIMER_STATUS_IDLE;
			pthread_mutex_unlock(&timer_node->tick_mutex);
		}
	}

	if (timer_node->timer_flag == TC_TIMER_FLAG_INSTANT)
		tc_timer_node_destroy(timer_node);
		//tc_timer_destroy(timer_node->id);

	return TC_OK;
}

static int
tc_timer_count_start()
{
	struct tc_timer_node *timer_node = NULL;	

	timer_node = (struct tc_timer_node *)calloc(1, sizeof(*timer_node));
	if (!timer_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	return	tc_thread_pool_node_add(global_timer_data.timer_count_id, &timer_node->list_node);
}

static int
tc_timer_check(
	struct list_head *node
)
{
	int i = 0;
	struct list_head *sl = NULL;
	struct hlist_node *hnode = NULL;
	struct tc_timer_node *timer_node = NULL, *safe = NULL;
	struct tc_timer_check_node *check_node = NULL;

	check_node = tc_list_entry(node, struct tc_timer_check_node, list_node);

	pthread_mutex_lock(&check_node->check_mutex);
	list_for_each_entry_safe(timer_node, safe, &check_node->check_head, check_list_node) {
		pthread_mutex_unlock(&check_node->check_mutex);
		pthread_mutex_lock(&timer_node->tick_mutex);
		timer_node->tick++;
		//PRINT("id = %d, tick = %d, timer_sec = %d\n", timer_node->id, timer_node->tick, timer_node->timer_sec);
		if (timer_node->tick >= timer_node->timer_sec) {
			if (timer_node->status == TC_TIMER_STATUS_RUNNING)
				goto next;
			timer_node->status = TC_TIMER_STATUS_RUNNING;
			tc_thread_pool_node_add(
					global_timer_data.timer_handle_id, 
					&timer_node->list_node);
		}
next:
		pthread_mutex_unlock(&timer_node->tick_mutex);	
		pthread_mutex_lock(&check_node->check_mutex);
	}
	/*pthread_mutex_lock(&check_node->mutex);
	list_splice_init(&check_node->head, &check_node->check_head);
	pthread_mutex_unlock(&check_node->mutex);*/
	check_node->status = TC_TIMER_STATUS_IDLE;
	pthread_mutex_unlock(&check_node->check_mutex);
	
	return TC_OK;
}

static void
tc_timer_check_node_init()
{
	int i = 0;
	int num = TC_TIMER_SPLICE;

	pthread_mutex_init(&global_timer_data.check_list_mutex, NULL);
	for (; i < num; i++) {
		INIT_LIST_HEAD(&global_timer_data.check_list[i].head);
		INIT_LIST_HEAD(&global_timer_data.check_list[i].check_head);
		pthread_mutex_init(&global_timer_data.check_list[i].mutex, NULL);
		pthread_mutex_init(&global_timer_data.check_list[i].check_mutex, NULL);
	}
}

int
tc_timer_setup()
{
	int ret = 0;

	ret = tc_thread_pool_create(
				1,
				32 * 1024,
				"timer_count",
				NULL,
				tc_timer_count,
				NULL,
				&global_timer_data.timer_count_id);
	if (ret != TC_OK)
		return ret;

	ret = tc_thread_pool_create(
				global_timer_data.thread_num,
				global_timer_data.thread_stack,
				"timer_check",
				NULL,
				NULL,
				tc_timer_check, 
				&global_timer_data.timer_check_id);
	if (ret != TC_OK)
		return ret;
	/*
	 * we are not sure if we should make the timer handle threads' number 
	 * to be configurable. At present, just make it constant.
	 */
	ret = tc_thread_pool_create(
				global_timer_data.thread_num,
				global_timer_data.thread_stack,
				"timer_handle",
				NULL,
				NULL,
				tc_timer_handle,
				&global_timer_data.timer_handle_id);
	if (ret != TC_OK) 
		return ret;

	tc_timer_count_start();
	return TC_OK;
}

static void
tc_timer_list_data_destroy(
	struct tc_timer_list_data *list_data
)
{
	struct list_head *sl = NULL;
	struct tc_timer_data_node *data_node = NULL;

	if (!list_data)
		return;

	pthread_mutex_lock(&list_data->mutex);
	sl = list_data->head.next;
	while (sl != &list_data->head) {
		data_node = tc_list_entry(sl ,struct tc_timer_data_node, list_node);
		sl = sl->next;
		list_del_init(&data_node->list_node);
		TC_FREE(data_node);
	}
	pthread_mutex_unlock(&list_data->mutex);
	pthread_mutex_destroy(&list_data->mutex);
}

static int
tc_timer_uninit()
{
	
	tc_hash_destroy(global_timer_data.timer_hash);

	tc_timer_list_data_destroy(&global_timer_data.timer_list);
	return TC_OK;
}

static int
tc_timer_config_setup()
{
	global_timer_data.thread_num = TC_TIMER_SPLICE;
	global_timer_data.thread_stack = TC_THREAD_DEFALUT_STACK;

	TC_CONFIG_ADD(
		"timer_thread_num", 
		&global_timer_data.thread_num, 
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"timer_stack_size", 
		&global_timer_data.thread_stack,
		FUNC_NAME(INT));
	
	return TC_OK;
}


int
tc_timer_init()
{
	int ret = 0;

	memset(&global_timer_data, 0, sizeof(global_timer_data));

	INIT_LIST_HEAD(&global_timer_data.timer_list.head);
	INIT_LIST_HEAD(&global_timer_data.free_id_list.head);
	pthread_mutex_init(&global_timer_data.timer_list.mutex, NULL);
	pthread_mutex_init(&global_timer_data.free_id_list.mutex, NULL);
	pthread_mutex_init(&global_timer_data.timer_tick.mutex, NULL);
	pthread_mutex_init(&global_timer_data.timer_count.mutex, NULL);
	tc_timer_check_node_init();

	global_timer_data.timer_hash = 	tc_hash_create(
						TC_TIMER_TABLE_SIZE,		
						tc_timer_hash, 
						tc_timer_hash_get, 
						tc_timer_hash_destroy);
	if (global_timer_data.timer_hash == TC_HASH_ERR) 
		return TC_ERR;

	ret = tc_user_cmd_add(tc_timer_config_setup);
	if (ret != TC_OK)
		return ret;

	ret = tc_init_register(tc_timer_setup);
	if (ret != TC_OK)
		return ret;

	return tc_uninit_register(tc_timer_uninit);
}

TC_MOD_INIT(tc_timer_init);
