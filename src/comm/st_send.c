/*
 *	由于现在需要采用异步建立连接的方式，而且epoll判断连接是否建立成功
 *	使用的是EPOLLOUT，并且处理方式和EPOLLIN有很大不同，因此，在这里新
 *	添加一个send模块，用于处理写事件
 */
#include "push_client.h"
#include "st_hub.h"
#include "st_send.h"
#include "st_boot.h"
#include "st_config.h"
#include "st_handle.h"
#include "st_recv_check.h"

static int giThreadID = 0;

static int
st_send_busy_data(
	PCHubNode *pStruHN,
	PCNatLink *pStruNL,
	PushClient *pStruPC
)
{
	int					  iRet = 0;
	int						iStatus = 0; 
	int						iSendLen =  0;
	unsigned long ulSendData = 0;
	struct sockaddr_in struAddr;

	if( pStruNL )
	{
		pStruNL->ulData = (unsigned long)pStruHN->pEvent;
		ulSendData = pStruNL->ulSendData;
		iSendLen = pStruNL->iSendLen;
		memcpy(&struAddr, &pStruNL->struSendAddr, sizeof(struct sockaddr_in));
		iStatus = pStruNL->iStatus;
	}
	else
	{
		pStruPC->ulData = (unsigned long)pStruHN->pEvent;
		ulSendData = pStruPC->ulSendData;
		iSendLen = pStruPC->iSendLen;
		memcpy(&struAddr, &pStruPC->struSendAddr, sizeof(struct sockaddr_in));
		pthread_mutex_lock(pStruPC->pStruResultMutex);
		iStatus = *(pStruPC->puiStatus);
		pthread_mutex_unlock(pStruPC->pStruResultMutex);
	}
	if( iSendLen && ulSendData )
	{
		iRet = st_send_data(
									pStruHN->iSockfd,
									iSendLen,
									(char *)ulSendData,
									pStruPC,
									&struAddr
								);
		if( iRet != ST_OK )
		{
			if( pStruNL )
			{
				st_set_fail(st_get_step_by_status(iStatus), iRet, pStruPC);
			}
			else
			{
				st_set_fail(st_get_step_by_status(iStatus), iRet, pStruPC);
			}
		}
		return ST_OK;
	}

	return ST_ERR;
}

static void
st_get_send_status(
	PCNatLink  *pStruNL, 
	PushClient *pStruPC,
	int				 *piStatus
)
{
	if( pStruNL )
	{
		*piStatus = pStruNL->iStatus;
	}
	else
	{
		pthread_mutex_lock(pStruPC->pStruResultMutex);
		*piStatus = *(pStruPC->puiStatus);
		pthread_mutex_unlock(pStruPC->pStruResultMutex);
	}
}

static int 
st_link_check(
	int iSockfd,
	PushClient *pStruPC
)
{
	int iRet = 0;
	int iResult = 0;
  socklen_t tResultLen = sizeof(iResult);
	PCConfig struConf;

	//检查是否连接成功
	if( getsockopt(iSockfd, SOL_SOCKET, SO_ERROR, &iResult, &tResultLen) < 0 ) 
	{
		PC_ERROR("getsockopt error:%s\n", strerror(errno));
		return ST_ERR;
	}
	if( iResult != 0 )
	{
		st_mod_write_listen_sockfd((PCEventData*)pStruPC->ulData);
		return ST_FAIL_CANNOT_CONNECT;
	}

	return ST_OK;
}

static int
st_link_handle(
	int				iStatus,
	PCHubNode *pStruHN,
	PushClient *pStruPC
)
{
	PCConfig struConf;

	if( pStruPC->pConnFunc )
	{
		return pStruPC->pConnFunc((unsigned long)pStruPC);
	}
	else
	{
		PC_ERROR("no connect function T_T\n");
		exit(0);
	}

	return ST_OK;
}

static int
st_send(
	struct list_head *pStruNode
)
{
	int iRet = 0;
	int iStatus = 0;
	PCHubNode *pStruHN = NULL;
	PCNatLink *pStruNL = NULL;
	PushClient *pStruPC = NULL, struPC;

	pStruHN = list_entry(pStruNode, PCHubNode, struNode);

	pStruPC = (PushClient *)pStruHN->pData;
	st_read_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, &struPC);

	st_mod_listen_sockfd((PCEventData*)pStruHN->pEvent);

	struPC.ulData = (unsigned long)pStruHN->pEvent;
	struPC.iCurSockfd = pStruHN->iSockfd;
	//st_get_assistan_link(&struPC, &pStruNL);

	st_get_send_status(pStruNL, &struPC, &iStatus);
	if( struPC.ulSendData )
	{
		iRet = st_send_busy_data(pStruHN, pStruNL, &struPC);
		free(pStruHN);
		return iRet;
	}

	iRet = st_link_check(pStruHN->iSockfd, &struPC);
	if( iRet == ST_ERR )	
	{
		free(pStruHN);
		return st_set_fail(ST_CHECK_LINK, iRet, &struPC);
	}
	else if( iRet != ST_OK )
	{
		free(pStruHN);
		return ST_OK;
	}

	iRet = st_link_handle(iStatus, pStruHN, &struPC);
	if( iRet != ST_OK )
	{
		st_set_fail(st_get_step_by_status(iStatus), iRet, &struPC);
	}
	
	free(pStruHN);

	return iRet;
}

int
st_init_send(
	int iThreadNum
)
{
	return st_create_thread_pool(
																			iThreadNum,	
																			st_handle_node_free,
																			NULL,
																			st_send,
																			&giThreadID
																		);
}

int
st_uninit_send()
{
	return ST_OK;
}

int
st_add_send_node(
	PCEventData *pStruED
)
{
	PCHubNode *pStruHub = NULL;

	ST_CALLOC(pStruHub, PCHubNode, 1);
	pStruHub->iSockfd = pStruED->iSockfd;
	pStruHub->sData = NULL;
	pStruHub->pData = pStruED->pData;
	pStruHub->pEvent = pStruED;

	return st_add_thread_pool_node(giThreadID, &pStruHub->struNode);
}
