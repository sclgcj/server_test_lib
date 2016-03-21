#ifndef ST_MANAGE_H
#define ST_MANAGE_H  1

#include "st_comm.h"
#include "st_timer.h"
#include "st_listen.h"
#include "st_thread.h"
#include "st_handle_opt.h"
#include "st_read_config.h"

typedef void * STHandle;

#define SERVER_THREAD_TABLE_SIZE 10

typedef struct _ServerTest{ 
	unsigned int   uiDurationTime;			//测试持续时间

	STRCHandle		 struRCHandle;				//config配置
	STOptHandle    struOptHandle;       //选项配置
	STListenHandle struListenHandle;		//套接字监听
	STTimerHandle  struTimerHandle;			//定时器
	STThreadHandle struThreadHandle;		//线程组表
}ServerTest, *PServerTest;

//创建测试服务器管理句柄
//首先调用的函数
void
st_create_manager(
	unsigned int uiDurationTime,
	STHandle *pStruHandle
);

int
st_create_manager_listener(
	int iWaitTime,
	int iListenNum,
	STListenOp *pStruListenOp,
	STHandle	 struHandle
);

int 
st_create_manager_thread(
	int iThreadGroupNum,
	STHandle struHandle
);

int
st_create_manager_timer(
	int iTimerNum,
	int iThreadNum,
	STHandle struHandle
);

int
st_create_manager_all(
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
st_destroy_manager(
	STHandle struHandle
);

//添加需要管理套接字
int
st_add_manage_sockfd(
	int  iEvent,
	int  iSockfd,
	void *pData,
	STHandle struHandle
);

//删除需要管理的套接字
int
st_del_mamage_sockfd(
	int iSockfd,
	STHandle struHandle
);

//改变需管理的套接字状态
int
st_mod_manage_sockfd(
	int iEvent,
	int iSockfd, 
	void *pData,
	STHandle struHandle
);

//启动套接字监听
int
st_start_manage_listener(
	STHandle	struHandle
);

#endif
