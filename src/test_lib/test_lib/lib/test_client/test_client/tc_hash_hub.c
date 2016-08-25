#include "tc_hash_hub.h"
#include "tc_hub_private.h"
#include "tc_err.h"
#include "tc_cmd.h"
#include "tc_comm.h"
#include "tc_init.h"
#include "tc_hash.h"
#include "tc_heap_timer.h"
#include "tc_print.h"
#include "tc_config.h"
#include "tc_thread.h"
#include "tc_timer_private.h"
#include "tc_create_private.h"
#include "tc_hash_hub.h"

struct tc_hash_hub_table {
	int status;
//	int next_pos;
	int send_times;
	int expire_time;
	pthread_mutex_t mutex;
	struct list_head hub_head;
	struct hlist_node node;
};

struct tc_hash_hub_list_data {
	int expire_time;
	struct list_head head;
	struct list_head node;
};

struct tc_hash_hub_node {
	int time_offset; 
	int expire_time;
	unsigned long extra_data;
	pthread_mutex_t extra_data_mutex;
	struct list_head node;
};

struct tc_hash_hub_config {
	int open_hub;
	int thread_num;
	int thread_stack;
	int hub_interval;
};

struct tc_hash_hub_data {
	int thread_id;
	unsigned long timer_id;
	int max_interval;
	int min_interval;
	int hub_interval;
	unsigned long *timer_id_array;
	time_t start_time;
	tc_hash_handle_t hub_hash;
	pthread_mutex_t hub_data_mutex;
};

static struct tc_hash_hub_data *global_hub_data = NULL;
static struct tc_hash_hub_config global_hub_config;

static int
tc_hash_hub_table_get(
	int interval,
	struct tc_hash_hub_table **hub_table
);

static int
tc_hash_hub_node_add(
	int 			interval,
	struct tc_hash_hub_table	*hub_table,
	struct tc_create_link_data *cl_data
);

/* 这个函数目前是想的很美好，实际实现呢，其实很悲催 */
int
tc_hash_hub_time_update(
	int expire_time,
	unsigned long user_data
)
{
	int ret = 0;
	int old_pos = 0, offset = 0, cur_pos = 0;
	struct hlist_node *hnode = NULL;
	struct tc_hash_hub_table *hub_table = NULL;
	struct tc_create_link_data *cl_data = NULL;
	struct tc_hash_hub_node *hub_node = NULL;

	cl_data = (struct tc_create_link_data*)user_data;

	hub_node = (struct tc_hash_hub_node *)cl_data->hub_data;
	//offset = old_pos - global_hub_data->hub_interval;

	hub_table = TC_HASH_GET(
				global_hub_data->hub_hash, 
				struct tc_hash_hub_table, 
				node, 
				hub_node->expire_time, 
				hub_node->expire_time);

	/* 从原来的表中删除 */
	pthread_mutex_lock(&hub_table->mutex);
	list_del_init(&hub_node->node);
	pthread_mutex_unlock(&hub_table->mutex);

	hub_table = NULL;
	cur_pos = expire_time - hub_node->expire_time + hub_table->expire_time;
	hub_node->expire_time = expire_time;
	ret = tc_hash_hub_table_get(cur_pos, &hub_table);
	if (ret != TC_OK)
		return ret;
	pthread_mutex_lock(&hub_table->mutex);
	list_add_tail(&hub_node->node, &hub_table->hub_head);
	pthread_mutex_unlock(&hub_table->mutex);

	return TC_OK;
}

void
tc_hash_hub_link_del(
	unsigned long hub_data
)
{
	struct tc_hash_hub_node *hub_node = NULL;

	if (!hub_data)
		return;

	hub_node = (struct tc_hash_hub_node *)hub_data;
	pthread_mutex_lock(&hub_node->extra_data_mutex);
	hub_node->extra_data = 0;
	pthread_mutex_unlock(&hub_node->extra_data_mutex);
}

static int
tc_hub_hash(
	struct hlist_node	*hnode,
	unsigned long		user_data
)
{
	char expire = 0;
	struct tc_hash_hub_table *hub_node = NULL;

	if (!hnode)
		expire = user_data;
	else {
		hub_node = tc_list_entry(hnode, struct tc_hash_hub_table, node);
		expire = hub_node->expire_time;
	}

	return (expire % global_hub_data->hub_interval);
}

static int
tc_hash_hub_get(
	struct hlist_node	*hnode,
	unsigned long		user_data	
)
{
	struct tc_hash_hub_table *hub_node = NULL;

	if (!hnode)
		return TC_ERR;

	hub_node = tc_list_entry(hnode, struct tc_hash_hub_table, node);
	if (hub_node->expire_time == (int)user_data)
		return TC_OK;

	return TC_ERR;
}

static int
tc_hash_hub_destroy(
	struct hlist_node *hnode
)
{
	struct tc_hash_hub_table *hub_node = NULL;

	if (!hnode)
		return TC_ERR;
	hub_node = tc_list_entry(hnode, struct tc_hash_hub_table, node);
}

static int
tc_hash_hub_uninit()
{
	return tc_hash_destroy(global_hub_data->hub_hash);
}

static int
tc_hash_hub_config_setup()
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

static int
tc_hash_hub_send_list(
	struct list_head *list_node
)
{
	int ret = 0, pos = 0;;
	struct list_head *sl = NULL;
	struct hlist_node *hnode = NULL;
	struct tc_hash_hub_table *table = NULL;
	struct tc_hash_hub_node *hub_node = NULL;
	struct tc_hash_hub_list_data *list_data = NULL;
	struct tc_create_link_data *cl_data = NULL;
	struct sockaddr_in addr;

	list_data = tc_list_entry(list_node, struct tc_hash_hub_list_data, node);
	sl = list_data->head.next;
	pos = list_data->expire_time;

	memset(&addr, 0, sizeof(addr));
	while (sl != &list_data->head) {
		hub_node = tc_list_entry(sl, struct tc_hash_hub_node, node);
		pthread_mutex_lock(&hub_node->extra_data_mutex);
		cl_data = (struct tc_create_link_data *)hub_node->extra_data;
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
			ret = cl_data->epoll_oper->harbor_func(cl_data->user_data);
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

	hnode = tc_hash_get(global_hub_data->hub_hash, pos, pos);
	table = tc_list_entry(hnode, struct tc_hash_hub_table, node);
	pthread_mutex_lock(&table->mutex);
	list_splice_init(&list_data->head, &table->hub_head);
	pthread_mutex_unlock(&table->mutex);

	TC_FREE(list_data);

	return TC_OK;
}

static void
tc_hash_hub_traversal(
	struct hlist_node	*hnode, 
	unsigned long		user_data
)
{
	struct tc_hash_hub_table *hub_table = NULL;
	struct tc_hash_hub_list_data *list_data = NULL;

	hub_table = tc_list_entry(hnode, struct tc_hash_hub_table, node);
	list_data = (struct tc_hash_hub_list_data*)calloc(1, sizeof(*list_data));
	if (!list_data) 
		TC_PANIC("Not Enough memory for %d\n", sizeof(*list_data));
	INIT_LIST_HEAD(&list_data->head);
	list_data->expire_time = hub_table->expire_time;
	pthread_mutex_lock(&hub_table->mutex);
	list_splice_init(&hub_table->hub_head, &list_data->head);
	pthread_mutex_unlock(&hub_table->mutex);

	tc_thread_pool_node_add(global_hub_data->thread_id, &list_data->node);
}

static int
tc_hash_hub_send_list_add(
	unsigned long user_data
)
{
	int cur_pos = 0;
	int next_pos = 0;
	int cur_tick = 0;
	struct hlist_node *hnode = NULL;

	cur_tick = tc_heap_timer_tick_get();
	cur_pos = cur_tick % global_hub_data->hub_interval;
	cur_pos += global_hub_data->hub_interval;

	PRINT("cur_pos = %d\n", cur_pos);
	return tc_hash_head_traversal(
				global_hub_data->hub_hash, 
				cur_pos, 
				user_data, 
				tc_hash_hub_traversal);
}

static int
tc_hash_hub_table_add(
	int interval,
	struct tc_hash_hub_table **table
)
{
	struct tc_hash_hub_table *hub_table = NULL;

	hub_table = (struct tc_hash_hub_table *)calloc(1, sizeof(struct tc_hash_hub_table));
	if (!hub_table) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	hub_table->expire_time = interval;
	INIT_LIST_HEAD(&hub_table->hub_head);
	pthread_mutex_init(&hub_table->mutex, NULL);
	(*table) = hub_table;

	return tc_hash_add(global_hub_data->hub_hash, &hub_table->node, interval);
}

static int
tc_hash_hub_cur_interval_get()
{
	int offset = 0;
	int interval = 0;
	int expire_time = 0;
	time_t cur_time = time(NULL);

	pthread_mutex_lock(&global_hub_data->hub_data_mutex);
	if (global_hub_data->start_time == 0) 
		global_hub_data->start_time = cur_time;
	else 
		offset = cur_time - global_hub_data->start_time;
	expire_time = global_hub_data->hub_interval;
	interval = expire_time + offset;
	if (global_hub_data->min_interval > interval)
		global_hub_data->min_interval = interval;
	if (global_hub_data->max_interval <= interval)
		global_hub_data->max_interval = interval + 1;
	pthread_mutex_unlock(&global_hub_data->hub_data_mutex);

	return interval;
}

static int
tc_hash_hub_node_add(
	int 			interval,
	struct tc_hash_hub_table	*hub_table,
	struct tc_create_link_data *cl_data
)
{
	struct tc_hash_hub_node *hub_node = NULL;

	hub_node = (struct tc_hash_hub_node *)calloc(1, sizeof(*hub_node));
	if (!hub_node)
		TC_PANIC("not enough memory: %s\n", strerror(errno));
	hub_node->expire_time = global_hub_data->hub_interval;
	hub_node->extra_data = (unsigned long)cl_data;
	cl_data->hub_data = (unsigned long)hub_node;
	pthread_mutex_init(&hub_node->extra_data_mutex, NULL);

	pthread_mutex_lock(&hub_table->mutex);
	list_add_tail(&hub_node->node, &hub_table->hub_head);
	pthread_mutex_unlock(&hub_table->mutex);

	return TC_OK;
}

static int
tc_hash_hub_table_get(
	int interval,
	struct tc_hash_hub_table **hub_table
)
{
	int ret = 0;
	struct hlist_node *hnode = NULL;

	hnode = tc_hash_get(global_hub_data->hub_hash, 
			    (unsigned long)interval, 
			    (unsigned long)interval);
	if (hnode) 
		(*hub_table) = tc_list_entry(hnode, struct tc_hash_hub_table, node);
	else
		ret = tc_hash_hub_table_add(interval, hub_table);

	return ret; 
}

int
tc_hash_hub_add(
	unsigned long user_data
)
{
	int id = 0;
	int ret = 0;
	int interval = 0, expire_time = 0;
	struct tc_hash_hub_table *hub_table = NULL;
	struct tc_create_link_data *cl_data = NULL;	

	if (!user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	cl_data = tc_create_link_data_get(user_data);

	interval = tc_hash_hub_cur_interval_get();

	ret = tc_hash_hub_table_get(interval, &hub_table);
	if (ret != TC_OK)
		return ret;

	tc_hash_hub_node_add(interval, hub_table, cl_data);

	id = interval % global_hub_data->hub_interval;
	PRINT("====================> id = %d, interval = %d\n", id, interval);
	if (global_hub_data->timer_id_array[id] == -1) {
		return tc_heap_timer_create( 
				global_hub_config.hub_interval, 
				TC_HEAP_TIMER_FLAG_CONSTANT, 
				id,
				tc_hash_hub_send_list_add, 
				NULL,
				&global_hub_data->timer_id_array[id]);
	}
	return TC_OK;
}

int
tc_hash_hub_create()
{
	int i = 0;
	int ret = 0;

	PRINT("open_hub = %d\n", global_hub_config.open_hub);
	if (global_hub_config.open_hub == 0)
		return TC_OK;

	global_hub_data = (struct tc_hash_hub_data *)calloc(1, sizeof(*global_hub_data));
	if (!global_hub_data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	global_hub_data->hub_interval = global_hub_config.hub_interval;
	pthread_mutex_init(&global_hub_data->hub_data_mutex, NULL);
	global_hub_data->timer_id_array = (unsigned long*)calloc(
						   global_hub_config.hub_interval, 
						   sizeof(unsigned long));
	if (!global_hub_data->timer_id_array) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	for (; i < global_hub_config.hub_interval; i++) 
		global_hub_data->timer_id_array[i] = -1;

	if (global_hub_config.thread_num <= 1)
		ret = tc_thread_pool_create(
					1,
					global_hub_config.thread_stack,
					"harbor", 
					NULL, 
					tc_hash_hub_send_list, 
					NULL,
					&global_hub_data->thread_id);
	else 
		ret = tc_thread_pool_create(
					global_hub_config.thread_num,
					global_hub_config.thread_stack,
					"harbor", 
					NULL,
					NULL,
					tc_hash_hub_send_list,
					&global_hub_data->thread_id);
	if (ret != TC_OK) 
		TC_PANIC("create thread pool error: %s\n", TC_CUR_ERRMSG_GET());

	ret = tc_heap_timer_create(
			1, 
			TC_HEAP_TIMER_FLAG_CONSTANT, 
			0, 
			tc_hash_hub_send_list_add, 
			NULL,
			&global_hub_data->timer_id);
	if (ret != TC_OK)
		TC_PANIC("create timer error: %s\n", TC_CUR_ERRMSG_GET());
	PRINT("hub_timer_id = %d, hub_interval = %d\n", 
			global_hub_data->timer_id, global_hub_config.hub_interval);

	global_hub_data->hub_hash = tc_hash_create(
						global_hub_config.hub_interval, 
						tc_hub_hash, 
						tc_hash_hub_get, 
						tc_hash_hub_destroy);
	if (global_hub_data->hub_hash == (void*)TC_HASH_ERR) 
		return TC_ERR;

	return TC_OK;
}

int
tc_hash_hub_init()
{
	int ret = 0;

	memset(&global_hub_config, 0, sizeof(global_hub_config));
	ret = tc_user_cmd_add(tc_hash_hub_config_setup);
	if (ret != TC_OK)
		return ret;

	ret = tc_init_register(tc_hash_hub_create);
	if (ret != TC_OK)
		return ret;

	return tc_uninit_register(tc_hash_hub_uninit);
}

TC_MOD_INIT(tc_hash_hub_init);

