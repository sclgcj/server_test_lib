#ifndef TC_HEAP_H
#define TC_HEAP_H 1

typedef void* tc_heap_handle_t;

enum {
	TC_HEAP_LARGER,
	TC_HEAP_LESS,
	TC_HEAP_SAME,
	TC_HEAP_MAX
};

tc_heap_handle_t
tc_heap_create(
	int (*user_cmp_func)(unsigned long new, unsigned long old)
);

int
tc_heap_node_add(
	tc_heap_handle_t handle,
	unsigned long    user_data
);

int
tc_heap_head_node_del(
	tc_heap_handle_t handle
);

int
tc_heap_root_data_get(
	tc_heap_handle_t handle,
	unsigned long    *user_data
);

int
tc_heap_traversal(
	tc_heap_handle_t handle,
	void (*heap_traversal)(unsigned long user_data)
);

#endif
