#ifndef ST_THREAD_POOL_H
#define ST_THREAD_POOL_H 1

#include "list.h"
#include "st_comm.h"

#define ST_THREAD_GROUP 10
#define ST_THREAD_MEMBER 1
#define ST_THREAD_LIST_LEN  50  //本来使用限制每个线程可处理的任务书的,后面发现这完全没用,所以目前是丢弃该配置的;

typedef void * STThreadHandle;

int
st_create_thread_table(
	int iThreadGroupNum, 
	STThreadHandle *pStruHandle
);

int
st_destroy_thread_table(
	STThreadHandle struHandle
);

int
st_create_thread_table_pool(
	int iThreadNum,
	int iStackSize,
	void (*group_free)(struct list_head *),
	int  (*group_func)(struct list_head *),
	int  (*execute_func)(struct list_head *),	
	STThreadHandle struHandle,
	int  *piThreadID
);

int
st_add_table_thread_pool_node(
	int							 iThreadID,
	struct list_head *pStruNode,
	STThreadHandle	 struHandle
);

#endif
