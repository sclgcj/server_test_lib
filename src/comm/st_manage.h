#ifndef ST_MANAGE_H
#define ST_MANAGE_H  1

#include "st_comm.h"
#include "st_hub.h"
#include "st_timer.h"
#include "st_listen.h"
#include "st_thread.h"
#include "st_create.h"
#include "st_handle_opt.h"
#include "st_read_config.h"

typedef void * STHandle;

#define SERVER_THREAD_TABLE_SIZE 10

typedef struct _ServerTest{ 
	unsigned int   uiDurationTime;			//测试持续时间

	STRCHandle		 struRCHandle;				//config配置
	STCLHandle     struCLHandle;				//create link 
	STHubHandle    struHubHandle;				//hub config
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
st_manager_create_listener(
	int iWaitTime,
	int iListenNum,
	STListenOp *pStruListenOp,
	STHandle	 struHandle
);

int 
st_manager_create_thread(
	int iThreadGroupNum,
	STHandle struHandle
);

int
st_manager_create_timer(
	int			 iTimerNum,
	int 		 iThreadNum,
	STHandle struHandle
);

int
st_manager_create_hub(
	int             iHubNum,
	int             iThreadNum,
	int             iStackSize,
	STHubFunc       pHubFunc,
	STHubHandleFunc pHubHandleFunc,
	STHandle        struHandle
);

int
st_manager_create_opt_config(
	int  iArgc,
	char **ssArgv,
	char *sParseFmt,
	STHandle struHandle
);

int
st_manager_create_read_config(
	char *sFile,
	STHandle struHandle
);

int
st_manage_create_link_handle(
	void             *pUserData,	
	STCLParam        *pStruCLParam,
	STCreateLinkFunc pCLFunc,
	STHandle				 struHandle
);

int
st_manager_create_all(
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

#endif
