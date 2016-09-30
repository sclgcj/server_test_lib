#include "tc_hub_private.h"
#include "tc_std_comm.h"
#include "tc_init_private.h"
#include "tc_cmd.h"
#include "tc_err.h"
#include "tc_hash.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_timer.h"
#include "tc_heap_timer.h"
#include "tc_timer_list_private.h"
#include "tc_thread.h"
#include "tc_create_private.h"

#if 0
#define TC_HUB_TABLE_SIZE 150000
struct tc_hub_table {
	int status;
	int next_pos;
	int send_times;
	int expire_time;
	struct list_head hub_head;
};

struct tc_hub_list_data {
	int expire_time;
	struct list_head head;
	struct list_head node;
};

struct tc_hub_node {
	int time_offset; 
	int expire_time;
	unsigned long extra_data;
	pthread_mutex_t extra_data_mutex;
	struct list_head node;
};

struct tc_hub_config {
	int open_hub;
	int thread_num;
	int thread_stack;
	int hub_interval;
};

struct tc_hub_data {
	int thread_id;
	unsigned long timer_id;
	int max_interval;
	int min_interval;
	int hub_interval;
	time_t start_time;
	tc_hash_handle_t hub_hash;
	pthread_mutex_t hub_data_mutex;
	pthread_mutex_t hub_table_mutex[TC_HUB_TABLE_SIZE];
	struct tc_hub_table hub_table[TC_HUB_TABLE_SIZE];
};

enum {
	TC_HUB_STATUS_IDLE,
	TC_HUB_STATUS_RUNNINIG,
	TC_HUB_STATUS_MAX
};

static struct tc_hub_data *global_hub_data = NULL;
static struct tc_hub_config global_hub_config;

static void
tc_hub_node_add(
	int interval, 
	int expire_time,
	struct tc_hub_node *hub_node
);

static void
tc_hub_node_del(
	int expire_time,
	struct tc_hub_node *hub_node
)
{
	int prev = 0;
	int old_pos = hub_node->expire_time;

	pthread_mutex_lock(&global_hub_data->hub_table_mutex[old_pos]);
	list_del_init(&hub_node->node);	
	pthread_mutex_unlock(&global_hub_data->hub_table_mutex[old_pos]);
}

int
tc_hub_time_update(
	int expire_time,
	unsigned long extra_data
)
{
	int save_time = 0;
	int old_pos = 0, cur_pos = 0;
	struct tc_create_link_data *cl_data = NULL;
	struct tc_hub_node *hub_node = NULL;

	cl_data = (struct tc_create_link_data*)extra_data;

	hub_node = (struct tc_hub_node *)cl_data->hub_data;
	old_pos = hub_node->expire_time;
	if (hub_node->expire_time - hub_node->time_offset == expire_time)
		return TC_OK;

	cur_pos = expire_time + hub_node->time_offset;
	pthread_mutex_lock(&cl_data->private_link_data.mutex);
	save_time = cl_data->private_link_data.hub_interval;
	cl_data->private_link_data.hub_interval = expire_time;
	pthread_mutex_unlock(&cl_data->private_link_data.mutex);



	tc_hub_node_add(cur_pos, expire_time, hub_node);

	return TC_OK;
}

void
tc_hub_link_del(
	unsigned long hub_data
)
{
	struct tc_hub_node *hub_node = NULL;

	if (!hub_data)
		return;

	hub_node = (struct tc_hub_node *)hub_data;
	pthread_mutex_lock(&hub_node->extra_data_mutex);
	hub_node->extra_data = 0;
	pthread_mutex_unlock(&hub_node->extra_data_mutex);
}

static void
tc_hub_node_add(
	int interval, 
	int expire_time,
	struct tc_hub_node *hub_node
)
{
	int prev = interval - expire_time;

	//PRINT("interval = %d, prev = %d, expire_time = %d\n", interval, prev, expire_time);
	pthread_mutex_lock(&global_hub_data->hub_table_mutex[interval]);
	global_hub_data->hub_table[interval].expire_time = expire_time;
	if (prev >= expire_time) {
		while (prev >= expire_time && 
				global_hub_data->hub_table[prev].expire_time == 0) 
			prev -= expire_time;
		if (prev >= expire_time)
			global_hub_data->hub_table[prev].next_pos = interval;
		else {
			prev += expire_time;
			global_hub_data->hub_table[prev].next_pos = interval;
		}
	}
	list_add_tail(&hub_node->node, &global_hub_data->hub_table[interval].hub_head);
	pthread_mutex_unlock(&global_hub_data->hub_table_mutex[interval]);
}

int
tc_hub_add(
	unsigned long user_data
)
{
	int offset = 0;
	int interval = 0, expire_time = 0;
	time_t cur_time = time(NULL);
	struct tc_hub_node *hub_node = NULL;
	struct tc_create_link_data *cl_data = NULL;	

	if (!user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	cl_data = tc_create_link_data_get(user_data);
	//cl_data = (struct tc_create_link_data *)user_data;
	pthread_mutex_lock(&global_hub_data->hub_data_mutex);
	if (global_hub_data->start_time == 0) 
		global_hub_data->start_time = cur_time;
	else 
		offset = cur_time - global_hub_data->start_time;
	pthread_mutex_lock(&cl_data->private_link_data.mutex);
	expire_time = cl_data->private_link_data.hub_interval;
	pthread_mutex_unlock(&cl_data->private_link_data.mutex);
	interval = expire_time + offset;
	if (global_hub_data->min_interval > interval)
		global_hub_data->min_interval = interval;
	if (global_hub_data->max_interval <= interval)
		global_hub_data->max_interval = interval + 1;
	pthread_mutex_unlock(&global_hub_data->hub_data_mutex);
	
	hub_node = (struct tc_hub_node *)calloc(1, sizeof(*hub_node));
	if (!hub_node)
		TC_PANIC("not enough memory: %s\n", strerror(errno));
	hub_node->extra_data = (unsigned long)cl_data;
//	hub_node->time_offset = offset;
//	hub_node->expire_time = interval;
	cl_data->hub_data = (unsigned long)hub_node;
	pthread_mutex_init(&hub_node->extra_data_mutex, NULL);

	tc_hub_node_add(interval, expire_time, hub_node);

	return TC_OK;
}

static int
tc_hub_send_list(
	struct list_head *list_node
)
{
	int ret = 0, pos = 0;;
	struct list_head *sl = NULL;
	struct tc_hub_node *hub_node = NULL;
	struct tc_hub_list_data *list_data = NULL;
	struct tc_create_link_data *cl_data = NULL;
	struct sockaddr_in addr;

	list_data = tc_list_entry(list_node, struct tc_hub_list_data, node);
	sl = list_data->head.next;
	pos = list_data->expire_time;

	PRINT("\n");
	memset(&addr, 0, sizeof(addr));
	while (sl != &list_data->head) {
		hub_node = tc_list_entry(sl, struct tc_hub_node, node);
		pthread_mutex_lock(&hub_node->extra_data_mutex);
		cl_data = (struct tc_create_link_data *)hub_node->extra_data;
		PRINT("\n");
		if (!cl_data) {
			sl = sl->next;
			list_del_init(&hub_node->node);
			pthread_mutex_unlock(&hub_node->extra_data_mutex);
			pthread_mutex_destroy(&hub_node->extra_data_mutex);
			TC_FREE(hub_node);
			continue;
		}
		if (cl_data->epoll_oper->harbor_func)  {
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = cl_data->link_data.peer_addr.s_addr;
			addr.sin_port = htons(cl_data->link_data.peer_port);
			ret = cl_data->epoll_oper->harbor_func(
						cl_data->user_data
						);
			if (ret != TC_OK) {
				pthread_mutex_unlock(&hub_node->extra_data_mutex);
				sl = sl->next;
				list_del_init(&hub_node->node);
				tc_create_link_data_del(cl_data);
				TC_FREE(hub_node);
				continue;
			}
		}
		pthread_mutex_unlock(&hub_node->extra_data_mutex);
		sl = sl->next;
	}
	pthread_mutex_lock(&global_hub_data->hub_table_mutex[pos]);
	list_splice_init(&list_data->head, &global_hub_data->hub_table[pos].hub_head);
	global_hub_data->hub_table[pos].status = TC_HUB_STATUS_IDLE;
	pthread_mutex_unlock(&global_hub_data->hub_table_mutex[pos]);

	TC_FREE(list_data);

	return TC_OK;
}

static int
tc_hub_send_list_add(
	unsigned long user_data
)
{
	int cur_pos = 0;
	int cur_tick = 0;
	int next_pos = 0;
	int min_interval = 0, max_interval = 0;
	struct tc_hub_list_data *list_data = NULL;

	cur_tick = tc_heap_timer_tick_get();
	cur_pos = cur_tick % global_hub_data->hub_interval;
	cur_pos += global_hub_data->hub_interval;
	pthread_mutex_lock(&global_hub_data->hub_data_mutex);
	min_interval = global_hub_data->min_interval;
	max_interval = global_hub_data->max_interval;
	pthread_mutex_unlock(&global_hub_data->hub_data_mutex);

	while (1) {
		pthread_mutex_lock(&global_hub_data->hub_table_mutex[cur_pos]);
		if (global_hub_data->hub_table[cur_pos].expire_time == 0 || 
		    global_hub_data->hub_table[cur_pos].send_times + cur_pos > cur_tick || 
		    global_hub_data->hub_table[cur_pos].status == TC_HUB_STATUS_RUNNINIG || 
		    list_empty(&global_hub_data->hub_table[cur_pos].hub_head)) {
			goto next;
		}
		global_hub_data->hub_table[cur_pos].status = TC_HUB_STATUS_RUNNINIG;
		//global_hub_data->hub_table[cur_pos].
		list_data = (struct tc_hub_list_data *)calloc(1, sizeof(*list_data));
		if (!list_data) 
			TC_PANIC("not enough memory\n");
		INIT_LIST_HEAD(&list_data->head);
		list_data->expire_time = cur_pos;
		global_hub_data->hub_table[cur_pos].send_times += 
				global_hub_data->hub_table[cur_pos].expire_time;
		list_splice_init(
				&global_hub_data->hub_table[cur_pos].hub_head, 
				&list_data->head);
		tc_thread_pool_node_add(global_hub_data->thread_id, &list_data->node);
next:
		next_pos = global_hub_data->hub_table[cur_pos].next_pos;
		pthread_mutex_unlock(&global_hub_data->hub_table_mutex[cur_pos]);
		cur_pos = next_pos;
		if (cur_pos == -1)
			break;
	}

	return TC_OK;
}

static void
tc_hub_data_init( 
	int hub_interval 
)
{
	int i = 0;

	global_hub_data->hub_interval = hub_interval;
	pthread_mutex_init(&global_hub_data->hub_data_mutex, NULL);

	for (; i < TC_HUB_TABLE_SIZE; i++) {
		global_hub_data->hub_table[i].next_pos = -1;
		global_hub_data->hub_table[i].send_times = 0;
		global_hub_data->hub_table[i].expire_time = 0;
		INIT_LIST_HEAD(&global_hub_data->hub_table[i].hub_head);
		pthread_mutex_init(&global_hub_data->hub_table_mutex[i], NULL);
	}
}

static int
tc_hub_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	char expire = 0;
	struct tc_hub_node *hub_node = NULL;

	if (!hnode)
		expire = user_data;
	else {
		hub_node = tc_list_entry(hnode, struct tc_hub_node, node);
		expire = hub_node->expire_time;
	}

	return (expire % global_hub_data->hub_interval);
}

static int
tc_hub_hash_get(
	struct hlist_node	*hnode,
	unsigned long		user_data	
)
{
	struct tc_hub_node *hub_node = NULL;

	if (!hnode)
		return TC_ERR;

	hub_node = tc_list_entry(hnode, struct tc_hub_node, node);
	if (hub_node->expire_time == (int)user_data)
		return TC_OK;

	return TC_ERR;
}

static int
tc_hub_hash_destroy(
	struct hlist_node *hnode
)
{
	struct tc_hub_node *hub_node = NULL;

	if (!hnode)
		return TC_ERR;
	hub_node = tc_list_entry(hnode, struct tc_hub_node, node);
}

int
tc_hub_create()
{
	int ret = 0;

	if (global_hub_config.open_hub == 0)
		return TC_OK;

	global_hub_data = (struct tc_hub_data *)calloc(1, sizeof(*global_hub_data));

	if (global_hub_config.thread_num <= 1)
		ret = tc_thread_pool_create(
					global_hub_config.thread_num,
					global_hub_config.thread_stack,
					"harbor", 
					NULL, 
					tc_hub_send_list, 
					NULL,
					&global_hub_data->thread_id);
	else 
		ret = tc_thread_pool_create(
					global_hub_config.thread_num,
					global_hub_config.thread_stack,
					"harbor", 
					NULL,
					NULL,
					tc_hub_send_list,
					&global_hub_data->thread_id);
	if (ret != TC_OK) 
		TC_PANIC("create thread pool error: %s\n", TC_CUR_ERRMSG_GET());

	ret = tc_heap_timer_create(
			1, 
			TC_HEAP_TIMER_FLAG_CONSTANT, 
			0, 
			tc_hub_send_list_add, 
			NULL,
			&global_hub_data->timer_id);
	if (ret != TC_OK)
		TC_PANIC("create timer error: %s\n", TC_CUR_ERRMSG_GET());
	PRINT("hub_timer_id = %d\n", global_hub_data->timer_id);

	tc_hub_data_init(global_hub_config.hub_interval);
//	global_hub_data->hub_interval = hub_interval;
	return TC_OK;
}

static void
tc_hub_list_destroy(
	struct list_head *head	
)
{
	struct list_head *sl = NULL;
	struct tc_hub_node *hub_node = NULL;
	struct tc_create_link_data *cl_data = NULL;

	sl = head->next;
	while (sl != head) {
		hub_node = list_entry(sl, struct tc_hub_node, node);
		pthread_mutex_destroy(&hub_node->extra_data_mutex);
		if (hub_node->extra_data) {
			cl_data = (struct tc_create_link_data *)hub_node->extra_data;
			cl_data->hub_data = 0;
		}
		list_del_init(sl);
		TC_FREE(hub_node);
		sl = head->next;
	}
}

static int
tc_hub_destroy()
{
	int i = 0;

	pthread_mutex_destroy(&global_hub_data->hub_data_mutex);

	for (; i < TC_HUB_TABLE_SIZE; i++) {
		tc_hub_list_destroy(&global_hub_data->hub_table[i].hub_head);
		pthread_mutex_destroy(&global_hub_data->hub_table_mutex[i]);
	}

	tc_heap_timer_destroy(global_hub_data->timer_id);
}

static int
tc_hub_config_setup()
{
	global_hub_config.thread_num = TC_THREAD_DEFAULT_NUM;
	global_hub_config.thread_stack = TC_THREAD_DEFALUT_STACK;

	TC_CONFIG_ADD(
		"hub_thread_num", 
		&global_hub_config.thread_num, 
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"hub_stack_size", 
		&global_hub_config.thread_stack, 
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"hub_interval",
		&global_hub_config.hub_interval,
		FUNC_NAME(INT));
	TC_CONFIG_ADD(
		"open_hub",
		&global_hub_config.open_hub, 
		FUNC_NAME(INT));

	return TC_OK;
}

int
tc_hub_init()
{
	int ret = 0;

	memset(&global_hub_config, 0, sizeof(global_hub_config));
	ret = tc_user_cmd_add(tc_hub_config_setup);
	if (ret != TC_OK)
		return ret;

	ret = tc_local_init_register(tc_hub_create);
	if (ret != TC_OK)
		return ret;

	return tc_uninit_register(tc_hub_destroy);
}

TC_MOD_INIT(tc_hub_init);
#endif
