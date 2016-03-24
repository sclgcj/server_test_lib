#include "st_exit.h"
#include "st_manage.h"

void
st_create_manager(
	unsigned int uiDurationTime,
	STHandle     *pStruHandle
)
{
	ServerTest *pStruST = NULL;

	st_init_exit();

	ST_CALLOC(pStruST, ServerTest, 1);
	
	pStruST->uiDurationTime = uiDurationTime;

	(*pStruHandle) = (STHandle)pStruST;
}

int
st_manager_create_listener(
	int iWaitTime,
	int iListenNum,
	STListenOp *pStruListeOp,
	STHandle   struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !pStruST )
	{
		return ST_PARAM_ERR;
	}

	if( !pStruST->struRecvHandle )
	{
		return ST_RECV_NOT_INIT;
	}
	
	return st_create_listener( 
														iWaitTime,
														iListenNum, 
														pStruListeOp, 
														pStruST->struRecvHandle,
														&pStruST->struListenHandle
													);
}

int
st_manager_create_thread(
	int iThreadGroupNum,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !pStruST )
	{
		return ST_PARAM_ERR;
	}

	if( iThreadGroupNum <= 0 )
	{
		iThreadGroupNum = ST_DEFAULT_GROUP_NUM;
	}

	return st_create_thread_table(
																		iThreadGroupNum,
																		&pStruST->struThreadHandle
																	);
}

int
st_manager_create_timer(
	int iTimerNum,
	int iThreadNum,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !pStruST )
	{
		return ST_PARAM_ERR;
	}
	if( !pStruST->struThreadHandle )
	{
		ST_ERROR("sfsdfs\n");
	}

	if( iTimerNum <= 0 )
	{
		iTimerNum = ST_DEFAULT_TIMER_NUM;
	}
	if( iThreadNum <= 0 )
	{
		iThreadNum = ST_DEFAULT_THREAD_NUM;
	}

	return st_create_timer(
										iTimerNum,
										iThreadNum,
										pStruST->struThreadHandle,
										&pStruST->struTimerHandle
									);
}

int
st_manager_create_hub(
	int iHubNum,
	int iThreadNum,
	int iStackSize,
	STHubFunc pHubFunc,
	STHubHandleFunc pHubHandleFunc,
	STHandle        struHandle
)
{
	ServerTest *pStruST = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}
	if( !pStruST->struTimerHandle )
	{
		return ST_TIMER_NOT_INIT;
	}
	if( !pStruST->struThreadHandle )
	{
		return ST_THREAD_NOT_INIT;
	}

	if( iHubNum <= 0 )
	{
		iHubNum = ST_DEFAULT_HUB_TABLE_SIZE;
	}
	if( iStackSize <= 0 )
	{
		iStackSize = ST_DEFAULT_THREAD_STACK_SIZE;;
	}
	if( iThreadNum )
	{
		iThreadNum = ST_DEFAULT_THREAD_NUM;
	}

	return st_create_hub(
									iHubNum,
									iThreadNum,
									iStackSize,
									pHubFunc,
									pHubHandleFunc,
									pStruST->struTimerHandle,
									pStruST->struThreadHandle,
									&pStruST->struHubHandle
								);
}

int
st_manager_create_opt_config(
	int  iArgc,
	char **ssArgv,
	char *sParseFmt,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	st_create_opt_config(iArgc, ssArgv, sParseFmt, &pStruST->struOptHandle);
}

int
st_manager_create_read_config(
	char *sFile,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	return st_create_read_config(sFile, &pStruST->struRCHandle);
}

int
st_manager_create_link_handle(
	void              *pUserData,
	STCLParam         *pStruCLParam,
	STCreateLinkFunc  pCLFunc,
	STHandle					struHandle
)
{
	ServerTest *pStruST = (ServerTest*)struHandle;

	if( !struHandle || !pStruCLParam )
	{
		return ST_PARAM_ERR;
	}

	if( !pStruST->struThreadHandle )
	{
		return ST_THREAD_NOT_INIT;
	}
	if( pStruCLParam->iThreadNum <= 0 )
	{
		pStruCLParam->iThreadNum = ST_DEFAULT_THREAD_NUM;	
	}
	if( pStruCLParam->iStackSize <= 0 )
	{
		pStruCLParam->iStackSize = ST_DEFAULT_THREAD_STACK_SIZE;
	}

	return st_create_link_handle(
														pUserData,
														pStruCLParam,
														pCLFunc,
														pStruST->struThreadHandle,
														&pStruST->struCLHandle
													);
}

int
st_manager_create_recv_check(
	int  iTotalLink,
	int  iRecvTimerout,
	int  iCheckListNum,
	STRecvCheckFailFunc pFunc,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	if( iCheckListNum <= 0 )
	{
		iCheckListNum = ST_DEFAULT_CHECK_SIZE;
	}

	return st_create_recv_check(
												iTotalLink,
												iRecvTimerout,
												iCheckListNum,
												pFunc,
												pStruST->struTimerHandle,
												&pStruST->struRecvCheckHandle
											);
}

int
st_manager_create_recv(
	int iThreadNum,
	int iStackSize,
	STRecvFunc pFunc,
	STHandle   struHandle
)
{
	ServerTest *pStruST = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	if( !pStruST->struThreadHandle )
	{
		return ST_THREAD_NOT_INIT;
	}
	if( !pStruST->struDisposeHandle )
	{
		return ST_DISPOSE_NOT_INIT;
	}

	if( iThreadNum <= 0 )
	{
		iThreadNum = ST_DEFAULT_THREAD_NUM;
	}
	if( iStackSize <= 0 )
	{
		iThreadNum = ST_DEFAULT_THREAD_STACK_SIZE;
	}

	return st_create_recv_handle(
													iThreadNum,
													iStackSize,
													pFunc,
													pStruST->struThreadHandle,
													pStruST->struDisposeHandle,
													&pStruST->struRecvHandle
												);
}

int
st_manager_create_all(
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

	ST_CALLOC(pStruST, ServerTest, 1);
	pStruST->uiDurationTime = uiDurationTime;

	if( !pStruST->struRecvHandle )
	{
		return ST_RECV_NOT_INIT;
	}
	
	iRet = st_create_listener(
												iWaitTime, 
												iListenNum, 
												pStruListenOp, 
												pStruST->struRecvHandle,
												&pStruST->struListenHandle
											);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_create_thread_table(iThreadGroupNum, &pStruST->struThreadHandle);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_create_timer(
										iThreadNum, 
										iTimerNum, 
										pStruST->struThreadHandle, 
										&pStruST->struTimerHandle 
									);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	(*pStruHandle) = (STHandle)pStruST;

	return ST_OK;
}

int
st_destroy_manager(
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_OK;
	}

	st_destroy_listener(pStruST->struListenHandle);

	st_destroy_thread_table(pStruST->struThreadHandle);

	st_destroy_timer(pStruST->struTimerHandle);

	st_destroy_opt_config(pStruST->struOptHandle);

	st_destroy_read_config(pStruST->struRCHandle);

	st_destroy_hub(pStruST->struHubHandle);

	st_destroy_link_handle(pStruST->struCLHandle);

	st_destroy_recv_check(pStruST->struRecvCheckHandle);

	return ST_OK;
}

