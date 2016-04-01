#include "m_recv.h"
#include <sys/un.h>

static void
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
		if( iRet < 0 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			ML_ERROR("recv error: %s\n", strerror(errno));
			exit(0);
		}
		break;
	}
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

	ML_ERROR("\n");
	ml_manager_mod_sockfd(
											(EPOLLONESHOT | EPOLLET | EPOLLOUT),
											pStruML->iSockfd,
											(void *)pStruML,
											pStruML->pStruM->struHandle
										);

	__m_recv_function(pStruML->iSockfd, 4, cLen);
	iSize = ntohl(*(int*)cLen);
	(*piRecvLen) = iSize;
	ML_ERROR("iSize = %d", iSize);
	if( iSize == 0 )
	{
		return ML_ERR;
	}
	ML_CALLOC(*ssRecvData, char, iSize+1);
	__m_recv_function(pStruML->iSockfd, iSize, *ssRecvData);
	ML_ERROR("sRecvData = %s\n", *ssRecvData);

	return ML_OK;
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

	ml_manager_mod_sockfd(
										(EPOLLONESHOT | EPOLLET | EPOLLIN),
										pStruML->iSockfd,
										(void *)pStruML,
										pStruML->pStruM->struHandle
									);

	iSize = sizeof(struct sockaddr_un);
	memset(&struUN, 0, iSize);
	iSockfd = accept(pStruML->iSockfd, (struct sockaddr*)&struUN, &iSize);
	if( iSockfd < 0 )
	{
		ML_ERROR("unix accept error: %s\n", strerror(errno));
		exit(0);
	}

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

	ml_manager_add_client_data(
											 (void*)pStruNew, 
											 pStruNew->pStruM->struHandle, 
											 &pStruNew->iDataID 
											);

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
	ml_manager_mod_sockfd(
										(EPOLLONESHOT | EPOLLET | EPOLLIN),
										pStruML->iSockfd,
										(void *)pStruML,
										pStruML->pStruM->struHandle
									);
	
	
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

	ml_manager_add_client_data(
											 (void*)pStruNew, 
											 pStruNew->pStruM->struHandle, 
											 &pStruNew->iDataID 
											);

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

	return iRet;
}

