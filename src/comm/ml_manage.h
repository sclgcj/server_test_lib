#ifndef ML_MANAGE_H
#define ML_MANAGE_H  1

#include "ml_comm.h"
#include "ml_hub.h"
#include "ml_recv.h"
#include "ml_send.h"
#include "ml_timer.h"
#include "ml_listen.h"
#include "ml_thread.h"
#include "ml_create.h"
#include "ml_result.h"
#include "ml_dispose.h"
#include "ml_handle_opt.h"
#include "ml_recv_check.h"
#include "ml_read_config.h"

typedef void * MLHandle;

#define SERVER_THREAD_TABLE_SIZE 10

typedef struct _ServerTest{ 
	unsigned int   uiDurationTime;			//测试持续时间

	MLRCHandle		 struRCHandle;				//config配置
	MLCLHandle     struCLHandle;				//create link 
	MLHubHandle    struHubHandle;				//hub config
	MLOptHandle    struOptHandle;       //选项配置
	MLRecvHandle   struRecvHandle;			//recv
	MLSendHandle   struSendHandle;
	MLTimerHandle  struTimerHandle;			//定时器
	MLListenHandle struListenHandle;		//套接字监e听
	MLThreadHandle struThreadHandle;		//线程组表
	MLResultHandle struResultHandle;
	MLDisposeHandle struDisposeHandle;
	MLRecvCheckHandle struRecvCheckHandle; //接收超时检测
}ServerTest, *PServerTest;

//创建测试服务器管理句柄
//首先调用的函数
void
ml_create_manager(
	unsigned int uiDurationTime,
	MLHandle *pStruHandle
);

int
ml_manager_create_listener(
	int iWaitTime,
	int iListenNum,
	MLListenOp *pStruListenOp,
	MLHandle	 struHandle
);

int 
ml_manager_create_thread(
	int iThreadGroupNum,
	MLHandle struHandle
);

int
ml_manager_create_timer(
	int			 iTimerNum,
	int 		 iThreadNum,
	MLHandle struHandle
);

int
ml_manager_create_hub(
	int             iHubNum,
	int             iThreadNum,
	int             iStackSize,
	MLHubFunc       pHubFunc,
	MLHubHandleFunc pHubHandleFunc,
	MLHandle        struHandle
);

int
ml_manager_create_opt_config(
	int  iArgc,
	char **ssArgv,
	char *sParseFmt,
	MLHandle struHandle
);

int
ml_manager_create_read_config(
	char *sFile,
	MLHandle struHandle
);

int
ml_manage_create_link_handle(
	void             *pUserData,	
	MLCLParam        *pStruCLParam,
	MLCreateLinkFunc pCLFunc,
	MLHandle				 struHandle
);

int
ml_manager_create_recv_check(
	int  iTotalLink,
	int  iRecvTimerout,
	int  iCheckListNum,
	MLRecvCheckFailFunc pFunc,
	MLHandle struHandle
);

int
ml_manager_create_recv(
	int iThreadNum,
	int iStackSize,
	MLRecvFunc pFunc,	
	MLHandle   struHandle
);

int
ml_manager_create_dispose(
	int iThreadNum,
	int iStackSize,
	MLDisposeFunc pFunc,
	MLHandle       struHandle
);

int
ml_manager_create_send(
	int iThreadNum,
	int iStackSize,
	MLSendFunc pFunc,
	MLHandle struHandle
);

int
ml_manager_create_result(
	int iTotalCnt,
	void *pUserData,
	char *sResultName,
	MLResultFunc pFunc,
	MLHandle struHandle
);

int
ml_manager_create_all(
	int      iTotalCnt,
	MLCLParam *pStruParam,
	MLHandle *pStruHandle
);

//摧毁服务器管理句柄
int
ml_destroy_manager(
	MLHandle struHandle
);

#endif
