#include <sys/epoll.h>
#include "server_test_comm.h"
#include "server_test_exit.h"
#include "server_test_timer.h"
#include "server_test_listen.h"

typedef struct _STListener
{
	int iEpollFd;
	int iWaitTime;
	int iListenNum;
	STListenOp *pStruListenOper;
	struct epoll_event *pStruEV;
}STListener, *PSTListener;

static int
server_test_dispatch_task(
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

	return SERVER_TEST_OK;
}

int
server_test_start_listener(
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
		return SERVER_TEST_PARAM_ERR;
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
		server_test_dispatch_task( 
													iNumfds, 
													pStruListener->pStruEV, 
													pStruListener->pStruListenOper 
												);
		iRet = SERVER_TEST_OK;
	}
	else if( iNumfds < 0 )
	{
		ST_ERROR( "epoll_wait: %s", strerror(errno) );
		iRet = SERVER_TEST_ERR;
	}
	else if( iNumfds == 0 )
	{
		iRet = SERVER_TEST_OK;
	}

	SERVER_TEST_FREE(pStruEV);

	return iRet;
}

int
server_test_create_listener(
	int iWaitTime,
	int iListenNum,
	STListenOp *pStruListenerOper,
	STListenHandle *pStruHandle
)
{
	STListener *pStruListener = NULL;

	if( !pStruListenerOper )
	{
		return SERVER_TEST_ERR;
	}

	SERVER_TEST_CALLOC(pStruListener, STListener, 1);

	pStruListener->iWaitTime				= iWaitTime;
	pStruListener->pStruListenOper  = pStruListenerOper;
	if( iListenNum )
	{
		pStruListener->iListenNum = iListenNum;
	}
	else
	{
		pStruListener->iListenNum = SERVER_TEST_MAX_EVENT;
	}
	SERVER_TEST_CALLOC(
						pStruListener->pStruEV, 
						struct epoll_event,
						pStruListener->iListenNum
					);
	pStruListener->iEpollFd = epoll_create(1000000);
	if( pStruListener->iEpollFd < 0 )
	{
		ST_ERROR( "gepollfd = %s", strerror(errno));
		return SERVER_TEST_ERR;
	}

	(*pStruHandle) = (void *)pStruListener;

	return SERVER_TEST_OK;
}

int 
server_test_destroy_listener(
	STListenHandle struHandle
)
{
	if( !struHandle )
	{
		return SERVER_TEST_OK;
	}

	SERVER_TEST_FREE(struHandle);

	return SERVER_TEST_OK;
}

int
server_test_add_listen_sockfd(
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
		return SERVER_TEST_PARAM_ERR;
	}

	memset(&struEV, 0, sizeof(struEV));
	struEV.events = iEvent;
	struEV.data.ptr = pData;
	
	if(epoll_ctl(pStruListen->iEpollFd, EPOLL_CTL_ADD, iSockfd, &struEV) == -1)
	{
		ST_ERROR( "epoll_ctrl err: %s\n", strerror(errno));
		return SERVER_TEST_ERR;
	}

	return SERVER_TEST_OK;
}

int
server_test_del_listen_sockfd(
	int iSockfd,
	STListenHandle struHandle
)
{
	int iRet = 0;
	STListener *pStruListener = (STListener *)struHandle;

	if( !struHandle )
	{
		return SERVER_TEST_PARAM_ERR;
	}

	iRet = epoll_ctl(pStruListener->iEpollFd, EPOLL_CTL_DEL, iSockfd, NULL);
	if( iRet != 0 )
	{
		ST_ERROR("iSockfd = %d, epoll error: %s\n", iSockfd, strerror(errno));
		return SERVER_TEST_ERR;
	}

	return SERVER_TEST_OK;
}

int
server_test_mod_listen_sockfd(
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
		return SERVER_TEST_PARAM_ERR;
	}

	memset(&struEV, 0, sizeof(struEV));
	struEV.events |= iEvent;
	struEV.data.ptr = pData;

	if(epoll_ctl(pStruListener->iEpollFd, EPOLL_CTL_MOD, iSockfd, &struEV) == -1)
	{
		ST_ERROR( "iSockfd = %d, epoll_ctrl err: %s\n", iSockfd, strerror(errno));
		return SERVER_TEST_ERR;
	}

	return SERVER_TEST_OK;
}


