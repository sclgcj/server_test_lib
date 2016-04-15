#include "m_send.h"
#include "m_error.h"
#include "manage.h"


int
m_send_data(
	int  iSockfd,
	char *sRawData
)
{
	int iRet = 0;
	int iLen = 0;
	char *sSendData = NULL;

	iLen = strlen(sRawData) + 4;
	ML_CALLOC(sSendData, char, iLen + 1);

	sprintf(sSendData + 4, "%s", sRawData);
	*(int*)sSendData = htonl(strlen(sRawData));

	ML_ERROR("len = %d\n", strlen(sRawData));
	
	while( 1 )
	{
		iRet = send(iSockfd, sSendData, iLen, 0);
		if( iRet < 0 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			ML_ERROR("send error = %s\n", strerror(errno));
			free(sSendData);
			exit(0);
		}
		break;
	}
	ML_FREE(sSendData);
	return ML_OK;
}

int
m_send_check_connect(
	int iSockfd
)
{
	int iResult = 0;
	socklen_t tResultLen = sizeof(iResult);

	if( getsockopt(iSockfd, SOL_SOCKET, SO_ERROR, &iResult, &tResultLen) < 0 )
	{
		ML_ERROR("getsockopt error: %s\n", strerror(errno));
		return ML_ERR;
	}
	if( iResult != 0 )
	{
		return ML_CANNOT_CONNECT;
	}

	return ML_OK;
}

void
m_send(
	void *pUserData 
)
{
	int iRet = 0;
	MLink *pStruML = (MLink *)pUserData;

	if( !pUserData )
	{
		return;
	}
	
	if( pStruML->iLinkStatus < M_STATUS_OK )
	{
		iRet = m_send_check_connect(pStruML->iSockfd);
		if( iRet != ML_OK )
		{
			return;	
		}
		pthread_mutex_lock(&pStruML->struLinkMutex);
		pStruML->iLinkStatus = M_STATUS_OK;
		pthread_mutex_unlock(&pStruML->struLinkMutex);
		ML_ERROR("pStruML->iLinkStatus = %d\n", pStruML->iLinkStatus);
		ml_manager_mod_sockfd(
											(EPOLLONESHOT|EPOLLIN|EPOLLET),
											pStruML->iSockfd,
											(void *)pStruML,
											pStruML->pStruM->struHandle
										);
		if( pStruML->pSendFunc )
		{
			pStruML->pSendFunc((void*)pStruML);
		}
	}

	return;
}
