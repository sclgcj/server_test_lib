#include <sys/epoll.h>
#include "ml_comm.h"
#include "ml_exit.h"
#include "ml_timer.h"
#include "ml_listen.h"

typedef struct _MLListener
{
	int iEpollFd;
	int iWaitTime;
	int iListenNum;
	MLListenOp *pStruListenOper;
	MLRecvHandle struRecvHandle;
	MLSendHandle struSendHandle;
	struct epoll_event *pStruEV;
}MLListener, *PMLListener;

static int
ml_dispatch_task(
	int iNumfds,		
	MLListener *pStruListener,
	struct epoll_event *pStruEV,
	MLListenOp *pStruListenOper
)
{
	int i = 0;
	int iRet = 0;	

	ML_ERROR("iNumfds = %d\n", iNumfds);
	for( ; i < iNumfds; i++ )
	{
		if( pStruEV[i].events & (EPOLLERR))
		{
			ML_ERROR("\n");
			if( pStruListenOper && pStruListenOper->pEpollErrFunc )
			{
				pStruListenOper->pEpollErrFunc(pStruEV[i].data.ptr);
			}
		}
		else if( pStruEV[i].events & EPOLLHUP )
		{
			ML_ERROR("\n");
			if( pStruListenOper && pStruListenOper->pEpollHupFunc )
			{
				pStruListenOper->pEpollHupFunc(pStruEV[i].data.ptr);
			}
		}
		else if( pStruEV[i].events & EPOLLRDHUP )
		{
			ML_ERROR("hup up\n");
			if( pStruListenOper && pStruListenOper->pEpollRDHupFunc )
			{
				pStruListenOper->pEpollRDHupFunc(pStruEV[i].data.ptr);
			}
		}
		else if( pStruEV[i].events & EPOLLIN )
		{	
			ML_ERROR("recv\n");
			ml_add_recv_node(pStruEV[i].data.ptr, pStruListener->struRecvHandle);	
		}
		else if( pStruEV[i].events & EPOLLOUT )
		{
			ML_ERROR("recv\n");
			ml_add_send_node(pStruEV[i].data.ptr, pStruListener->struSendHandle);
		}

	}

	return ML_OK;
}

int
ml_start_listener(
	MLListenHandle struHandle
)
{
	int iRet = 0;
	int iSize = 0;
	int iNumfds = 0;
	MLListener *pStruListener = (MLListener*)struHandle;
	struct epoll_event *pStruEV = NULL;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	iSize = pStruListener->iListenNum * sizeof(struct epoll_event);
	memset(pStruListener->pStruEV, 0, iSize);

	iNumfds = epoll_wait(
								pStruListener->iEpollFd, 
								pStruListener->pStruEV, 
								pStruListener->iListenNum, 
								pStruListener->iWaitTime 
							);
	if( iNumfds > 0 )
	{
		ml_dispatch_task( 
										iNumfds, 
										pStruListener,
										pStruListener->pStruEV, 
										pStruListener->pStruListenOper 
									);
		iRet = ML_OK;
	}
	else if( iNumfds < 0 )
	{
		ML_ERROR( "epoll_wait: %s", strerror(errno) );
		iRet = ML_ERR;
	}
	else if( iNumfds == 0 )
	{
		iRet = ML_OK;
	}

	return iRet;
}

int
ml_create_listener(
	int iWaitTime,
	int iListenNum,
	MLListenOp *pStruListenerOper,
	MLRecvHandle struRecvHandle,
	MLSendHandle struSendHandle,
	MLListenHandle *pStruHandle
)
{
	MLListener *pStruListener = NULL;

	ML_CALLOC(pStruListener, MLListener, 1);

	pStruListener->iWaitTime				= iWaitTime;
	pStruListener->struRecvHandle   = struRecvHandle;
	pStruListener->struSendHandle   = struSendHandle;
	pStruListener->pStruListenOper  = pStruListenerOper;
	if( iListenNum )
	{
		pStruListener->iListenNum = iListenNum;
	}
	else
	{
		pStruListener->iListenNum = ML_MAX_EVENT;
	}
	ML_CALLOC(
						pStruListener->pStruEV, 
						struct epoll_event,
						pStruListener->iListenNum
					);
	pStruListener->iEpollFd = epoll_create(1000000);
	if( pStruListener->iEpollFd < 0 )
	{
		ML_ERROR( "gepollfd = %s", strerror(errno));
		return ML_ERR;
	}

	(*pStruHandle) = (void *)pStruListener;

	return ML_OK;
}

int 
ml_destroy_listener(
	MLListenHandle struHandle
)
{
	if( !struHandle )
	{
		return ML_OK;
	}

	ML_FREE(struHandle);

	return ML_OK;
}

int
ml_add_listen_sockfd(
	int iEvent,
	int iSockfd,
	void *pData,
	MLListenHandle struHandle
)
{
	struct epoll_event struEV;
	MLListener *pStruListen = (MLListener*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	memset(&struEV, 0, sizeof(struEV));
	struEV.events = iEvent;
	struEV.data.ptr = pData;
	
	if(epoll_ctl(pStruListen->iEpollFd, EPOLL_CTL_ADD, iSockfd, &struEV) == -1)
	{
		ML_ERROR( "epoll_ctrl err: %s\n", strerror(errno));
		return ML_ERR;
	}

	return ML_OK;
}

int
ml_del_listen_sockfd(
	int iSockfd,
	MLListenHandle struHandle
)
{
	int iRet = 0;
	MLListener *pStruListener = (MLListener *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	iRet = epoll_ctl(pStruListener->iEpollFd, EPOLL_CTL_DEL, iSockfd, NULL);
	if( iRet != 0 )
	{
		ML_ERROR("iSockfd = %d, epoll error: %s\n", iSockfd, strerror(errno));
		return ML_ERR;
	}

	return ML_OK;
}

int
ml_mod_listen_sockfd(
		int iEvent,
		int iSockfd,
		void *pData,
		MLListenHandle struHandle
)
{
	struct epoll_event struEV;
	MLListener *pStruListener = (MLListener *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	memset(&struEV, 0, sizeof(struEV));
	struEV.events |= iEvent;
	struEV.data.ptr = pData;

	if(epoll_ctl(pStruListener->iEpollFd, EPOLL_CTL_MOD, iSockfd, &struEV) == -1)
	{
		ML_ERROR( "iSockfd = %d, epoll_ctrl err: %s\n", iSockfd, strerror(errno));
		return ML_ERR;
	}

	return ML_OK;
}

