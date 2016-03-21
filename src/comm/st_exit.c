#include "st_comm.h"
#include "st_exit.h"
#include <pthread.h>

static int giExit = 0;
static pthread_mutex_t gExitMutex = PTHREAD_MUTEX_INITIALIZER;

int
st_init_exit()
{
	return ST_OK;
}

int
st_uninit_exit()
{
	pthread_mutex_destroy(&gExitMutex);

	return ST_OK;
}

int
st_set_exit()
{
	pthread_mutex_lock(&gExitMutex);
	giExit = 1;
	pthread_mutex_unlock(&gExitMutex);

	return ST_OK;
}

int
st_check_exit()
{
	int iRet = ST_ERR;
	pthread_mutex_lock(&gExitMutex);
	if( giExit == 1 )
	{
		iRet = ST_OK;
	}
	pthread_mutex_unlock(&gExitMutex);

	return iRet;
}

