#include "tc_comm.h"
#include "tc_err.h"
#include "tc_heap.h"
#include "tc_print.h"

#include "list.h"

/*
 * 该模块类似于一个优先级队列，采用的是最小堆的实现方法，目的在于
 * 帮助简化定时器的检测。当前的定时器实现，是检测所有的节点，这样
 * 子虽然目前满足需求，但是总感觉不是很好，因此参考了libevent的介
 * 绍，思考之后决定尝试使用最小堆。
 */
struct tc_heap_node {
	unsigned long user_data;
	struct tc_heap_node *parent, *right, *left;
	struct list_head node, traversal_node;
};

struct tc_heap_head {
	struct tc_heap_node *last_leaf;
	struct tc_heap_node *last;
	struct tc_heap_node *root;
};

struct tc_heap_data {
	int (*user_cmp_func)(unsigned long new, unsigned long old);
	struct list_head head;
	pthread_mutex_t heap_mutex;
	pthread_mutex_t heap_list_mutex;
	struct tc_heap_head heap_head;
};


static int
tc_heap_default_cmp(
	unsigned long new,
	unsigned long old
)
{
	if (new > old)
		return TC_HEAP_LARGER;
	if (new == old)
		return TC_HEAP_SAME;
	return TC_HEAP_LESS;
}

tc_heap_handle_t
tc_heap_create(
	int (*user_cmp_func)(unsigned long new, unsigned long old)
)
{
	struct tc_heap_data *heap_data = NULL;	

	heap_data = (struct tc_heap_data *)calloc(1, sizeof(heap_data));
	if (!heap_data) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	INIT_LIST_HEAD(&heap_data->head);
	pthread_mutex_init(&heap_data->heap_mutex, NULL);
	pthread_mutex_init(&heap_data->heap_list_mutex, NULL);
	if (user_cmp_func)
		heap_data->user_cmp_func = user_cmp_func;
	else
		heap_data->user_cmp_func = tc_heap_default_cmp;
	heap_data->heap_head.root = (struct tc_heap_node*)calloc(
						1, sizeof(*heap_data->heap_head.root));
	heap_data->heap_head.root->user_data = (unsigned long)-1;
	if (!heap_data->heap_head.root) {
		pthread_mutex_destroy(&heap_data->heap_mutex);
		TC_FREE(heap_data);
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	list_add_tail(&heap_data->heap_head.root->node, &heap_data->head);

	return (tc_heap_handle_t)heap_data;
}

static void
tc_heap_swap_val(
	struct tc_heap_node *heap1,
	struct tc_heap_node *heap2
)
{
	unsigned long tmp = 0;

	tmp = heap1->user_data;
	heap1->user_data = heap2->user_data;
	heap2->user_data = tmp;
}

static void
tc_heap_add_adjust(
	struct tc_heap_data *heap_data,
	struct tc_heap_node *heap_node
)
{
	int ret = 0;
	struct tc_heap_node *tmp = heap_node;

	if (!heap_node->parent)
		return;

	pthread_mutex_lock(&heap_data->heap_mutex);
	while(tmp != heap_data->heap_head.root) {
		if (tmp->parent) {
			ret = heap_data->user_cmp_func(tmp->user_data, tmp->parent->user_data);
			if (ret != TC_HEAP_LARGER) {
				tc_heap_swap_val(tmp, tmp->parent);
				tmp = tmp->parent;
				continue;
			}
		}
		break;
	}
	pthread_mutex_unlock(&heap_data->heap_mutex);

	//printf("--------------------------------------------\n");
	//tc_heap_traversal((tc_heap_handle_t)heap_data, NULL);
	//printf("--------------------------------------------\n");
}

static void
tc_heap_del_adjust(
	struct tc_heap_data *heap_data
)
{
	int ret = 0;
	struct tc_heap_node *tmp = NULL, *child = NULL;

	tmp = heap_data->heap_head.root;
	
	while (tmp->right || tmp->left) {
		if (tmp->right || tmp->left) {
			ret = heap_data->user_cmp_func(
					tmp->right->user_data, 
					tmp->left->user_data);
			if (ret == TC_HEAP_LARGER)
				child = tmp->left;
			else 
				child = tmp->right;
		} else {
			if (tmp->right)
				child = tmp->right;
			else
				child = tmp->left;
		}
		ret = heap_data->user_cmp_func(tmp->user_data, child->user_data);
		if (ret != TC_HEAP_LARGER)
			return;
		PRINT("\n");
		tc_heap_swap_val(tmp, child);
		tmp = child;
	}
}

int
tc_heap_node_add(
	tc_heap_handle_t handle,
	unsigned long    user_data
)
{
	struct tc_heap_data *heap_data = NULL;
	struct tc_heap_node *right_node = NULL, *left_node = NULL, *heap_node = NULL;

	if (!handle) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_OK;
	}

	heap_data = (struct tc_heap_data*)handle;

	pthread_mutex_lock(&heap_data->heap_list_mutex);
	heap_node = list_entry(heap_data->head.next, struct tc_heap_node, node);
	list_del_init(&heap_node->node);	
	PRINT("\n");
	if (heap_node->user_data == (unsigned long)-1) {
		heap_node->user_data = user_data;
		list_add_tail(&heap_node->node, &heap_data->head);
		pthread_mutex_unlock(&heap_data->heap_list_mutex);
		tc_heap_add_adjust(heap_data, heap_node);
		return TC_OK;
	}
	pthread_mutex_unlock(&heap_data->heap_list_mutex);

	PRINT("\n");
	left_node = (struct tc_heap_node *)calloc(1, sizeof(*left_node));
	if (!left_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	left_node->user_data = user_data;
	left_node->parent = heap_node;
	heap_node->left = left_node;
	tc_heap_add_adjust(heap_data, left_node);
	right_node = (struct tc_heap_node *)calloc(1, sizeof(*right_node));
	if (!right_node) {
		TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return TC_ERR;
	}
	right_node->parent = heap_node;
	right_node->user_data = (unsigned long)-1;
	heap_node->right = right_node;
	PRINT("\n");
	pthread_mutex_lock(&heap_data->heap_list_mutex);
	list_add_tail(&left_node->node, &heap_data->head);
	list_add(&right_node->node, &heap_data->head);
	pthread_mutex_unlock(&heap_data->heap_list_mutex);
	/*pthread_mutex_lock(&heap_data->heap_mutex);
	if (heap_data->heap_head.last == NULL) {
		heap_data->heap_head.last = heap_node;
		heap_data->heap_head.root = heap_node;
	} else {
		heap_node->parent = heap_data->heap_head.last;
		if (!heap_data->heap_head.last->left) 
			heap_data->heap_head.last->left = heap_node;
		else {
			heap_data->heap_head.last->right = heap_node;
			if (!heap_data->heap_head.last->parent) 
				heap_data->heap_head.last = 
					heap_data->heap_head.last->left;
			else if (heap_data->heap_head.last->parent->right == 
					heap_data->heap_head.last)
				heap_data->heap_head.last = 
					heap_data->heap_head.last->parent->left->left;
			else 
				heap_data->heap_head.last = 
					heap_data->heap_head.last->parent->right;
		}
		tc_heap_add_adjust(heap_data, heap_node);
	}
	pthread_mutex_unlock(&heap_data->heap_mutex);*/

	return TC_OK;
}

int
tc_heap_head_node_del(
	tc_heap_handle_t handle
)
{
	struct tc_heap_data *heap_data = NULL;

	if (!handle) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	heap_data = (struct tc_heap_data *)handle;

	pthread_mutex_lock(&heap_data->heap_mutex);
	tc_heap_del_adjust(heap_data);
	pthread_mutex_unlock(&heap_data->heap_mutex);

	return TC_OK;
}

int
tc_heap_root_data_get(
	tc_heap_handle_t handle,
	unsigned long    *user_data
)
{
	struct tc_heap_data *heap_data = NULL;

	if (!handle || !user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	heap_data = (struct tc_heap_data *)handle;
	if (!heap_data->heap_head.root) {
		TC_ERRNO_SET(TC_NO_HEAP_DATA);
		return TC_ERR;
	}

	pthread_mutex_lock(&heap_data->heap_mutex);
	(*user_data) = heap_data->heap_head.root->user_data;
	pthread_mutex_unlock(&heap_data->heap_mutex);

	return TC_OK;
}

static void
tc_heap_default_traversal(
	unsigned long user_data
)
{
	PRINT("user_data = %d\n", user_data);
}

int
tc_heap_traversal(
	tc_heap_handle_t handle,
	void (*heap_traversal)(unsigned long user_data)
)
{
	void (*tmp_traversal)(unsigned long user_data);
	struct list_head head;
	struct tc_heap_data *heap_data = NULL;
	struct tc_heap_node *tmp_left = NULL, *tmp_right = NULL, *tmp = NULL;

	PRINT("\n");
	if (!handle) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	if (heap_traversal)
		tmp_traversal = heap_traversal;
	else
		tmp_traversal = tc_heap_default_traversal;
	
	heap_data = (struct tc_heap_data *)handle;
	PRINT("\n");
	tmp = heap_data->heap_head.root;
	if (!tmp)
		return TC_OK;

	PRINT("\n");
	INIT_LIST_HEAD(&head);
	list_add_tail(&tmp->traversal_node, &head);

	while (!list_empty(&head)) {
		PRINT("\n");
		tmp = list_entry(head.next, struct tc_heap_node, traversal_node);
		list_del_init(head.next);
		if (tmp->user_data == (unsigned long)-1)
			break;
		tmp_traversal(tmp->user_data);
		if (tmp->left) 
			list_add_tail(&tmp->left->traversal_node, &head);
		if (tmp->right)
			list_add_tail(&tmp->right->traversal_node, &head);
	}

	return TC_OK;
}
