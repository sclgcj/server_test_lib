#include "tc_comm.h"
#include "tc_err.h"
#include "tc_init.h"
#include "tc_print.h"

struct tc_thread_data {
	char *hd_thread_name;
	int  hd_group_id;
	int  hd_member_id;
};

struct tc_thread_member {
	pthread_mutex_t tm_mutex;
	pthread_cond_t  tm_cond;
	struct list_head tm_head;
	struct list_head tm_node;
};

struct tc_thread_group {
	int tg_count;
	struct tc_thread_member *tg_member;
	void (*tg_group_free)(struct list_head *node);
	int  (*tg_group_func)(struct list_head *node);
	int  (*tg_execute_func)(struct list_head *node);
	pthread_cond_t tg_cond;
	pthread_mutex_t tg_mutex;
	struct list_head tg_head;
};


#define TC_THREAD_GROUP_INIT 256
static pthread_condattr_t global_cond_attr;
static pthread_mutex_t global_group_mutex = PTHREAD_MUTEX_INITIALIZER;
static int global_thread_group_num = 0, global_thread_group_count = 0;
static int global_thread_num = 0, global_thread_exit = 0;
static struct tc_thread_group *global_thread_group = NULL;

int
tc_thread_test_exit()
{
	int ret = TC_ERR;
	pthread_mutex_lock(&global_group_mutex);
	if (global_thread_exit) {
		if (global_thread_group_num > 0)
			global_thread_group_num--;
		ret = TC_OK;
	}
	pthread_mutex_unlock(&global_group_mutex);

	return ret;
}

static void
tc_thread_start()
{
	pthread_mutex_lock(&global_group_mutex);
	global_thread_group_num++;
	pthread_mutex_unlock(&global_group_mutex);
}

static int
tc_get_thread_node(
	struct list_head *head,
	struct list_head **node
)
{
	if (list_empty(head)) 
		return TC_ERR;

	(*node) = head->next;
	list_del_init(*node);

	return TC_OK;
}

static int
tc_get_thread_group_node(
	int id,
	struct list_head **sl
)
{
	int ret = 0;
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_sec += 1;
	pthread_mutex_lock(&global_thread_group[id].tg_mutex);
	ret = tc_get_thread_node(&global_thread_group[id].tg_head, sl);
	if (ret != TC_OK) {
		ret = pthread_cond_timedwait(
					&global_thread_group[id].tg_cond, 
					&global_thread_group[id].tg_mutex, 
					&ts);
		if (ret == ETIMEDOUT) 
			ret = TC_TIMEOUT;
		ret = tc_get_thread_node(&global_thread_group[id].tg_head, sl);

	}
	pthread_mutex_unlock(&global_thread_group[id].tg_mutex);

	return ret;
}

static int
tc_add_thread_member_node(
	struct list_head *node,
	struct tc_thread_member *thread_memb
)
{
	int ret = 0;

	pthread_mutex_lock(&thread_memb->tm_mutex);
	list_add_tail(node, &thread_memb->tm_head);
	pthread_cond_broadcast(&thread_memb->tm_cond);
	pthread_mutex_unlock(&thread_memb->tm_mutex);

	return TC_OK;
}

static void *
tc_master_thread(
	void *arg
)
{
	int i = 0, id = 0, ret = 0, count = 0;
	char *name = NULL;
	struct list_head *sl = NULL;
	struct tc_thread_data *thread_data = NULL;

	thread_data = (struct tc_thread_data*)arg;
	id = thread_data->hd_group_id;
	if (thread_data->hd_thread_name) {
		name = (char*)calloc(256, sizeof(char));
		if (!name) 
			TC_PANIC("no enough memory\n");
		prctl(PR_SET_NAME, name);
		TC_FREE(name);
	}
	TC_FREE(arg);
	//PRINT("id = %d\n", id);
	while (1) {
		ret = tc_thread_test_exit();
		if (ret == TC_OK)
			break;
		ret = tc_get_thread_group_node(id, &sl);
		if (ret != TC_OK || !sl)
			continue;
		if (global_thread_group[id].tg_group_func) {
			ret = global_thread_group[id].tg_group_func(sl);
			if (ret != TC_OK)
				continue;
		}
		if (global_thread_group[id].tg_count <= 1)
			continue;

		for (i = count; i < global_thread_group[id].tg_count; i++) {
			ret = tc_add_thread_member_node(sl, &global_thread_group[id].tg_member[i]);
			if (ret == TC_OK)
				break;
		}
		if (i + 1 == global_thread_group[id].tg_count || 
				i == global_thread_group[id].tg_count) 
			i = 0;
		i++;
		count = i;
		if (count == global_thread_group[id].tg_count)
			count = 0;
	}

	return NULL;
}

static int
tc_master_thread_create(
	char *thread_name,
	int  thread_id,
	pthread_attr_t *attr,
	struct tc_thread_group *group
)
{
	pthread_t th;
	int *id = NULL;
	struct tc_thread_data *thread_data = NULL;

	thread_data = (struct tc_thread_data*)calloc(1, sizeof(*thread_data));
	if (!thread_data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	thread_data->hd_group_id = thread_id;
	thread_data->hd_thread_name = thread_name;

	if (pthread_create(&th, attr, tc_master_thread, (void*)thread_data) < 0) {
		TC_ERRNO_SET(TC_CREATE_THREAD_ERR);
		return TC_ERR;
	}
	tc_thread_start();

	return TC_OK;
}

static struct tc_thread_data *
tc_thread_data_calloc(
	int pos,
	int group_id,
	char *thread_name
)
{
	struct tc_thread_data *thread_data = NULL;

	thread_data = (struct tc_thread_data*)
				calloc(1, sizeof(*thread_data));
	if (!thread_data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	thread_data->hd_member_id = pos;
	thread_data->hd_group_id  = group_id;
	thread_data->hd_thread_name = thread_name;

	return thread_data;
}

static void
tc_thread_member_init(
	struct tc_thread_member *memb
)
{
	pthread_mutex_init(&memb->tm_mutex, NULL);
	pthread_cond_init(&memb->tm_cond, &global_cond_attr);
	INIT_LIST_HEAD(&memb->tm_head);
}

static int
tc_get_thread_member_node(
	int group_id,
	int memb_id,
	struct list_head **node
)
{
	int ret = TC_OK;
	struct timespec ts;
	struct tc_thread_member *thread_memb = NULL;

	thread_memb = &global_thread_group[group_id].tg_member[memb_id];
	pthread_mutex_lock(&thread_memb->tm_mutex);
	ret = tc_get_thread_node(&thread_memb->tm_head, node);
	if (ret != TC_OK) {
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ts.tv_sec += 1;
		ret = pthread_cond_timedwait(
					&thread_memb->tm_cond, 
					&thread_memb->tm_mutex, 
					&ts);
		if (ret == ETIMEDOUT) 
			ret = TC_TIMEOUT;
		else
			ret = tc_get_thread_node(&thread_memb->tm_head, node);
	}
	pthread_mutex_unlock(&thread_memb->tm_mutex);

	return ret;
}

static void *
tc_thread_member_handle(
	void *arg
)
{
	int ret = 0;
	struct list_head *sl = NULL;
	struct tc_thread_data *thread_data = NULL;

	thread_data = (struct tc_thread_data*)arg;
	if (thread_data->hd_thread_name)
		prctl(PR_SET_NAME, thread_data->hd_thread_name);

	while (1) {
		ret = tc_thread_test_exit();
		if (ret == TC_OK)
			break;
		ret = tc_get_thread_member_node(
					thread_data->hd_group_id,
					thread_data->hd_member_id,
					&sl);
		if (ret != TC_OK || !sl)
			continue;

		if (global_thread_group[thread_data->hd_group_id].tg_execute_func) {
			global_thread_group[thread_data->hd_group_id].tg_execute_func(sl);
		}
	}

	TC_FREE(thread_data);
	pthread_exit(NULL);
}

static int
tc_member_thread_create(
	char *thread_name,
	int  group_id,
	pthread_attr_t *attr,
	struct tc_thread_group *group
)
{
	int i = 0;	
	pthread_t th;
	struct tc_thread_data *thread_data = NULL;

	group->tg_member = (struct tc_thread_member*)
				calloc(group->tg_count, sizeof(*group->tg_member));
	if (!group->tg_member) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}

	for (; i < group->tg_count; i++) {
		thread_data = tc_thread_data_calloc(i, group_id, thread_name);
		if (!thread_data)
			return TC_ERR;
		tc_thread_member_init(&global_thread_group[group_id].tg_member[i]);
		if (pthread_create(&th, attr, tc_thread_member_handle, thread_data) < 0) {
			TC_ERRNO_SET(TC_CREATE_THREAD_ERR);
			return TC_ERR;
		}
		tc_thread_start();
	}

	return TC_OK;
}

int
tc_thread_pool_create(
	int count,
	int stack_size,
	char *thread_name,
	void (*group_free)(struct list_head *node),
	int  (*group_func)(struct list_head *node),
	int  (*execute_func)(struct list_head *node),
	int  *group_id
)
{
	int id = 0;
	int ret = 0;
	pthread_attr_t attr; 
	
	pthread_mutex_lock(&global_group_mutex);
	id = global_thread_group_count++;
	pthread_mutex_unlock(&global_group_mutex);
	if (group_id)
		(*group_id) = id;
	global_thread_group[(id)].tg_count = count;
	global_thread_group[(id)].tg_execute_func = execute_func;
	global_thread_group[(id)].tg_group_free = group_free;
	global_thread_group[(id)].tg_group_func = group_func;

	//PRINT("count = %d\n", count);
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stack_size);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret = tc_master_thread_create(thread_name, id, &attr, &global_thread_group[(id)]);
	if (ret != TC_OK)
		return ret;

	if (count <= 1)
		return TC_OK;

	return tc_member_thread_create(thread_name, id, &attr, &global_thread_group[(id)]);
}

void
tc_thread_exit_wait()
{
	int sec = 5;
	int num = 0;

	pthread_mutex_lock(&global_group_mutex);
	global_thread_exit = 1;
	pthread_mutex_unlock(&global_group_mutex);
	while(sec) {
		pthread_mutex_lock(&global_group_mutex);
		num = global_thread_group_num;
		pthread_mutex_unlock(&global_group_mutex);
		if (num == 0)
		{
			break;
		}
		sleep(1);
		sec--;
	}
}

static int
tc_thread_uninit()
{
	int i = 0, j = 0;
	struct list_head *sl = NULL;

	/*
	 * Here, we don't think about a better way to handle the thread exit.
	 * This is just a temporary resolution, if we find a better one, we 
	 * will change it.
	 */
	pthread_mutex_lock(&global_group_mutex);
	for (; i < global_thread_group_num; i++) {
		if (global_thread_group[i].tg_count <= 1)
			continue;
		for (j = 0; j < global_thread_group[i].tg_count; j++) {
			if (!global_thread_group[i].tg_member)
				continue;
			if (list_empty(&global_thread_group[i].tg_member[j].tm_head))
				continue;
			sl = global_thread_group[i].tg_member[j].tm_head.next;
			while (sl != &global_thread_group[i].tg_member[j].tm_head) {
				list_del_init(sl);
				if (global_thread_group[i].tg_group_free)
					global_thread_group[i].tg_group_free(sl);
				sl = global_thread_group[i].tg_member[j].tm_head.next;
			}
			pthread_mutex_destroy(&global_thread_group[i].tg_member[j].tm_mutex);
			pthread_cond_destroy(&global_thread_group[i].tg_member[j].tm_cond);
		}
	}
	pthread_mutex_unlock(&global_group_mutex);

	TC_FREE(global_thread_group);

	return TC_OK;
}

int
tc_thread_pool_node_add(
	int  group_id,
	struct list_head *node
)
{
	int ret = 0;

	pthread_mutex_lock(&global_thread_group[group_id].tg_mutex);
	list_add_tail(node, &global_thread_group[group_id].tg_head);
	pthread_cond_broadcast(&global_thread_group[group_id].tg_cond);
	pthread_mutex_unlock(&global_thread_group[group_id].tg_mutex);

	return TC_OK;
}

int
tc_thread_init()
{
	int i = 0;
	global_thread_group_num = TC_THREAD_GROUP_INIT;
	pthread_condattr_init(&global_cond_attr);
	pthread_condattr_setclock(&global_cond_attr, CLOCK_MONOTONIC);

	global_thread_group = (struct tc_thread_group*)
				calloc(global_thread_group_num, sizeof(*global_thread_group));
	if (!global_thread_group) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	for (; i < global_thread_group_num; i++) {
		pthread_cond_init(&global_thread_group[i].tg_cond, &global_cond_attr);
		pthread_mutex_init(&global_thread_group[i].tg_mutex, NULL);
		INIT_LIST_HEAD(&global_thread_group[i].tg_head);
	}

	return tc_uninit_register(tc_thread_uninit);
}

TC_MOD_INIT(tc_thread_init);
