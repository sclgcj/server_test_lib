#include "server_test_manage.h"

int
server_test_add_manage_sockfd(
	int  iEvent,
	int  iSockfd,
	void *pData,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return SERVER_TEST_PARAM_ERR;
	}
	
	return server_test_add_listen_sockfd(iEvent, iSockfd, pData, pStruST->struListenHandle);
}

int
server_test_del_mamage_sockfd(
	int iSockfd,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return SERVER_TEST_PARAM_ERR;
	}

	return server_test_del_listen_sockfd(iSockfd, pStruST->struListenHandle);
}

int
server_test_mod_manage_sockfd(
	int iEvent,
	int iSockfd, 
	void *pData,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return SERVER_TEST_PARAM_ERR;
	}

	return server_test_mod_listen_sockfd(iEvent, iSockfd, pData, pStruST->struListenHandle);
}

static int
server_test_check_timer_duration(
	unsigned int uiDurationTime,
	STTimerHandle struHandle
)
{
	int iRet = 0;
	int iTick = 0;

	server_test_get_tick(struHandle, &iTick);

	ST_ERROR("iTick = %d\n", iTick);
	if( iTick >= uiDurationTime )
	{
		return SERVER_TEST_OK;
	}

	return SERVER_TEST_ERR;
}



int
server_test_start_manage_listener(
	STHandle	struHandle
)
{
	int iRet = 0;
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return SERVER_TEST_PARAM_ERR;
	}

	while(1)
	{
		iRet = server_test_check_exit();
		if( iRet == SERVER_TEST_OK )
		{
			ST_ERROR("exit listener thread\n");
			break;
		}
		iRet = server_test_start_listener(pStruST->struListenHandle);
		if( iRet != SERVER_TEST_OK )
		{
			break;
		}
		iRet = server_test_check_timer_duration(
																			pStruST->uiDurationTime,
																			pStruST->struTimerHandle 
																		);
		if(iRet == SERVER_TEST_OK)
		{
			break;
		}
	}

	return SERVER_TEST_OK;
}

