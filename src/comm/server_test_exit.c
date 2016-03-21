#include "server_test_comm.h"
#include "server_test_exit.h"
#include <pthread.h>

static int giExit = 0;
static pthread_mutex_t gExitMutex = PTHREAD_MUTEX_INITIALIZER;

int
server_test_init_exit()
{
	return SERVER_TEST_OK;
}

int
server_test_uninit_exit()
{
	pthread_mutex_destroy(&gExitMutex);

	return SERVER_TEST_OK;
}

int
server_test_set_exit()
{
	pthread_mutex_lock(&gExitMutex);
	giExit = 1;
	pthread_mutex_unlock(&gExitMutex);

	return SERVER_TEST_OK;
}

int
server_test_check_exit()
{
	int iRet = SERVER_TEST_ERR;
	pthread_mutex_lock(&gExitMutex);
	if( giExit == 1 )
	{
		iRet = SERVER_TEST_OK;
	}
	pthread_mutex_unlock(&gExitMutex);

	return iRet;
}

