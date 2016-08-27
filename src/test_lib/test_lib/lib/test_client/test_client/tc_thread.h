#ifndef TC_THREAD_H
#define TC_THREAD_H 1

#include "list.h"

/*
 * This is thread module. We design our thread pool different from normal ones, because 
 * we want to manage our threads in a more simple and effective way. 
 */

/*
 * tc_thread_pool_create() - create a thread pool
 * @count: the number of the member threads
 * @stack_size: the stack size of every thread
 * @group_free: the free function in group thread
 *	@@node: user structure's list node pointer
 * @group_func: the execute_func in group thread
 *	@@node: user structure's list node pointer
 * @execute_func: the execute_func in member thread
 *	@@node: user structure's list node pointer
 * @group_id:   thread group id
 *
 * Use this fucntion to create a thread pool which consists of a master(group) thread and 
 * count member thread. Every member thread will execute execute_func. group_free and 
 * group_func executed in master thread.
 *
 * Return: 0 if successful, -1 if not and specified error will be set
 */
int
tc_thread_pool_create(
	int  count,
	int  stack_size,
	char *sThreadName,
	void (*group_free)(struct list_head *node),
	int  (*group_func)(struct list_head *node),
	int  (*execute_func)(struct list_head *node),
	int  *group_id
);


/*
 * tc_thread_pool_node_add() - add node to thread pool
 * @group_id:	the thread group id
 * @node:	the node pointer
 */
int 
tc_thread_pool_node_add(
	int group_id,
	struct list_head *node
);

/*
 * tc_thread_test_exit() - check wether to exit the thread
 *
 * Return: 0 if needing to exit, -1 if not
 */
int
tc_thread_test_exit();

/*
 * tc_thread_exit_wait() - set thread exit and wait all created
 *			   threads to exit
 *
 * The implemettion of this function is an unsafe at present, because
 * we wait just 5 seconds to exit. There may be some reason that
 * some threads don't exit in 5 seconds, and we don't do any effort
 * on this bug. We hope we can find a better way to dispose this.
 */
void
tc_thread_exit_wait();


#endif
