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

static int 
m_recv_function(
	MLink *pStruML,
	int  *piRecvLen,
	char **ssRecvData	
)
{
	int iRet = 0;
	int iSize = 0;
	char cLen[4] = { 0 };

	ml_manager_mod_sockfd(
											(EPOLLONESHOT | EPOLLET | EPOLLOUT),
											pStruML->iSockfd,
											(void *)pStruML,
											pStruML->pStruM->struHandle
										);

	__m_recv_function(pStruML->iSockfd, 4, cLen);
	iSize = ntohl(*(int*)cLen);
	ML_CALLOC(*ssRecvData, char, iSize+1);
	__m_recv_function(pStruML->iSockfd, iSize, *ssRecvData);

	return ML_OK;
}

static int
m_unix_listen_function( 
	MLink *pStruML 
)
{
	int iSize = 0;
	int iSockfd = 0;
	struct in_addr struAddr;
	struct sockaddr_un struUN;
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

	return ML_OK;
}


static int
m_listen_function(
	MLink *pStruML
)
{
	int iSize = 0;
	int iSockfd = 0;
	struct sockaddr_in struAddr;
	MLink *pStruNew = NULL;
	
	ml_manager_mod_sockfd(
										(EPOLLONESHOT | EPOLLET | EPOLLIN),
										pStruML->iSockfd,
										(void *)pStruML,
										pStruML->pStruM->struHandle
									);
	
	iSize = sizeof(struct sockaddr_in);
	memset(&struAddr, 0, iSize);
	iSockfd = accept(pStruML->iSockfd, (struct sockaddr*)&struAddr, &iSize);
	if( iSockfd < 0 )
	{
		ML_ERROR("accept error: %s\n", strerror(errno));
		exit(0);
	}
	
	m_calloc_mlink(
							iSockfd,
							M_LINK_TYPE_NORMAL, 
							struAddr.sin_addr, 
							ntohs(struAddr.sin_port), 
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

	ml_manager_add_recv_check(
												(void *)pStruNew,
												pStruNew->pStruM->struHandle,
												&pStruNew->iRecvCheckID	
											);

	return ML_OK;
}

int
m_recv(
	void *pEventData,
	int  *piRecvLen, 
	char **ssRecvData
)
{
	MLink *pStruML = (MLink *)pEventData;

	ML_ERROR("\n");
	if( !pStruML )
	{
		return ML_PARAM_ERR;
	}

	if( pStruML->iLinkType == M_LINK_TYPE_LISTEN )
	{
		m_listen_function(pStruML);
		return ML_LISTEN;
	}
	else if( pStruML->iLinkType == M_LINK_TYPE_UNIX_LISTEN)
	{
		m_unix_listen_function(pStruML);
		return ML_LISTEN;
	}

	return m_recv_function(pStruML, piRecvLen, ssRecvData);
}

