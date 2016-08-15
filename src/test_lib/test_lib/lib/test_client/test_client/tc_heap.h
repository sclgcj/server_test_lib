#ifndef TC_HEAP_H
#define TC_HEAP_H 1

typedef void* tc_heap_handle_t;

enum {
	TC_HEAP_LARGER,
	TC_HEAP_LESS,
	TC_HEAP_SAME,
	TC_HEAP_MAX
};

/*
 * tc_heap_create() - create the heap handle 
 * @user_cmp_func:  the function to decide if the new is larger/less than old, 
 *		    if it is, user_cmp_func return 0, otherwise return -1
 *	@first: the first parameter pointed to a user defined data
 *	@second: the second parameter pointed to a user defined data
 *	@Return: in min heap, 0 if first is larger than second, otherwise return -1;
 *		 in max heap, 0 if first is less than second, otherwise return -1;
 * @user_data_destroy: destroy the user data, this function will be called when delete
 *		       the root node or destroy the heap
 *
 * Return: a heap handle will be returned if successful, NULL if not and errno will be set
 */
tc_heap_handle_t
tc_heap_create(
	int (*user_cmp_func)(unsigned long first, unsigned long second)
);

/*
 * tc_heap_node_add() - add a heap node
 * @handle:	the heap handle returned by tc_heap_create() function
 * @use_data:	user data
 *
 * Return:	0 if successful, -1 if not and errno will be set
 */
unsigned long
tc_heap_node_add(
	tc_heap_handle_t handle,
	unsigned long    user_data
);

/*
 * tc_heap_root_data_get() - get the root node's  data
 * @handle:	the heap handle returned by tc_heap_create()
 * @user_data:  user to store the address of data in the root node
 *
 * Return:	0 if successful, -1 if not and errno will be set
 */
int
tc_heap_root_data_get(
	tc_heap_handle_t handle,
	unsigned long    *user_data
);

/*
 * tc_heap_traversal()  - traversal the heap
 * @handle:	the heap handle returned by tc_heap_create()
 *
 * Be careful, This function is not thread-safa
 *	
 * Return:	0 if successful, -1 if not and errno will be set
 */
int
tc_heap_traversal(
	tc_heap_handle_t handle,
	void (*heap_traversal)(unsigned long user_data)
);

int
tc_heap_destroy(
	tc_heap_handle_t handle,
	void (*user_data_destroy)(unsigned long user_data)
);

#endif
