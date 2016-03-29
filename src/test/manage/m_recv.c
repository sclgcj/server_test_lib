#include "m_recv.h"

static int 
m_recv_function(
	void *pEventData,
	int  *piRecvLen,
	char **ssRecvData	
)
{
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

	return m_recv_function(pStruML, piRecvLen, ssRecvData);
}

