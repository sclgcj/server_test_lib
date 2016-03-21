#include "server_test_exit.h"
#include "server_test_manage.h"

void
server_test_create_manager(
	unsigned int uiDurationTime,
	STHandle     *pStruHandle
)
{
	ServerTest *pStruST = NULL;

	server_test_init_exit();

	SERVER_TEST_CALLOC(pStruST, ServerTest, 1);
	
	pStruST->uiDurationTime = uiDurationTime;

	(*pStruHandle) = (STHandle)pStruST;
}

int
server_test_create_manager_listener(
	int iWaitTime,
	int iListenNum,
	STListenOp *pStruListeOp,
	STHandle   struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !pStruST )
	{
		return SERVER_TEST_PARAM_ERR;
	}
	
	return server_test_create_listener( 
																iWaitTime,
																iListenNum, 
																pStruListeOp, 
																&pStruST->struListenHandle
															);
}

int
server_test_create_manager_thread(
	int iThreadGroupNum,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !pStruST )
	{
		return SERVER_TEST_PARAM_ERR;
	}

	return server_test_create_thread_table(
																		iThreadGroupNum,
																		&pStruST->struThreadHandle
																	);
}

int
server_test_create_manager_timer(
	int iTimerNum,
	int iThreadNum,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !pStruST )
	{
		return SERVER_TEST_PARAM_ERR;
	}
	if( !pStruST->struThreadHandle )
	{
		ST_ERROR("sfsdfs\n");
	}

	return server_test_create_timer(
															iTimerNum,
															iThreadNum,
															pStruST->struThreadHandle,
															&pStruST->struTimerHandle
														);
}

int
server_test_create_manager_all(
	int iWaitTime,
	int iListenNum,			
	unsigned int uiDurationTime,
	int iThreadNum,
	int	iThreadGroupNum, 
	int iTimerNum,
	STListenOp *pStruListenOp,
	STHandle *pStruHandle
)
{
	int iRet = 0;
	ServerTest *pStruST = NULL;	

	SERVER_TEST_CALLOC(pStruST, ServerTest, 1);
	pStruST->uiDurationTime = uiDurationTime;
	
	iRet = server_test_create_listener(
												iWaitTime, 
												iListenNum, 
												pStruListenOp, 
												pStruST->struListenHandle
											);
	if( iRet != SERVER_TEST_OK )
	{
		return iRet;
	}

	iRet = server_test_create_thread_table(iThreadGroupNum, &pStruST->struThreadHandle);
	if( iRet != SERVER_TEST_OK )
	{
		return iRet;
	}

	iRet = server_test_create_timer(
														iThreadNum, 
														iTimerNum, 
														pStruST->struThreadHandle, 
														&pStruST->struTimerHandle 
													);
	if( iRet != SERVER_TEST_OK )
	{
		return iRet;
	}

	(*pStruHandle) = (STHandle)pStruST;

	return SERVER_TEST_OK;
}

int
server_test_destroy_manager(
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return SERVER_TEST_OK;
	}

	server_test_destroy_listener(pStruST->struListenHandle);

	server_test_destroy_thread_table(pStruST->struThreadHandle);

	server_test_destroy_timer(pStruST->struTimerHandle);

	return SERVER_TEST_OK;
}


