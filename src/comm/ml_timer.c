#include "ml_comm.h"
#include "ml_timer.h"
#include "ml_manage.h"
#include "list.h"

typedef struct _MLTimerHead
{
	int iTick;
	int iMSecond;
	int iNum;						//用于记录有多少用户子使用该定时器
	int iTimerFlag;   //定时器状态,是持续存在还是,只是一致存在
	int iTimerStatus; //是否正在进行处理
	unsigned long ulData; //私有数据
	void *pData;
	int (*func)(unsigned long);
	void (*free)(unsigned long);
	pthread_mutex_t struMutex;
}MLTimerHead, *PMLTimerHead;

typedef struct _MLTimer
{
	int					iTimerCnt;
	int					iTimerTick;
	int					iTimerCountThreadID;
	int					iTimerThreadID;
	int					iTimerArrayNum;
	void			  *pData;							//管理结构
	pthread_mutex_t struCntMutex;
	pthread_mutex_t struTickMutex;
	MLTimerHead *pStruTimerArray;
}MLTimer, *PMLTimer;

enum
{
	ML_TIMER_IDLE,
	ML_TIMER_READY,
	ML_TIMER_RUNNING,
	ML_TIMER_MAX
};

#define MAX_COUNT  10

static void
__ml_del_timer( MLTimerHead *pStruTH );

static int
ml_check_timer(
	MLTimer *pStruTimer
)
{
	int i = 0;
	int iCnt = 0;
	int iNum = 0, iCurCnt = 0;
	MLCommNode *pStruHN = NULL;

	pthread_mutex_lock(&pStruTimer->struCntMutex);
	iCurCnt = pStruTimer->iTimerCnt;
	iNum    = pStruTimer->iTimerArrayNum;
	pthread_mutex_unlock(&pStruTimer->struCntMutex);

	for( i = 0; i < iNum && iCnt < iCurCnt; i++ )
	{
		pthread_mutex_lock(&pStruTimer->pStruTimerArray[i].struMutex);
		if( pStruTimer->pStruTimerArray[i].iTimerStatus == ML_TIMER_IDLE )
		{
			goto next;
		}
		iCnt++;
		pStruTimer->pStruTimerArray[i].iTick++;
		if( pStruTimer->pStruTimerArray[i].iTick == pStruTimer->pStruTimerArray[i].iMSecond )
		{
			pStruTimer->pStruTimerArray[i].iTick = 0;
			ML_CALLOC(pStruHN, MLCommNode, 1);	
			pStruHN->ulData = i;
			ml_add_table_thread_pool_node(
																	pStruTimer->iTimerThreadID, 
																	&pStruHN->struNode,
																	(MLThreadHandle)pStruTimer->pData
																);
		}
next:
		pthread_mutex_unlock(&pStruTimer->pStruTimerArray[i].struMutex);
	}
}

static void
ml_decrease_count(
	MLTimer *pStruTimer 
)
{
	pthread_mutex_lock(&pStruTimer->struCntMutex);
	pStruTimer->iTimerCnt--;
	pthread_mutex_unlock(&pStruTimer->struCntMutex);
}

static void
ml_increase_count(
	MLTimer *pStruTimer
)
{
	pthread_mutex_lock(&pStruTimer->struTickMutex);
	pStruTimer->iTimerTick++;
	pthread_mutex_unlock(&pStruTimer->struTickMutex);
}

static int
ml_timer_count(
	struct list_head *pStruNode
)
{
	int iRet = 0;
	int iCount = 0;
	struct timespec struStart, struEnd;
	MLCommNode *pStruCN = NULL;
	MLTimer *pStruTimer = NULL;

	pStruCN = list_entry(pStruNode, MLCommNode, struNode);
	pStruTimer = (MLTimer *)pStruCN->ulData;
	free(pStruCN);
	pStruCN = NULL;

	memset(&struStart, 0, sizeof(struStart));
	memset(&struEnd, 0, sizeof(struEnd));
	while(1)
	{
		iRet = ml_check_exit();
		if( iRet == ML_OK )
		{
			pthread_exit(NULL);;
		}
		sleep(1);
		iRet = ml_check_exit();
		if( iRet == ML_OK )
		{
			pthread_exit(NULL);;
		}
		ml_increase_count(pStruTimer);
		ml_check_timer(pStruTimer);
	}
}

static void
ml_start_timer_count(
	MLTimer *pStruTimer 
)
{
	MLCommNode *pStruCN = NULL;

	ML_CALLOC(pStruCN, MLCommNode, 1);
	pStruCN->ulData = (unsigned long)pStruTimer;

	ml_add_table_thread_pool_node(
													pStruTimer->iTimerCountThreadID,
													&pStruCN->struNode,
													(MLThreadHandle)pStruTimer->pData
													);
}

static int
ml_handle_timer(
	struct list_head *pStruNode
)
{
	int iRet = 0;
	MLTimer *pStruTimer = NULL;
	MLCommNode *pStruCN = NULL;
	MLTimerHead *pStruTH = NULL;

	pStruCN = list_entry(pStruNode, MLCommNode, struNode);

	pStruTH = (MLTimerHead *)pStruCN->ulData;

	if( pStruTH->func )
	{
		pStruTH->func(pStruTH->ulData);
	}
	if( pStruTH->iTimerFlag != ML_TIMER_MLATUS_CONMLANT )
	{
		__ml_del_timer(pStruTH);
		ml_decrease_count((MLTimer *)pStruTH->pData);
	}

	free(pStruCN);
	pStruCN = NULL;

	return ML_OK;
}

int
ml_create_timer(
	int						 iThreadNum,
	int						 iTimerNum,
	MLThreadHandle struThreadHandle,
	MLTimerHandle  *pStruHandle
)
{
	int i = 0;
	int iRet = 0;
	MLTimer *pStruTimer = NULL;

	ML_CALLOC(pStruTimer, MLTimer, 1);
	pStruTimer->iTimerArrayNum = iTimerNum;
	pStruTimer->pData = (void *)struThreadHandle;
	pthread_mutex_init(&pStruTimer->struCntMutex, NULL);
	pthread_mutex_init(&pStruTimer->struTickMutex, NULL);
	ML_CALLOC(pStruTimer->pStruTimerArray, MLTimerHead, iTimerNum);
	for( i = 0; i < iTimerNum; i++ )
	{
		pStruTimer->pStruTimerArray[i].iMSecond = -1;
		pStruTimer->pStruTimerArray[i].pData    = (void*)pStruTimer;
		pthread_mutex_init(&pStruTimer->pStruTimerArray[i].struMutex, NULL);
	}

	iRet = ml_create_thread_table_pool(
																1, 
																4*1024, 
																NULL,
																NULL,
																ml_timer_count, 
																struThreadHandle, 
																&pStruTimer->iTimerCountThreadID 
															);
	if( iRet != ML_OK )
	{
		return iRet;
	}
	iRet = ml_create_thread_table_pool(
																				iThreadNum,
																				0,
																				NULL,
																				NULL,
																				ml_handle_timer,
																				struThreadHandle,
																				&pStruTimer->iTimerThreadID
																			);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	ml_start_timer_count(pStruTimer);

	(*pStruHandle) = (MLTimerHandle)pStruTimer;

	return iRet;
}

static void
ml_destroy_timer_head(
	int					iTimerNum,
	MLTimerHead *pStruTH
)
{
	int i = 0;

	for( ; i < iTimerNum; i++ )	
	{
		pthread_mutex_destroy(&pStruTH[i].struMutex);
	}
}

int
ml_destroy_timer(
	MLTimerHandle struHandle
)
{
	MLTimer *pStruTimer = (MLTimer*)struHandle;
	if( !struHandle )
	{
		return ML_OK;
	}
	
	ml_destroy_timer_head(pStruTimer->iTimerArrayNum, pStruTimer->pStruTimerArray);
	free(pStruTimer->pStruTimerArray);
	pStruTimer->pStruTimerArray = NULL;

	pthread_mutex_destroy(&pStruTimer->struCntMutex);
	pthread_mutex_destroy(&pStruTimer->struTickMutex);

	return ML_OK;
}

void
ml_get_tick(
	MLTimerHandle struHandle,
	int *piTick
)
{
	MLTimer *pStruTimer = (MLTimer *)struHandle;

	if( !struHandle )
	{
		return;
	}
	pthread_mutex_lock(&pStruTimer->struTickMutex);
	(*piTick) = pStruTimer->iTimerTick;
	pthread_mutex_unlock(&pStruTimer->struTickMutex);
}

int
ml_add_timer(
	int iNum,
	int iMSecond,
	int iFlag,
	unsigned long ulData,
	int (*func)(unsigned long),
	void (*free_data)(unsigned long),
	MLTimerHandle struHandle,
	int *piTimerID
)
{
	int i = 0;
	MLTimer *pStruTimer = (MLTimer *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	for( ; i < pStruTimer->iTimerArrayNum; i++ )
	{
		if( pStruTimer->pStruTimerArray[i].iMSecond == -1 )
		{
			//ML_DEBUG("--------------------------------------- i = %d\n", i);
			pthread_mutex_lock(&pStruTimer->pStruTimerArray[i].struMutex);
			pStruTimer->pStruTimerArray[i].iMSecond = iMSecond;
			pStruTimer->pStruTimerArray[i].iNum = iNum;
			pStruTimer->pStruTimerArray[i].func = func;
			pStruTimer->pStruTimerArray[i].iTick = 0;
			pStruTimer->pStruTimerArray[i].ulData = ulData;
			pStruTimer->pStruTimerArray[i].free = free_data;
			pStruTimer->pStruTimerArray[i].iTimerFlag = iFlag;
			pStruTimer->pStruTimerArray[i].iTimerStatus = ML_TIMER_READY;
			if(piTimerID)
			{
				(*piTimerID) = i;
			}
			pthread_mutex_unlock(&pStruTimer->pStruTimerArray[i].struMutex);
			pthread_mutex_lock(&pStruTimer->struCntMutex);
			pStruTimer->iTimerCnt++;
			ML_DEBUG("--------------------------------------- giTimerCnt = %d\n", giTimerCnt);
			pthread_mutex_unlock(&pStruTimer->struCntMutex);
			return ML_OK;
		}
	}

	return ML_ERR;
}

static void
__ml_del_timer(
	MLTimerHead *pStruTH
)
{
	pthread_mutex_lock(&pStruTH->struMutex);
	pStruTH->iTimerStatus = ML_TIMER_IDLE;
	pStruTH->iMSecond = -1;
	if( pStruTH->free )
	{
		pStruTH->free(pStruTH->ulData);
	}
	pthread_mutex_unlock(&pStruTH->struMutex);
}

int
ml_del_timer(
	int iTimerID,
	MLTimerHandle struHandle
)
{	
	MLTimer *pStruTimer = (MLTimer*)struHandle;

	if( !struHandle )
	{
		return ML_OK;
	}
	if( iTimerID >= pStruTimer->iTimerArrayNum || iTimerID < 0 )
	{
		return ML_OK;
	}

	__ml_del_timer(&pStruTimer->pStruTimerArray[iTimerID]);

	ml_decrease_count(pStruTimer);

	return ML_OK;
}
