#include "ml_comm.h"
#include "ml_manage.h"

void
ml_create_manager(
	MLHandle     *pStruHandle
)
{
	ServerTest *pStruML = NULL;

	ML_CALLOC(pStruML, ServerTest, 1);

	(*pStruHandle) = (MLHandle)pStruML;
}

int
ml_manager_create_listener(
	int iWaitTime,
	int iListenNum,
	MLListenOp *pStruListeOp,
	MLHandle   struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !pStruML )
	{
		return ML_PARAM_ERR;
	}

	if( !pStruML->struRecvHandle )
	{
		return ML_RECV_NOT_INIT;
	}
	if( !pStruML->struSendHandle )
	{
		return ML_SEND_NOT_INIT;
	}
	
	return ml_create_listener( 
														iWaitTime,
														iListenNum, 
														pStruListeOp, 
														pStruML->struRecvHandle,
														pStruML->struSendHandle,
														&pStruML->struListenHandle
													);
}

int
ml_manager_create_thread(
	int iThreadGroupNum,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !pStruML )
	{
		return ML_PARAM_ERR;
	}
	if( !pStruML->struExitHandle )
	{
		return ML_EXIT_NOT_INIT;
	}

	if( iThreadGroupNum <= 0 )
	{
		iThreadGroupNum = ML_DEFAULT_GROUP_NUM;
	}

	return ml_create_thread_table(
															iThreadGroupNum,
															pStruML->struExitHandle,
															&pStruML->struThreadHandle
														);
}

int
ml_manager_create_timer(
	int iTimerNum,
	int iThreadNum,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !pStruML )
	{
		return ML_PARAM_ERR;
	}
	if( !pStruML->struThreadHandle )
	{
		return ML_THREAD_NOT_INIT;
	}
	if( !pStruML->struExitHandle )
	{
		return ML_EXIT_NOT_INIT;
	}

	if( iTimerNum <= 0 )
	{
		iTimerNum = ML_DEFAULT_TIMER_NUM;
	}
	if( iThreadNum <= 0 )
	{
		iThreadNum = ML_DEFAULT_THREAD_NUM;
	}

	return ml_create_timer(
										iThreadNum,
										iTimerNum,
										pStruML->struExitHandle,
										pStruML->struThreadHandle,
										&pStruML->struTimerHandle
									);
}

int
ml_manager_create_hub(
	int iHubNum,
	int iThreadNum,
	int iStackSize,
	MLHubFunc pHubFunc,
	MLHubHandleFunc pHubHandleFunc,
	MLHandle        struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	if( !pStruML->struTimerHandle )
	{
		return ML_TIMER_NOT_INIT;
	}
	if( !pStruML->struThreadHandle )
	{
		return ML_THREAD_NOT_INIT;
	}

	if( iHubNum <= 0 )
	{
		iHubNum = ML_DEFAULT_HUB_TABLE_SIZE;
	}
	if( iStackSize <= 0 )
	{
		iStackSize = ML_DEFAULT_THREAD_MLACK_SIZE;;
	}
	if( iThreadNum )
	{
		iThreadNum = ML_DEFAULT_THREAD_NUM;
	}

	return ml_create_hub(
									iHubNum,
									iThreadNum,
									iStackSize,
									pHubFunc,
									pHubHandleFunc,
									pStruML->struTimerHandle,
									pStruML->struThreadHandle,
									&pStruML->struHubHandle
								);
}

int
ml_manager_create_opt_config(
	int  iArgc,
	char **ssArgv,
	char *sParseFmt,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	ml_create_opt_config(iArgc, ssArgv, sParseFmt, &pStruML->struOptHandle);
}

int
ml_manager_create_read_config(
	char *sFile,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_create_read_config(sFile, &pStruML->struRCHandle);
}

int
ml_manager_create_link_handle(
	void              *pUserData,
	MLCLParam         *pStruCLParam,
	MLCreateLinkFunc  pCLFunc,
	MLHandle					struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	if( !struHandle || !pStruCLParam )
	{
		return ML_PARAM_ERR;
	}
	if( !pStruML->struThreadHandle )
	{
		return ML_THREAD_NOT_INIT;
	}
	if( !pStruML->struExitHandle )
	{
		return ML_EXIT_NOT_INIT;
	}

	if( pStruCLParam->iThreadNum <= 0 )
	{
		pStruCLParam->iThreadNum = ML_DEFAULT_THREAD_NUM;	
	}
	if( pStruCLParam->iStackSize <= 0 )
	{
		pStruCLParam->iStackSize = ML_DEFAULT_THREAD_MLACK_SIZE;
	}

	return ml_create_link_handle(
														pUserData,
														pStruCLParam,
														pCLFunc,
														pStruML->struExitHandle,
														pStruML->struThreadHandle,
														&pStruML->struCLHandle
													);
}

int
ml_manager_create_recv_check(
	int  iTotalLink,
	int  iRecvTimerout,
	int  iCheckListNum,
	MLRecvCheckFailFunc pFunc,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	if( iCheckListNum <= 0 )
	{
		iCheckListNum = ML_DEFAULT_CHECK_SIZE;
	}

	return ml_create_recv_check(
												iTotalLink,
												iRecvTimerout,
												iCheckListNum,
												pFunc,
												pStruML->struTimerHandle,
												&pStruML->struRecvCheckHandle
											);
}

int
ml_manager_create_recv(
	int iThreadNum,
	int iStackSize,
	MLRecvFunc pFunc,
	MLHandle   struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	if( !pStruML->struThreadHandle )
	{
		return ML_THREAD_NOT_INIT;
	}
	if( !pStruML->struDisposeHandle )
	{
		return ML_DISPOSE_NOT_INIT;
	}

	if( iThreadNum <= 0 )
	{
		iThreadNum = ML_DEFAULT_THREAD_NUM;
	}
	if( iStackSize <= 0 )
	{
		iStackSize = ML_DEFAULT_THREAD_MLACK_SIZE;
	}

	return ml_create_recv_handle(
													iThreadNum,
													iStackSize,
													pFunc,
													pStruML->struThreadHandle,
													pStruML->struDisposeHandle,
													&pStruML->struRecvHandle
												);
}

int
ml_manager_create_dispose(
	int iThreadNum,
	int iStackSize,
	MLDisposeFunc pFunc,
	MLHandle       struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	if( !pStruML->struThreadHandle )
	{
		return ML_THREAD_NOT_INIT;
	}

	if( iThreadNum <= 0 )
	{
		iThreadNum = ML_DEFAULT_THREAD_NUM;
	}
	if( iStackSize <= 0 )
	{
		iStackSize = ML_DEFAULT_THREAD_MLACK_SIZE;
	}

	ml_create_dispose_handle(
	 										iThreadNum,
	 										iStackSize,
	 										pFunc,
	 										pStruML->struThreadHandle,
	 										&pStruML->struDisposeHandle
	 								);
	return ML_OK;
}

int
ml_manager_create_send(
	int iThreadNum,
	int iStackSize,
	MLSendFunc pFunc,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	if( !pStruML->struThreadHandle )
	{
		return ML_THREAD_NOT_INIT;
	}
	
	if( iThreadNum <= 0 )
	{
		iThreadNum = ML_DEFAULT_THREAD_NUM;
	}
	if( iStackSize <= 0 )
	{
		iStackSize = ML_DEFAULT_THREAD_MLACK_SIZE;
	}

	ml_create_send_handle(
										iThreadNum,
										iStackSize,
										pFunc,
										pStruML->struThreadHandle,
										&pStruML->struSendHandle
									);

	return ML_OK;
}

int
ml_manager_create_result(
	int iTotalLink,
	void *pUserData,
	char *sResultName,
	MLResultFunc pFunc,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !pStruML )
	{
		return ML_PARAM_ERR;
	}

	return ml_create_result_handle(
															iTotalLink,
															pUserData,
															sResultName,
															pFunc,
															&pStruML->struResultHandle
													);
}

int
ml_manager_create_exit(
	int      iDurationTime,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !pStruML )
	{
		return ML_PARAM_ERR;
	}

	if( iDurationTime <= 0 )
	{
		iDurationTime = ML_DEFAULT_DURATION_TIME;
	}

	ml_create_exit_handle(
											iDurationTime,															
											&pStruML->struExitHandle
										);
	return ML_OK;
}

int
ml_manager_create_client_data(
	int iClientNum,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	if( !pStruML )
	{
		return ML_PARAM_ERR;
	}

	if( iClientNum <= 0 )
	{
		iClientNum = ML_DEFAULT_CLIENT_NUM;
	}

	ml_create_client_data(iClientNum, &pStruML->struDataHandle);

	return ML_OK;
}

int
ml_manager_create_data(
	int iMLDataNum,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	if( iMLDataNum )
	{
		iMLDataNum = ML_DEFAULT_MLDATA_NUM;
	}

	ml_create_data_handle(iMLDataNum, &pStruML->struDataHandle);

	return ML_OK;
}

int
ml_manager_create_all(
	int iTotalLink,
	MLCLParam *pStruParam,
	MLHandle *pStruHandle
)
{
	int iRet = 0;
	ServerTest *pStruML = NULL;	

	ML_CALLOC(pStruML, ServerTest, 1);
	(*pStruHandle) = (MLHandle)pStruML;

	ml_manager_create_opt_config(0, NULL, NULL, *pStruHandle);
	ml_manager_create_read_config(NULL, *pStruHandle);
	ml_manager_create_thread(0, *pStruHandle);
	ml_manager_create_timer(0, 0, *pStruHandle);
	ml_manager_create_hub(0, 0, 0, NULL, NULL, *pStruHandle);
	ml_manager_create_link_handle(NULL, pStruParam, NULL, *pStruHandle);
	ml_manager_create_recv_check(iTotalLink, 60, 0, NULL,*pStruHandle);
	ml_manager_create_dispose(0, 0, NULL, *pStruHandle);
	ml_manager_create_send(0, 0, NULL, *pStruHandle);
	ml_manager_create_recv(0, 0, NULL, *pStruHandle);
	ml_manager_create_result(
										iTotalLink, 
										NULL, 
										"./result.txt", 
										NULL, 
										*pStruHandle 
									);
	ml_manager_create_listener(
														1000,
														0,
														NULL,
														*pStruHandle
												);

	return ML_OK;
}

int
ml_destroy_manager(
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_OK;
	}
	sleep(3);

	ml_destroy_listener(pStruML->struListenHandle);

	ml_destroy_thread_table(pStruML->struThreadHandle);

	ml_destroy_timer(pStruML->struTimerHandle);

	ml_destroy_opt_config(pStruML->struOptHandle);

	ml_destroy_read_config(pStruML->struRCHandle);

	ml_destroy_hub(pStruML->struHubHandle);

	ml_destroy_link_handle(pStruML->struCLHandle);

	ml_destroy_recv_check(pStruML->struRecvCheckHandle);

	ml_destroy_exit_handle(pStruML->struExitHandle);

	ML_FREE(struHandle);

	return ML_OK;
}

int
ml_manage_start(
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	
	ml_manager_start_create_link(struHandle);

	ml_manager_start_listener(struHandle);

	ml_destroy_manager(struHandle);

	ml_handle_result(pStruML->struResultHandle);

	return ML_OK;
}

