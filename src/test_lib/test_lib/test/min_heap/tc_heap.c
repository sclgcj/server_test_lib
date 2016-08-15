#include "tc_comm.h"
#include "tc_err.h"
#include "tc_heap.h"
#include "tc_print.h"

#include "list.h"

/*
 * 该模块类似于一个优先级队列，采用的是最小堆的实现方法，目的在于
 * 帮助简化定时器的检测。当前的定时器实现，是检测所有的节点，这样
 * 子虽然目前满足需求，但是总感觉不是很好，因此参考了libevent的介
 * 绍，思考之后决定尝试使用最小堆。经过测试，2016-8-15 的版本，在
 * 对100万的数据进行处理的时候，插入删除的效率感觉还行，基本上都在
 * 1s之内完成了操作，但是感觉还是不是很满意。 在性能比较好的测试机
 * 上进行测试，1000W的处理效率阔以保持在2s以内，这还是让人感觉非常
 * 兴奋，看来是开始的处理时间是因为虚拟机的本身的性能比较低造成的。
 */
struct tc_heap_node {
	unsigned long user_data;
	struct tc_heap_node *parent, *right, *left;
	struct list_head node, traversal_node;
};

struct tc_heap_head {
	struct tc_heap_node *last;
	struct tc_heap_node *root;
};

struct tc_heap_data {
	int (*user_cmp_func)(unsigned long new, unsigned long old);
	struct list_head head;
	pthread_mutex_t heap_mutex;
	struct tc_heap_head heap_head;
};


static int
tc_heap_default_cmp(
	unsigned long new,
	unsigned long old
)
{
	//默认采用最小堆
	if (new > old)
		return TC_OK;
	return TC_ERR;
}

tc_heap_handle_t
tc_heap_create(
	int (*user_cmp_func)(unsigned long new, unsigned long old)
)
{
	struct tc_heap_data *heap_data = NULL;	

	heap_data = (struct tc_heap_data *)calloc(1, sizeof(*heap_data));
	if (!heap_data) {
		//TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	INIT_LIST_HEAD(&heap_data->head);
	pthread_mutex_init(&heap_data->heap_mutex, NULL);
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
		//TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
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

	while(tmp != heap_data->heap_head.root) {
		if (tmp->parent) {
			ret = heap_data->user_cmp_func(tmp->user_data, tmp->parent->user_data);
			if (ret != TC_OK) {
				tc_heap_swap_val(tmp, tmp->parent);
				tmp = tmp->parent;
				continue;
			}
		}
		break;
	}
}

static void
tc_heap_del_adjust(
	struct tc_heap_data *heap_data,
	struct tc_heap_node *root
)
{
	int ret = 0;
	struct tc_heap_node *tmp = NULL, *child = NULL;

	tmp = root;
	
	while (tmp->right || tmp->left) {
		if (tmp->right && tmp->left && 
				tmp->right->user_data != (unsigned long)-1) {	
			ret = heap_data->user_cmp_func(
					tmp->right->user_data, 
					tmp->left->user_data);
			if (ret == TC_OK)
				child = tmp->left;
			else 
				child = tmp->right;
		} else {
			if (tmp->right && 
					(tmp->right->user_data != (unsigned long)-1))
				child = tmp->right;
			else
				child = tmp->left;
		}
		if (child->user_data == -1)
			break;
		ret = heap_data->user_cmp_func(tmp->user_data, child->user_data);
		if (ret == TC_OK) {
			tc_heap_swap_val(tmp, child);
			tmp = child;
			continue;
		}
		break;
	}
}

unsigned long
tc_heap_node_add(
	tc_heap_handle_t handle,
	unsigned long    user_data
)
{
	unsigned long ret = 0;
	struct tc_heap_data *heap_data = NULL;
	struct tc_heap_node *right_node = NULL, *left_node = NULL, *heap_node = NULL;

	if (!handle) {
		//TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_OK;
	}

	heap_data = (struct tc_heap_data*)handle;

	pthread_mutex_lock(&heap_data->heap_mutex);
	heap_node = list_entry(heap_data->head.next, struct tc_heap_node, node);
	list_del_init(&heap_node->node);	
	if (heap_node->user_data == (unsigned long)-1) {
		heap_node->user_data = user_data;
		heap_data->heap_head.last = heap_node;
		list_add_tail(&heap_node->node, &heap_data->head);
		tc_heap_add_adjust(heap_data, heap_node);
		goto out;
	} 

	left_node = (struct tc_heap_node *)calloc(1, sizeof(*left_node));
	if (!left_node) {
		//TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		goto err;
	}
	left_node->user_data = user_data;
	left_node->parent = heap_node;
	heap_node->left = left_node;
	heap_data->heap_head.last = left_node;
	tc_heap_add_adjust(heap_data, left_node);
	right_node = (struct tc_heap_node *)calloc(1, sizeof(*right_node));
	if (!right_node) {
		//TC_ERRNO_SET(TC_NOT_ENOUGH_MEMORY);
		goto err;
	}
	right_node->parent = heap_node;
	right_node->user_data = (unsigned long)-1;
	heap_node->right = right_node;
	list_add_tail(&left_node->node, &heap_data->head);
	list_add(&right_node->node, &heap_data->head);
	ret = TC_OK;
	goto out;
err:
	ret = TC_ERR;
out:
	pthread_mutex_unlock(&heap_data->heap_mutex);
	return TC_OK;
}

static void
tc_heap_node_del( struct tc_heap_data *heap_data
)
{
	struct tc_heap_node *heap_node = NULL, *last = NULL;

	last = heap_data->heap_head.last;
	heap_node = heap_data->heap_head.root;
	if (!heap_data->heap_head.last) 
		return;
	//交换根结点和最后一个结点的值
	tc_heap_swap_val(last, heap_node);	
	heap_data->heap_head.last->user_data = (unsigned long)-1;

	// 把最后一个结点从添加队列中删除，此时它应该是最末尾的一个，
	// 然后再添加到添加队列的头，这样它就会被第一个处理，处理
	// 完之后会被再次添加进队列尾部
	list_del_init(&last->node);
	list_add(&last->node, &heap_data->head);

	//将last结点指向最后结点的上一个结点。
	if (last == last->parent->right)
		heap_data->heap_head.last = last->parent->left;
	else
		heap_data->heap_head.last = last->parent;

	tc_heap_del_adjust(heap_data, heap_node);
}

int
tc_heap_root_data_get(
	tc_heap_handle_t handle,
	unsigned long    *user_data
)
{
	struct tc_heap_data *heap_data = NULL;

	if (!handle || !user_data) {
		//TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	heap_data = (struct tc_heap_data *)handle;
	if (!heap_data->heap_head.root) {
		//TC_ERRNO_SET(TC_NO_HEAP_DATA);
		return TC_ERR;
	}

	pthread_mutex_lock(&heap_data->heap_mutex);
	(*user_data) = heap_data->heap_head.root->user_data;
	tc_heap_node_del(handle);
	pthread_mutex_unlock(&heap_data->heap_mutex);

	return TC_OK;
}

static void
tc_heap_default_traversal(
	unsigned long user_data
)
{
	PRINT("user_data = %ld\n", user_data);
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

	if (!handle) {
		//TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	if (heap_traversal)
		tmp_traversal = heap_traversal;
	else
		tmp_traversal = tc_heap_default_traversal;
	
	heap_data = (struct tc_heap_data *)handle;
	tmp = heap_data->heap_head.root;
	if (!tmp)
		return TC_OK;

	INIT_LIST_HEAD(&head);
	list_add_tail(&tmp->traversal_node, &head);

	while (!list_empty(&head)) {
//		PRINT("\n");
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

int
tc_heap_destroy(
	tc_heap_handle_t handle,
	void (*user_data_destroy)(unsigned long user_data)
)
{
	int count = 0;
	void (*tmp_traversal)(unsigned long user_data);
	struct list_head head;
	struct tc_heap_data *heap_data = NULL;
	struct tc_heap_node *tmp_left = NULL, *tmp_right = NULL, *tmp = NULL;

	if (!handle) {
		//TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}

	heap_data = (struct tc_heap_data *)handle;
	tmp = heap_data->heap_head.root;
	if (!tmp)
		return TC_OK;

	INIT_LIST_HEAD(&head);
	list_add_tail(&tmp->traversal_node, &head);

	while (!list_empty(&head)) {
//		PRINT("\n");
		tmp = list_entry(head.next, struct tc_heap_node, traversal_node);
		list_del_init(head.next);
		if (tmp->user_data == (unsigned long)-1) {
			TC_FREE(tmp);
			break;
		}
		if (user_data_destroy)
			user_data_destroy(tmp->user_data);
		if (tmp->left) 
			list_add_tail(&tmp->left->traversal_node, &head);
		if (tmp->right)
			list_add_tail(&tmp->right->traversal_node, &head);
		tmp->right = NULL;
		tmp->left = NULL;
		TC_FREE(tmp);
	}

	return TC_OK;
}
