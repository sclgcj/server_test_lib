#include "tc_comm.h"
#include "tc_heap.h"

static int
tc_test_cmp(
	unsigned long new,
	unsigned long old
)
{
	int new_val = 0, old_val = 0;

	printf("address = %p, %p--00\n", (char*)new, (char*)old);
	new_val = *((int*)new);
	old_val = *((int*)old);

	if (new_val > old_val)
		return TC_HEAP_LARGER;
	else if (new_val == old_val)
		return TC_HEAP_LESS;
	return TC_HEAP_SAME;
}

static void
tc_test_print(
	unsigned long user_data
)
{
	printf("%d ", *((int*)user_data));
}


#define ARRAY_SIZE 100
int 
main(
	int argc,
	char **argv
)
{
	int i = 0;
	int num = 0;
	int limit_num = 0;
	int *array = NULL;
	unsigned long user_data = 0;
	tc_heap_handle_t handle;

	srand(time(NULL));

	num = atoi(argv[1]);

	limit_num = num * 10;
	printf("num = %d\n", num);
	array = (int*)calloc(num, sizeof(int));

	handle = tc_heap_create(tc_test_cmp);

	for (; i < num; i++) {
		array[i] = random() % num * limit_num;
		printf("%d ", array[i]);
	}
	printf("\n");

	for (i = 0; i < num; i++) 
		tc_heap_node_add(handle, (unsigned long)&array[i]);

	tc_heap_traversal(handle, tc_test_print);

	/*for (i = 0; i < 10; i++) {
		tc_heap_root_data_get(handle, &user_data);
		*((int*)user_data) += 60;
		tc_heap_head_node_del(handle);
		printf("====================start=========================\n");
		tc_heap_traversal(handle, tc_test_print);
		printf("\n");
		printf("==================== end =========================\n");
	}*/
}

