#ifndef ML_LIMLEN_H
#define ML_LIMLEN_H 1

#include "ml_recv.h"
#include "ml_send.h"

typedef void * MLListenHandle;
typedef void (*MLEpollinFunc)(void *pEpollData);
typedef void (*MLEpollOutFunc)(void *pEpollData);
typedef void (*MLEpollErrFunc)(void *pEpollData);
typedef void (*MLEpollHupFunc)(void *pEpollData);
typedef void (*MLEpollPriFunc)(void *pEpollData);
typedef void (*MLEpollRDHupFunc)(void *pEpollData);

typedef struct _MLListenerOp
{
	MLEpollErrFunc   pEpollErrFunc;				//错误操作
	MLEpollHupFunc   pEpollHupFunc;				//挂起操作
	MLEpollPriFunc   pEpollPriFunc;				//外带数据操作
	MLEpollRDHupFunc pEpollRDHupFunc;			//对端关闭套接字操作
}MLListenOp, *PMLListenOp;

// 监听事件个数
#define ML_MAX_EVENT 8192

int
ml_create_listener(
	int iWaitTime,
	int iListenNum,
	MLListenOp *pStruListenerOper,
	MLRecvHandle struRecvHandle,
	MLSendHandle struSendHandle,
	MLListenHandle *pStruHandle
);

int 
ml_destroy_listener(
	MLListenHandle struHandle
);

/*
 *	启动监听器
 */
int
ml_start_listener( MLListenHandle struHandle );

/*
 * 添加监听套接字
 */
int
ml_add_listen_sockfd( int iEvent, int iSockfd, void *pData, MLListenHandle struHandle );

/*
 *	删除监听套接字
 */
int
ml_del_listen_sockfd( int iSockfd, MLListenHandle struHandle );

int
ml_mod_listen_sockfd(
	int iEvent,
	int iSockfd,
	void *pData,
	MLListenHandle struHandle
);

#endif
