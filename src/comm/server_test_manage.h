#ifndef SERVER_TEST_MANAGE_H
#define SERVER_TEST_MANAGE_H  1

#include "server_test_comm.h"
#include "server_test_timer.h"
#include "server_test_listen.h"
#include "server_test_thread.h"

typedef void * STHandle;

#define SERVER_THREAD_TABLE_SIZE 10

typedef struct _ServerTest{ 
	unsigned int   uiDurationTime;			//测试持续时间

	STListenHandle struListenHandle;		//套接字监听句柄
	STTimerHandle  struTimerHandle;			//定时器
	STThreadHandle struThreadHandle;		//线程组表
}ServerTest, *PServerTest;

//创建测试服务器管理句柄
//首先调用的函数
void
server_test_create_manager(
	unsigned int uiDurationTime,
	STHandle *pStruHandle
);

int
server_test_create_manager_listener(
	int iWaitTime,
	int iListenNum,
	STListenOp *pStruListenOp,
	STHandle	 struHandle
);

int 
server_test_create_manager_thread(
	int iThreadGroupNum,
	STHandle struHandle
);

int
server_test_create_manager_timer(
	int iTimerNum,
	int iThreadNum,
	STHandle struHandle
);

int
server_test_create_manager_all(
	int iWaitTime,
	int iListenNum,			
	unsigned int uiDurationTime,
	int iThreadNum,
	int	iThreadGroupNum, 
	int iTimerNum,
	STListenOp *pStruListenOp,
	STHandle *pStruHandle
);

//摧毁服务器管理句柄
int
server_test_destroy_manager(
	STHandle struHandle
);

//添加需要管理套接字
int
server_test_add_manage_sockfd(
	int  iEvent,
	int  iSockfd,
	void *pData,
	STHandle struHandle
);

//删除需要管理的套接字
int
server_test_del_mamage_sockfd(
	int iSockfd,
	STHandle struHandle
);

//改变需管理的套接字状态
int
server_test_mod_manage_sockfd(
	int iEvent,
	int iSockfd, 
	void *pData,
	STHandle struHandle
);

//启动套接字监听
int
server_test_start_manage_listener(
	STHandle	struHandle
);

#endif
