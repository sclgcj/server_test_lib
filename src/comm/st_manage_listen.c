#include "st_manage.h"

int
st_add_manage_sockfd(
	int  iEvent,
	int  iSockfd,
	void *pData,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}
	
	return st_add_listen_sockfd(iEvent, iSockfd, pData, pStruST->struListenHandle);
}

int
st_del_mamage_sockfd(
	int iSockfd,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	return st_del_listen_sockfd(iSockfd, pStruST->struListenHandle);
}

int
st_mod_manage_sockfd(
	int iEvent,
	int iSockfd, 
	void *pData,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	return st_mod_listen_sockfd(iEvent, iSockfd, pData, pStruST->struListenHandle);
}

static int
st_check_timer_duration(
	unsigned int uiDurationTime,
	STTimerHandle struHandle
)
{
	int iRet = 0;
	int iTick = 0;

	st_get_tick(struHandle, &iTick);

	ST_ERROR("iTick = %d\n", iTick);
	if( iTick >= uiDurationTime )
	{
		return ST_OK;
	}

	return ST_ERR;
}



int
st_start_manage_listener(
	STHandle	struHandle
)
{
	int iRet = 0;
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	while(1)
	{
		iRet = st_check_exit();
		if( iRet == ST_OK )
		{
			ST_ERROR("exit listener thread\n");
			break;
		}
		iRet = st_start_listener(pStruST->struListenHandle);
		if( iRet != ST_OK )
		{
			break;
		}
		iRet = st_check_timer_duration(
																			pStruST->uiDurationTime,
																			pStruST->struTimerHandle 
																		);
		if(iRet == ST_OK)
		{
			break;
		}
	}

	return ST_OK;
}

