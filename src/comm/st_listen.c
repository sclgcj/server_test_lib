#include <sys/epoll.h>
#include "st_comm.h"
#include "st_exit.h"
#include "st_timer.h"
#include "st_listen.h"

typedef struct _STListener
{
	int iEpollFd;
	int iWaitTime;
	int iListenNum;
	STListenOp *pStruListenOper;
	struct epoll_event *pStruEV;
}STListener, *PSTListener;

static int
st_dispatch_task(
	int iNumfds,		
	struct epoll_event *pStruEV,
	STListenOp *pStruListenOper
)
{
	int i = 0;
	int iRet = 0;	

	for( ; i < iNumfds; i++ )
	{
		if( pStruEV[i].events & EPOLLIN )
		{	
			if( pStruListenOper->pEpollInFunc )
			{
				pStruListenOper->pEpollInFunc(pStruEV[i].data.ptr);
			}
		}
		else if( pStruEV[i].events & EPOLLOUT )
		{
			if( pStruListenOper->pEpollOutFunc )
			{
				pStruListenOper->pEpollOutFunc(pStruEV[i].data.ptr);
			}
		}
		else if( pStruEV[i].events & EPOLLERR )
		{
			if( pStruListenOper->pEpollErrFunc )
			{
				pStruListenOper->pEpollErrFunc(pStruEV[i].data.ptr);
			}
		}
		else if( pStruEV[i].events & (EPOLLHUP) )
		{
			if( pStruListenOper->pEpollHupFunc )
			{
				pStruListenOper->pEpollHupFunc(pStruEV[i].data.ptr);
			}
		}
		else if( pStruEV[i].events & EPOLLRDHUP )
		{
			if( pStruListenOper->pEpollRDHupFunc )
			{
				pStruListenOper->pEpollRDHupFunc(pStruEV[i].data.ptr);
			}
		}
	}

	return ST_OK;
}

int
st_start_listener(
	STListenHandle struHandle
)
{
	int iRet = 0;
	int iSize = 0;
	int iNumfds = 0;
	STListener *pStruListener = (STListener*)struHandle;
	struct epoll_event *pStruEV = NULL;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
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
		st_dispatch_task( 
													iNumfds, 
													pStruListener->pStruEV, 
													pStruListener->pStruListenOper 
												);
		iRet = ST_OK;
	}
	else if( iNumfds < 0 )
	{
		ST_ERROR( "epoll_wait: %s", strerror(errno) );
		iRet = ST_ERR;
	}
	else if( iNumfds == 0 )
	{
		iRet = ST_OK;
	}

	ST_FREE(pStruEV);

	return iRet;
}

int
st_create_listener(
	int iWaitTime,
	int iListenNum,
	STListenOp *pStruListenerOper,
	STListenHandle *pStruHandle
)
{
	STListener *pStruListener = NULL;

	if( !pStruListenerOper )
	{
		return ST_ERR;
	}

	ST_CALLOC(pStruListener, STListener, 1);

	pStruListener->iWaitTime				= iWaitTime;
	pStruListener->pStruListenOper  = pStruListenerOper;
	if( iListenNum )
	{
		pStruListener->iListenNum = iListenNum;
	}
	else
	{
		pStruListener->iListenNum = ST_MAX_EVENT;
	}
	ST_CALLOC(
						pStruListener->pStruEV, 
						struct epoll_event,
						pStruListener->iListenNum
					);
	pStruListener->iEpollFd = epoll_create(1000000);
	if( pStruListener->iEpollFd < 0 )
	{
		ST_ERROR( "gepollfd = %s", strerror(errno));
		return ST_ERR;
	}

	(*pStruHandle) = (void *)pStruListener;

	return ST_OK;
}

int 
st_destroy_listener(
	STListenHandle struHandle
)
{
	if( !struHandle )
	{
		return ST_OK;
	}

	ST_FREE(struHandle);

	return ST_OK;
}

int
st_add_listen_sockfd(
	int iEvent,
	int iSockfd,
	void *pData,
	STListenHandle struHandle
)
{
	struct epoll_event struEV;
	STListener *pStruListen = (STListener*)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	memset(&struEV, 0, sizeof(struEV));
	struEV.events = iEvent;
	struEV.data.ptr = pData;
	
	if(epoll_ctl(pStruListen->iEpollFd, EPOLL_CTL_ADD, iSockfd, &struEV) == -1)
	{
		ST_ERROR( "epoll_ctrl err: %s\n", strerror(errno));
		return ST_ERR;
	}

	return ST_OK;
}

int
st_del_listen_sockfd(
	int iSockfd,
	STListenHandle struHandle
)
{
	int iRet = 0;
	STListener *pStruListener = (STListener *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	iRet = epoll_ctl(pStruListener->iEpollFd, EPOLL_CTL_DEL, iSockfd, NULL);
	if( iRet != 0 )
	{
		ST_ERROR("iSockfd = %d, epoll error: %s\n", iSockfd, strerror(errno));
		return ST_ERR;
	}

	return ST_OK;
}

int
st_mod_listen_sockfd(
		int iEvent,
		int iSockfd,
		void *pData,
		STListenHandle struHandle
)
{
	struct epoll_event struEV;
	STListener *pStruListener = (STListener *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	memset(&struEV, 0, sizeof(struEV));
	struEV.events |= iEvent;
	struEV.data.ptr = pData;

	if(epoll_ctl(pStruListener->iEpollFd, EPOLL_CTL_MOD, iSockfd, &struEV) == -1)
	{
		ST_ERROR( "iSockfd = %d, epoll_ctrl err: %s\n", iSockfd, strerror(errno));
		return ST_ERR;
	}

	return ST_OK;
}


