#include "ml_comm.h"
#include "ml_manage.h"
#include "ml_listen.h" static char *gsArgFmt = "f:d";
static char *gsFilePath = "./config";

static int
ml_teml_create_link(
	MLHandle struHandle 
)
{
	MLCLParam struParam;		

	memset(&struParam, 0, sizeof(struParam));

	return ml_manager_create_link_handle(NULL, &struParam, NULL, struHandle);
}



int main(
	int  iArgc,
	char **ppArgv
)
{
	int iRet = 0;
	MLHandle struHandle;	
	MLListenOp struListenOper;

	ml_create_manager(10, &struHandle);

	iRet = ml_manager_create_opt_config(iArgc, ppArgv, gsArgFmt, struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_read_config(gsFilePath, struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_set_listen_op( &struListenOper );
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_thread(0, struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_timer(0, 0, struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_hub(0, 0, 0, NULL, NULL, struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_teml_create_link(struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_recv_check(1, 60, 0, NULL,struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_dispose(0, 0, NULL, struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_send(0, 0, NULL, struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_recv(0, 0, NULL, struHandle);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_listener(
																	1000,
																	0,
																	&struListenOper,
																	struHandle
															);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	iRet = ml_manager_create_result(
															1,
															NULL,
															"./result.txt",
															NULL,
															struHandle
														);

	return ml_manage_start(struHandle);
}

