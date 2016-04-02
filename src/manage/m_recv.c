#include "m_recv.h"
#include "ml_manage_listen.h"
#include <sys/un.h>

static int
__m_recv_function(
	int iSockfd,
	int iLen,
	char *sData
)
{
	int iRet = 0;

	while(1)
	{
		iRet = recv(iSockfd, sData, iLen, 0);
		if( iRet == 0 )
		{
			ML_ERROR("peer closed");
			return M_PEER_CLOSED;
		}
		
		if( iRet < 0 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			if( errno == ECONNRESET )
			{
				return M_PEER_RESET;
			}
			ML_ERROR("recv error: %s\n", strerror(errno));
			exit(0);
		}
		break;
	}

	return ML_OK;
}

int 
m_recv_function(
	void *pData,
	int  *piRecvLen,
	char **ssRecvData	
)
{
	int iRet = 0;
	int iSize = 0;
	char cLen[4] = { 0 };
	MLink *pStruML = (MLink *)pData;

	iRet = __m_recv_function(pStruML->iSockfd, 4, cLen);
	if( iRet != ML_OK )
	{
		return iRet;
	}
	iSize = ntohl(*(int*)cLen);
	(*piRecvLen) = iSize;
	ML_ERROR("iSize = %d", iSize);
	ML_CALLOC(*ssRecvData, char, iSize+1);
	iRet = __m_recv_function(pStruML->iSockfd, iSize, *ssRecvData);
	if( iRet == ML_OK )
	{
		ML_ERROR("sRecvData = %s\n", *ssRecvData);
	}

	return iRet;
}

int
m_unix_listen_function( 
	void *pEventData,
	int  *piRecvLen, 
	char **ssRecvData
)
{
	int iSize = 0;
	int iSockfd = 0;
	struct in_addr struAddr;
	struct sockaddr_un struUN;
	MLink *pStruML = (MLink *)pEventData;
	MLink *pStruNew = NULL;

	ML_ERROR("\n");
	iSize = sizeof(struct sockaddr_un);
	memset(&struUN, 0, iSize);
	iSockfd = accept(pStruML->iSockfd, (struct sockaddr*)&struUN, &iSize);
	if( iSockfd < 0 )
	{
		ML_ERROR("unix accept error: %s\n", strerror(errno));
		exit(0);
	}

/*	ml_manager_mod_sockfd(
										(EPOLLONESHOT | EPOLLET | EPOLLIN),
										pStruML->iSockfd,
										(void *)pStruML,
										pStruML->pStruM->struHandle
									);*/

	struAddr.s_addr = 0;
	m_calloc_mlink(
							iSockfd,
							M_LINK_TYPE_UNIX_NORMAL, 
							struAddr, 
							0,
							m_recv_function,
							NULL,
							pStruML->pStruM,
							&pStruNew
					);

	ml_manager_add_sockfd(
												(EPOLLONESHOT | EPOLLET | EPOLLIN),
												iSockfd,
												(void *)pStruNew,
												pStruML->pStruM->struHandle
											);

	/*ml_manager_add_client_data(
											 (void*)pStruNew, 
											 pStruNew->pStruM->struHandle, 
											 &pStruNew->iDataID 
											);*/

	return ML_LISTEN;
}

int
m_listen_function(
	void *pEventData,
	int  *piRecvLen, 
	char **ssRecvData
)
{
	int iSize = 0;
	int iSockfd = 0;
	struct sockaddr_in struAddr;
	MLink *pStruML = (MLink*)pEventData;
	MLink *pStruNew = NULL;
	
	iSize = sizeof(struct sockaddr_in);
	memset(&struAddr, 0, iSize);
	ML_ERROR("\n");
	iSockfd = accept(pStruML->iSockfd, (struct sockaddr*)&struAddr, &iSize);
	if( iSockfd < 0 )
	{
		ML_ERROR("accept error: %s\n", strerror(errno));
		exit(0);
	}

	ML_ERROR("\n");
	/*ml_manager_mod_sockfd(
										(EPOLLONESHOT | EPOLLET | EPOLLIN),
										pStruML->iSockfd,
										(void *)pStruML,
										pStruML->pStruM->struHandle
									);
*/	
	
	m_calloc_mlink(
							iSockfd,
							M_LINK_TYPE_NORMAL, 
							struAddr.sin_addr, 
							ntohs(struAddr.sin_port), 
							m_recv_function,
							NULL,
							pStruML->pStruM,
							&pStruNew
					);
	pStruNew->iLinkStatus = 1;

	/*ml_manager_add_client_data(
											 (void*)pStruNew, 
											 pStruNew->pStruM->struHandle, 
											 &pStruNew->iDataID 
											);*/

	ml_manager_add_recv_check(
												(void *)pStruNew,
												pStruNew->pStruM->struHandle,
												&pStruNew->iRecvCheckID	
											);

	ml_manager_add_sockfd(
												(EPOLLONESHOT | EPOLLET | EPOLLIN),
												iSockfd,
												(void *)pStruNew,
												pStruML->pStruM->struHandle
											);



	return ML_LISTEN;
}

int
m_recv(
	void *pEventData,
	int  *piRecvLen, 
	char **ssRecvData
)
{
	int iRet = ML_OK;
	MLink *pStruML = (MLink *)pEventData;

	ML_ERROR("\n");
	if( !pStruML )
	{
		return ML_PARAM_ERR;
	}

	if( pStruML->pRecvFunc )
	{
		ML_ERROR("\n");
		iRet = pStruML->pRecvFunc((void*)pStruML, piRecvLen, ssRecvData);
	}

	if( iRet != ML_OK && iRet != ML_LISTEN )
	{
		m_free_mlink(pStruML);
	}
	else
	{
		ml_manager_mod_sockfd(
												(EPOLLONESHOT | EPOLLET | EPOLLIN),
												pStruML->iSockfd,
												(void *)pStruML,
												pStruML->pStruM->struHandle
											);
	}

	return iRet;
}

