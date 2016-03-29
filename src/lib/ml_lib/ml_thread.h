#ifndef ML_THREAD_POOL_H
#define ML_THREAD_POOL_H 1

#include "list.h"
#include "ml_exit.h"
#include "ml_comm.h"

#define ML_THREAD_GROUP 10
#define ML_THREAD_MEMBER 1
#define ML_THREAD_LIML_LEN  50  //本来使用限制每个线程可处理的任务书的,后面发现这完全没用,所以目前是丢弃该配置的;

typedef void * MLThreadHandle;

int
ml_create_thread_table(
	int iThreadGroupNum, 
	MLExitHandle   struExitHandle,
	MLThreadHandle *pStruHandle
);

int
ml_destroy_thread_table(
	MLThreadHandle struHandle
);

int
ml_create_thread_table_pool(
	int iThreadNum,
	int iStackSize,
	void (*group_free)(struct list_head *),
	int  (*group_func)(struct list_head *),
	int  (*execute_func)(struct list_head *),	
	MLThreadHandle struHandle,
	int  *piThreadID
);

int
ml_add_table_thread_pool_node(
	int							 iThreadID,
	struct list_head *pStruNode,
	MLThreadHandle	 struHandle
);

#endif
