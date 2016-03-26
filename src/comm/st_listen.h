#ifndef ST_LISTEN_H
#define ST_LISTEN_H 1

#include "st_recv.h"
#include "st_send.h"

typedef void * STListenHandle;
typedef void (*STEpollinFunc)(void *pEpollData);
typedef void (*STEpollOutFunc)(void *pEpollData);
typedef void (*STEpollErrFunc)(void *pEpollData);
typedef void (*STEpollHupFunc)(void *pEpollData);
typedef void (*STEpollPriFunc)(void *pEpollData);
typedef void (*STEpollRDHupFunc)(void *pEpollData);

typedef struct _STListenerOp
{
	STEpollErrFunc   pEpollErrFunc;				//错误操作
	STEpollHupFunc   pEpollHupFunc;				//挂起操作
	STEpollPriFunc   pEpollPriFunc;				//外带数据操作
	STEpollRDHupFunc pEpollRDHupFunc;			//对端关闭套接字操作
}STListenOp, *PSTListenOp;

// 监听事件个数
#define ST_MAX_EVENT 8192

int
st_create_listener(
	int iWaitTime,
	int iListenNum,
	STListenOp *pStruListenerOper,
	STRecvHandle struRecvHandle,
	STSendHandle struSendHandle,
	STListenHandle *pStruHandle
);

int 
st_destroy_listener(
	STListenHandle struHandle
);

/*
 *	启动监听器
 */
int
st_start_listener( STListenHandle struHandle );

/*
 * 添加监听套接字
 */
int
st_add_listen_sockfd( int iEvent, int iSockfd, void *pData, STListenHandle struHandle );

/*
 *	删除监听套接字
 */
int
st_del_listen_sockfd( int iSockfd, STListenHandle struHandle );

int
st_mod_listen_sockfd(
	int iEvent,
	int iSockfd,
	void *pData,
	STListenHandle struHandle
);

#endif
