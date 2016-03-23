#include "st_comm.h"
#include "st_manage.h"
#include "st_listen.h"

static char *gsArgFmt = "f:d";
static char *gsFilePath = "./config";

static int
st_test_create_link(
	STHandle struHandle 
)
{
	STCLParam struParam;		

	memset(&struParam, 0, sizeof(struParam));

	return st_manager_create_link_handle(NULL, &struParam, NULL, struHandle);
}

int main(
	int  iArgc,
	char **ppArgv
)
{
	int iRet = 0;
	STHandle struHandle;	
	STListenOp struListenOper;

	st_create_manager(10, &struHandle);


	iRet = st_manager_create_opt_config(iArgc, ppArgv, gsArgFmt, struHandle);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_manager_create_read_config(gsFilePath, struHandle);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_set_listen_op( &struListenOper );
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_manager_create_listener(
																	1000,
																	0,
																	&struListenOper,
																	struHandle
															);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_manager_create_thread(0, struHandle);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_manager_create_timer(0, 0, struHandle);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_manager_create_hub(0, 0, 0, NULL, NULL, struHandle);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	iRet = st_test_create_link(struHandle);

	return st_manage_start_listener(struHandle);
}

