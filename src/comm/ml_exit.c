#include "ml_comm.h"
#include "ml_exit.h"
#include <pthread.h>

static int giExit = 0;
static pthread_mutex_t gExitMutex = PTHREAD_MUTEX_INITIALIZER;

int
ml_init_exit()
{
	return ML_OK;
}

int
ml_uninit_exit()
{
	pthread_mutex_destroy(&gExitMutex);

	return ML_OK;
}

int
ml_set_exit()
{
	pthread_mutex_lock(&gExitMutex);
	giExit = 1;
	pthread_mutex_unlock(&gExitMutex);

	return ML_OK;
}

int
ml_check_exit()
{
	int iRet = ML_ERR;
	pthread_mutex_lock(&gExitMutex);
	if( giExit == 1 )
	{
		iRet = ML_OK;
	}
	pthread_mutex_unlock(&gExitMutex);

	return iRet;
}

