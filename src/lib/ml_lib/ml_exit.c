#include "ml_comm.h"
#include "ml_exit.h"

typedef struct _MLExit
{
	int iExit;
	int iDurationTime;
	pthread_mutex_t struExitMutex;
}MLExit, *PMLExit;

void
ml_create_exit_handle(
	int iDurationTime,
	MLExitHandle *pStruHandle
)
{
	MLExit *pStruE = NULL;

	ML_CALLOC(pStruE, MLExit, 1);
	pStruE->iDurationTime = iDurationTime;
	pthread_mutex_init(&pStruE->struExitMutex, NULL);

	(*pStruHandle) = (MLExitHandle)pStruE;
}

void
ml_destroy_exit_handle(
	MLExitHandle struHandle
)
{
	MLExit *pStruE = (MLExit *)struHandle;

	if( !struHandle )
	{
		return;
	}

	pthread_mutex_destroy(&pStruE->struExitMutex);

	ML_FREE(pStruE);
}

int
ml_check_exit(
	MLExitHandle struHandle
)
{
	int iRet = 0;
	MLExit *pStruE = (MLExit *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	pthread_mutex_lock(&pStruE->struExitMutex);
	iRet = pStruE->iExit;
	pthread_mutex_unlock(&pStruE->struExitMutex);

	if( iRet == 1 )
	{
		return ML_OK;
	}

	return ML_ERR;
}

int
ml_check_exit_duration(
	int					 iTick,
	MLExitHandle struHandle
)
{
	int iRet = ML_ERR;
	MLExit *pStruE = (MLExit *)struHandle;

	if( !pStruE )
	{
		return ML_PARAM_ERR;	
	}

	pthread_mutex_lock(&pStruE->struExitMutex);
	if( pStruE->iExit == 0 )
	{
		if(pStruE->iDurationTime <= iTick )
		{
			pStruE->iExit = 1;
			iRet = ML_OK;
		}
	}
	pthread_mutex_unlock(&pStruE->struExitMutex);

	return iRet;
}


