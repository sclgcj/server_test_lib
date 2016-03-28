#include "ml_manage_listen.h"

int
ml_manager_add_sockfd(
	int  iEvent,
	int  iSockfd,
	void *pData,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	
	return ml_add_listen_sockfd(iEvent, iSockfd, pData, pStruML->struListenHandle);
}

int
ml_mamage_del_sockfd(
	int iSockfd,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_del_listen_sockfd(iSockfd, pStruML->struListenHandle);
}

int
ml_manager_mod_sockfd(
	int iEvent,
	int iSockfd, 
	void *pData,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_mod_listen_sockfd(iEvent, iSockfd, pData, pStruML->struListenHandle);
}

static int
ml_check_timer_duration(
	unsigned int uiDurationTime,
	MLTimerHandle struHandle
)
{
	int iRet = 0;
	int iTick = 0;

	ml_get_tick(struHandle, &iTick);

	ML_ERROR("iTick = %d\n", iTick);
	if( iTick >= uiDurationTime )
	{
		return ML_OK;
	}

	return ML_ERR;
}

int
ml_manager_start_listener(
	MLHandle	struHandle
)
{
	int iRet = 0;
	ServerTest *pStruML = (ServerTest *)struHandle;

	ML_ERROR("\n");
	if( !struHandle )
	{
	ML_ERROR("\n");
		return ML_PARAM_ERR;
	}

	ML_ERROR("\n");
	while(1)
	{
		iRet = ml_check_exit();
		if( iRet == ML_OK )
		{
			ML_ERROR("exit listener thread\n");
			break;
		}
		ML_ERROR("\n");
		iRet = ml_start_listener(pStruML->struListenHandle);
		if( iRet != ML_OK )
		{
			break;
		}
		iRet = ml_check_timer_duration(
																	pStruML->uiDurationTime,
																	pStruML->struTimerHandle 
																);
		if(iRet == ML_OK)
		{
			break;
		}
	}

	return ML_OK;
}
