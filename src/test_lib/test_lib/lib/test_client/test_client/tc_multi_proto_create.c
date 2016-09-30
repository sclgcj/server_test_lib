#include "tc_std_comm.h"
#include "tc_err.h"
#include "tc_init_private.h"
#include "tc_print.h"
#include "tc_print.h"
#include "tc_hash.h"

/*
 * 目前的实现前提是：同时使用的协议数量比较少。
 * 本想使用hash的，但是hash需要外部传入参数，
 * 使用起来比较麻烦，而且就目前情况来说，可能
 * 同时使用的协议不多，可以考虑把hash换成链表。
 * 希望以后能找到既能少传参数又能满足快速访问
 * 多个协议中的某一个的方法
 */

struct tc_multi_proto_node {
	char *proto;
	char *data;
	int realloc;
	int data_cnt;
	int data_size;
	pthread_mutex_t mutex;
	struct list_head lnode;
	struct hlist_node node;
};

#define TC_CREATE_PROTO_HASH_NUM 32
struct tc_multi_proto_data {
	int proto_cnt;
	struct list_head proto_list;
	tc_hash_handle_t mpd_hash;
	pthread_mutex_t list_mutex;
	pthread_mutex_t proto_mutex;
};

static struct tc_multi_proto_data global_multi_proto_data;

static struct tc_multi_proto_node *
tc_multi_proto_node_create(
	int data_size,
	int data_cnt,
	char *proto
)
{
	struct tc_multi_proto_node *pnode = NULL;	

	pnode = (struct tc_multi_proto_node *)calloc(1, sizeof(*pnode));
	if (!pnode) 
		TC_PANIC("Not enough memory for %d bytes\n", sizeof(*pnode));

	pnode->proto = strdup(proto);
	pnode->data_size = data_size;
	pnode->data_cnt = data_cnt;
	pnode->data = (char*)calloc(data_cnt, data_size);
	if (!pnode->data) 
		TC_PANIC("Not enough memory for %d bytes\n", data_cnt * data_size);
	pthread_mutex_init(&pnode->mutex, NULL);

	return pnode;
}

static void
tc_multi_proto_node_destroy(
	struct tc_multi_proto_node *pnode
)
{
	TC_FREE(pnode->proto);
	TC_FREE(pnode->data);
	TC_FREE(pnode);
	pthread_mutex_destroy(&pnode->mutex);
}

int
tc_multi_proto_add(
	int  data_size,
	int  data_cnt,
	char *proto
)
{
	int ret = 0;
	struct tc_multi_proto_node *pnode = NULL;	

	pnode = tc_multi_proto_node_create(data_size, data_cnt, proto);

	ret = tc_hash_add(global_multi_proto_data.mpd_hash, 
			  &pnode->node, (unsigned long)proto);
	if (ret != TC_OK) {
		tc_multi_proto_node_destroy(pnode);
		return ret;
	}

	pthread_mutex_lock(&global_multi_proto_data.list_mutex);
	list_add_tail(&pnode->lnode, &global_multi_proto_data.proto_list);
	pthread_mutex_unlock(&global_multi_proto_data.list_mutex);

	return TC_OK;
}

void
tc_multi_proto_user_data_get(
	int id,
	char *proto,
	unsigned long *user_data
)
{
	int data_size = 0;
	struct hlist_node *hnode = NULL;		
	struct tc_multi_proto_node *pnode = NULL;

	hnode = tc_hash_get(global_multi_proto_data.mpd_hash, 
			    (unsigned long)proto,
			    (unsigned long)proto);
	if (!hnode) 
		return;

	pnode = tc_list_entry(hnode, struct tc_multi_proto_node, node);
	data_size = pnode->data_size;	
	(*user_data) = (unsigned long)&pnode->data[id * data_size];
}

#define ADDRINPATH(cur, start, end) \
	((cur >= start && cur <= end) || (cur >= end && cur <= start))

void
tc_multi_proto_data_id_get(
	unsigned long user_data,
	int	      *id
)
{
	int ret = 0;
	int end = 0;
	struct list_head *sl = NULL;
	struct tc_multi_proto_node *pnode = NULL;

	sl = global_multi_proto_data.proto_list.next;	
	pthread_mutex_lock(&global_multi_proto_data.list_mutex);
	while(sl != &global_multi_proto_data.proto_list) {
		pnode = tc_list_entry(sl, struct tc_multi_proto_node, lnode);
		end = pnode->data_cnt * pnode->data_size;
		ret = ADDRINPATH(user_data, 
				 (unsigned long)pnode->data, 
				 (unsigned long)&pnode->data[end-1]);
		if (ret) {
			*id = labs(user_data - (unsigned long)pnode->data) / pnode->data_size;
			break;
		}
		sl = sl->next;
	}
	pthread_mutex_unlock(&global_multi_proto_data.list_mutex);
}

static int
tc_multi_proto_hash(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	int proto = 0;
	struct tc_multi_proto_node *pnode = NULL;

	if (!hnode)
		proto = ((char*)user_data)[0];
	else {
		pnode = tc_list_entry(hnode, struct tc_multi_proto_node, node);
		if (pnode->proto)
			proto = pnode->proto[0];
	}

	return (proto % TC_CREATE_PROTO_HASH_NUM);
}

static int
tc_multi_proto_hash_get(
	struct hlist_node *hnode,
	unsigned long user_data
)
{
	struct tc_multi_proto_node *pnode = NULL;

	pnode = tc_list_entry(hnode, struct tc_multi_proto_node, node);
	if (!strcmp((char*)user_data, pnode->proto))
		return TC_OK;

	return TC_ERR;
}

static int
tc_multi_proto_hash_destroy(
	struct hlist_node *hnode
)
{
	struct tc_multi_proto_node *pnode = NULL;

	pnode = tc_list_entry(hnode, struct tc_multi_proto_node, node);
	tc_multi_proto_node_destroy(pnode);

	return TC_OK;
}

int
tc_multi_proto_init()
{
	memset(&global_multi_proto_data, 0, sizeof(global_multi_proto_data));
	pthread_mutex_init(&global_multi_proto_data.proto_mutex, NULL);
	global_multi_proto_data.mpd_hash = tc_hash_create(
						TC_CREATE_PROTO_HASH_NUM, 
						tc_multi_proto_hash, 
						tc_multi_proto_hash_get,
						tc_multi_proto_hash_destroy);
	if (global_multi_proto_data.mpd_hash == TC_HASH_ERR)
		return TC_ERR;

	return TC_OK;
}

TC_MOD_INIT(tc_multi_proto_init);
