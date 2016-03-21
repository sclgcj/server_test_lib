#ifndef SERVER_TEST_THREAD_POOL_H
#define SERVER_TEST_THREAD_POOL_H 1

#include "list.h"
#include "server_test_comm.h"

#define SERVER_TEST_THREAD_GROUP 10
#define SERVER_TEST_THREAD_MEMBER 1
#define SERVER_TEST_THREAD_LIST_LEN  50  //本来使用限制每个线程可处理的任务书的,后面发现这完全没用,所以目前是丢弃该配置的;

typedef void * STThreadHandle;

int
server_test_create_thread_table(
	int iThreadGroupNum, 
	STThreadHandle *pStruHandle
);

int
server_test_destroy_thread_table(
	STThreadHandle struHandle
);

int
server_test_create_thread_table_pool(
	int iThreadNum,
	int iStackSize,
	void (*group_free)(struct list_head *),
	int  (*group_func)(struct list_head *),
	int  (*execute_func)(struct list_head *),	
	STThreadHandle struHandle,
	int  *piThreadID
);

int
server_test_add_table_thread_pool_node(
	int							 iThreadID,
	struct list_head *pStruNode,
	STThreadHandle	 struHandle
);

#endif
