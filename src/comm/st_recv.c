#include "push_client.h"
#include "st_hub.h"
#include "st_recv.h"
#include "st_handle.h"

#include <sys/epoll.h>

static int giThreadID = 0;

static int
st_set_recv_fail(
	int				 iRet, 
	PushClient *pStruPC
)
{
	int iStatus = 0;
	PCNatLink *pStruNL = NULL;

	{
		pthread_mutex_lock(pStruPC->pStruResultMutex);
		iStatus = *(pStruPC->puiStatus);
		pthread_mutex_unlock(pStruPC->pStruResultMutex);
	}
	return st_set_fail(st_get_step_by_status((iStatus)), iRet, pStruPC);
}

static void
st_clear_recv_param(
	PushClient *pStruPC
)
{
	PCNatLink *pStruNL = NULL;

	{
		pStruPC->iRecvCnt = 0;
		pStruPC->iRecvLen = 0;
		pStruPC->ulRecvData = 0;
		st_write_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, pStruPC);
	}
}

static int
__st_recv(
	PCHubNode *pStruHN,
	PushClient *pStruPC
)
{
	int iRet = 0;
	PCNatLink *pStruNL = NULL;

	pStruPC->iCurSockfd = pStruHN->iSockfd;
	iRet = st_recv_data_with_malloc(pStruHN->iSockfd, pStruPC, &pStruHN->sData);	
	if( iRet == ST_OK || iRet == ST_FAIL_WOULDBLOCK )
	{
		st_mod_listen_sockfd((PCEventData*)pStruHN->pEvent);
		if( iRet != ST_OK )
		{
			goto eblock;
		}
		else
		{
			if( pStruPC->iRecvCnt != pStruPC->iRecvLen )
			{
				goto eblock;
			}
			else
			{
				st_clear_recv_param(pStruPC);
				iRet = st_add_handle_node(&pStruHN->struNode);
				goto back;
			}
		}
	}
	else
	{
		st_clear_recv_param(pStruPC);
		if( iRet == ST_FAIL_PROGRAM_EXIT )
		{
			goto out;
		}
		st_set_recv_fail(iRet, pStruPC);
		PC_ERROR("fsdfsdfs999 -- sData = %p\n", pStruHN->sData);
		goto out;
	}

eblock:
	iRet = ST_FAIL_WOULDBLOCK;
	pStruHN->sData = NULL;
	goto back;
out:
	iRet = ST_ERR;
back:
	return iRet;
}

static int
st_accept_client(
	PCHubNode  *pStruHN,
	PushClient *pStruPC
)
{
	int iSize = sizeof(struct sockaddr_in);
	int iCurPortCnt = 0;
	void *pData = NULL;
	struct sockaddr_in struAddr;
	
	memset(&struAddr, 0, iSize);
	pStruPC->iNatSockfd = accept(pStruHN->iSockfd, (struct sockaddr *)&struAddr, &iSize);
	if( pStruPC->iNatSockfd < 0 )
	{
		if( errno == EAGAIN || errno == EWOULDBLOCK )
		{
			st_mod_listen_sockfd((PCEventData *)pStruHN->pEvent);
			return ST_ERR;
		}
		PC_ERROR("accept error: %s\n", strerror(errno));
		return ST_ERR;
	}
	pStruPC->struPeerIP = struAddr.sin_addr;
	pStruPC->usPeerPort = ntohs(struAddr.sin_port);
	PC_DEBUG("iNatSockfd = %d == %s:%d\n", pStruPC->iNatSockfd, inet_ntoa(pStruPC->struPeerIP), pStruPC->usPeerPort);

	pthread_mutex_lock(pStruPC->pStruAssistantMutex);
	iCurPortCnt = (*pStruPC->pusMapPortCnt);
	(*pStruPC->pusMapPortCnt) += 1;
	pthread_mutex_unlock(pStruPC->pStruAssistantMutex);

	st_get_pushclient_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, &pData);
	pStruPC->iEvents = (EPOLLIN | EPOLLET | EPOLLONESHOT);
	st_add_listen_sockfd(iCurPortCnt, pStruPC->iNatSockfd, pData, pStruPC);

	return ST_CONNECT_TO_SERVER;
}

static int
st_recv(
	struct list_head *pStruNode
)
{
	int iRet = 0;
	PCHubNode *pStruHN = NULL;
	PushClient struPC, *pStruPC = NULL;

	pStruHN = list_entry(pStruNode, PCHubNode, struNode);

	pStruPC = (PushClient *)pStruHN->pData;
	memset(&struPC, 0, sizeof(struPC));
	st_read_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, &struPC);

	PC_DEBUG("listen fd = %d == %d\n", pStruPC->iListenPeerFd, pStruHN->iSockfd);
	if( pStruPC->iListenPeerFd == pStruHN->iSockfd )
	{
		iRet = st_accept_client(pStruHN, &struPC);
	}
	else
	{
		iRet = __st_recv(pStruHN, &struPC);
	}

	if( iRet != ST_OK )
	{
		free(pStruHN);
		pStruHN = NULL;
	}

	return iRet;
}

int
st_init_recv( 
	int iThreadNum
)
{
	return st_create_thread_pool(
																	iThreadNum, 
																	st_handle_node_free,
																	NULL,
																	st_recv,
																	&giThreadID
																);
}

int
st_uninit_recv()
{
	return ST_OK;
}

int
st_add_recv_node(
	PCEventData *pStruED
)
{
	PCHubNode *pStruHub = NULL;

	ST_CALLOC(pStruHub, PCHubNode, 1);
	/*pStruHub = (PCHubNode *)calloc(1, sizeof(PCHubNode));
	if( !pStruHub )
	{
		PC_ERROR("calloc: %s\n", strerror(errno));
		exit(0);
	}*/
	pStruHub->iSockfd = pStruED->iSockfd;
	pStruHub->sData = NULL;
	pStruHub->pData = pStruED->pData;
	pStruHub->pEvent = pStruED;

	return st_add_thread_pool_node(giThreadID, &pStruHub->struNode);
}

