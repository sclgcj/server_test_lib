#include "server_test_comm.h"
#include "server_test_manage.h"
#include "server_test_listen.h"

int main(
	int  iArgc,
	char **ppArgv
)
{
	int iRet = 0;
	STHandle struHandle;	
	STListenOp struListenOper;

	iRet = server_test_set_listen_op( &struListenOper );
	if( iRet != SERVER_TEST_OK )
	{
		return iRet;
	}

	server_test_create_manager(10, &struHandle);

	iRet = server_test_create_manager_listener(
																				1000,
																				0,
																				&struListenOper,
																				struHandle
																			);
	if( iRet != SERVER_TEST_OK )
	{
		return iRet;
	}

	iRet = server_test_create_manager_thread(0, struHandle);
	if( iRet != SERVER_TEST_OK )
	{
		return iRet;
	}

	iRet = server_test_create_manager_timer(0, 0, struHandle);
	if( iRet != SERVER_TEST_OK )
	{
		return iRet;
	}

	ST_ERROR("\n");
	return server_test_start_manage_listener(struHandle);
}

