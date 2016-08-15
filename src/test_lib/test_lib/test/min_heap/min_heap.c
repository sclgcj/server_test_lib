#include "tc_comm.h"
#include "tc_heap.h"

struct tc_test {
	int val;
	unsigned long heap_data;
};

static int
tc_test_cmp(
	unsigned long new,
	unsigned long old
)
{
	struct tc_test *new_val = NULL, *old_val = NULL;

	//printf("address = %p, %p--00\n", (char*)new, (char*)old);
	new_val = ((struct tc_test*)new);
	old_val = ((struct tc_test*)old);
//	printf("val = %d, %d\n", new_val->val, old_val->val);

	if (new_val->val < old_val->val)
		return 0;

	return -1;
}

static void
tc_test_print(
	unsigned long user_data
)
{
	printf("%d ", *((int*)user_data));
}

static void
tc_test_destroy(
	unsigned long user_data
)
{
	struct tc_test *tt = (struct tc_test *)user_data;

	return;
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
	struct tc_test *array = NULL;
	unsigned long user_data = 0;
	tc_heap_handle_t handle;
	struct timespec t1, t2;

	srand(time(NULL));

	num = atoi(argv[1]);

	limit_num = num * 10;
	printf("num = %d\n", num);
	array = (struct tc_test*)calloc(num, sizeof(struct tc_test));

	handle = tc_heap_create(tc_test_cmp);


	clock_gettime(CLOCK_MONOTONIC, &t1);
	for (; i < num; i++) {
		array[i].val = random() % limit_num + 60;
		printf("%d ", array[i].val);
	}
	printf("\n");
	clock_gettime(CLOCK_MONOTONIC, &t2);
	fprintf(stderr, "t2 - t1 = %ld.%ld\n", t2.tv_sec - t1.tv_sec, t2.tv_nsec, t1.tv_nsec);

	clock_gettime(CLOCK_MONOTONIC, &t1);
	for (i = 0; i < num; i++) {
		array[i].heap_data = tc_heap_node_add(handle, (unsigned long)&array[i]);
	}
	clock_gettime(CLOCK_MONOTONIC, &t2);
	fprintf(stderr, "t2 - t1 = %ld.%ld\n", t2.tv_sec - t1.tv_sec, t2.tv_nsec, t1.tv_nsec);

	tc_heap_traversal(handle, tc_test_print);
	printf("\n");

	//for (i = 0; i < num; i++) {
		clock_gettime(CLOCK_MONOTONIC, &t1);
		tc_heap_root_data_get(handle, &user_data);
		clock_gettime(CLOCK_MONOTONIC, &t2);
		fprintf(stderr, "t2 - t1 = %ld.%ld\n", t2.tv_sec - t1.tv_sec, t2.tv_nsec, t1.tv_nsec);
		
		tc_heap_traversal(handle, tc_test_print);
		((struct tc_test*)user_data)->val -= 60;
		//printf("value = %d\n", ((struct tc_test*)user_data)->val);
		clock_gettime(CLOCK_MONOTONIC, &t1);
		tc_heap_node_add(handle, user_data);
		clock_gettime(CLOCK_MONOTONIC, &t2);
		fprintf(stderr, "t2 - t1 = %ld.%ld\n", t2.tv_sec - t1.tv_sec, t2.tv_nsec, t1.tv_nsec);
		//tc_heap_node_adjust(handle, ((struct tc_test*)user_data)->heap_data);
		printf("====================start=========================\n");
		tc_heap_traversal(handle, tc_test_print);
		printf("\n");
		printf("==================== end =========================\n");
//	}

	fprintf(stderr, "hhhhhhhhhhhhhhhhh\n");
	sleep(10);
	tc_heap_destroy(handle, NULL);//tc_test_destroy);
	fprintf(stderr, "see result\n");
	sleep(20);
	return 0;
}

