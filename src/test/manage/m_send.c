#include "m_send.h"

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
	
	if( pStruML->iLinkStatus != M_LINK_STATUS_OK )
	{
		iRet = m_send_check_connect(pStruML->iSockfd);
		if( iRet != ML_OK )
		{
			return;	
		}
		pthread_mutex_lock(&pStruML->struLinkMutex);
		pStruML->iLinkStatus = M_LINK_STATUS_OK;
		pthread_mutex_unlock(&pStruML->struLinkMutex);
	}

	return;
}
