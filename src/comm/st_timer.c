#include "st_comm.h"
#include "st_timer.h"
#include "st_manage.h"
#include "list.h"

typedef struct _STTimerHead
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
}STTimerHead, *PSTTimerHead;

typedef struct _STTimer
{
	int					iTimerCnt;
	int					iTimerTick;
	int					iTimerCountThreadID;
	int					iTimerThreadID;
	int					iTimerArrayNum;
	void			  *pData;							//管理结构
	pthread_mutex_t struCntMutex;
	pthread_mutex_t struTickMutex;
	STTimerHead *pStruTimerArray;
}STTimer, *PSTTimer;

enum
{
	ST_TIMER_IDLE,
	ST_TIMER_READY,
	ST_TIMER_RUNNING,
	ST_TIMER_MAX
};

#define MAX_COUNT  10

static void
__st_del_timer( STTimerHead *pStruTH );

static int
st_check_timer(
	STTimer *pStruTimer
)
{
	int i = 0;
	int iCnt = 0;
	int iNum = 0, iCurCnt = 0;
	STCommNode *pStruHN = NULL;

	pthread_mutex_lock(&pStruTimer->struCntMutex);
	iCurCnt = pStruTimer->iTimerCnt;
	iNum    = pStruTimer->iTimerArrayNum;
	pthread_mutex_unlock(&pStruTimer->struCntMutex);

	for( i = 0; i < iNum && iCnt < iCurCnt; i++ )
	{
		pthread_mutex_lock(&pStruTimer->pStruTimerArray[i].struMutex);
		if( pStruTimer->pStruTimerArray[i].iTimerStatus == ST_TIMER_IDLE )
		{
			goto next;
		}
		iCnt++;
		pStruTimer->pStruTimerArray[i].iTick++;
		if( pStruTimer->pStruTimerArray[i].iTick == pStruTimer->pStruTimerArray[i].iMSecond )
		{
			pStruTimer->pStruTimerArray[i].iTick = 0;
			ST_CALLOC(pStruHN, STCommNode, 1);	
			pStruHN->ulData = i;
			st_add_table_thread_pool_node(
																	pStruTimer->iTimerThreadID, 
																	&pStruHN->struNode,
																	(STThreadHandle)pStruTimer->pData
																);
		}
next:
		pthread_mutex_unlock(&pStruTimer->pStruTimerArray[i].struMutex);
	}
}

static void
st_decrease_count(
	STTimer *pStruTimer 
)
{
	pthread_mutex_lock(&pStruTimer->struCntMutex);
	pStruTimer->iTimerCnt--;
	pthread_mutex_unlock(&pStruTimer->struCntMutex);
}

static void
st_increase_count(
	STTimer *pStruTimer
)
{
	pthread_mutex_lock(&pStruTimer->struTickMutex);
	pStruTimer->iTimerTick++;
	pthread_mutex_unlock(&pStruTimer->struTickMutex);
}

static int
st_timer_count(
	struct list_head *pStruNode
)
{
	int iRet = 0;
	int iCount = 0;
	struct timespec struStart, struEnd;
	STCommNode *pStruCN = NULL;
	STTimer *pStruTimer = NULL;

	pStruCN = list_entry(pStruNode, STCommNode, struNode);
	pStruTimer = (STTimer *)pStruCN->ulData;
	free(pStruCN);
	pStruCN = NULL;

	memset(&struStart, 0, sizeof(struStart));
	memset(&struEnd, 0, sizeof(struEnd));
	while(1)
	{
		iRet = st_check_exit();
		if( iRet == ST_OK )
		{
			pthread_exit(NULL);;
		}
		sleep(1);
		iRet = st_check_exit();
		if( iRet == ST_OK )
		{
			pthread_exit(NULL);;
		}
		st_increase_count(pStruTimer);
		st_check_timer(pStruTimer);
	}
}

static void
st_start_timer_count(
	STTimer *pStruTimer 
)
{
	STCommNode *pStruCN = NULL;

	ST_CALLOC(pStruCN, STCommNode, 1);
	pStruCN->ulData = (unsigned long)pStruTimer;

	st_add_table_thread_pool_node(
													pStruTimer->iTimerCountThreadID,
													&pStruCN->struNode,
													(STThreadHandle)pStruTimer->pData
													);
}

static int
st_handle_timer(
	struct list_head *pStruNode
)
{
	int iRet = 0;
	STTimer *pStruTimer = NULL;
	STCommNode *pStruCN = NULL;
	STTimerHead *pStruTH = NULL;

	pStruCN = list_entry(pStruNode, STCommNode, struNode);

	pStruTH = (STTimerHead *)pStruCN->ulData;

	if( pStruTH->func )
	{
		pStruTH->func(pStruTH->ulData);
	}
	if( pStruTH->iTimerFlag != ST_TIMER_STATUS_CONSTANT )
	{
		__st_del_timer(pStruTH);
		st_decrease_count((STTimer *)pStruTH->pData);
	}

	free(pStruCN);
	pStruCN = NULL;

	return ST_OK;
}

int
st_create_timer(
	int						 iThreadNum,
	int						 iTimerNum,
	STThreadHandle struThreadHandle,
	STTimerHandle  *pStruHandle
)
{
	int i = 0;
	int iRet = 0;
	STTimer *pStruTimer = NULL;

	ST_CALLOC(pStruTimer, STTimer, 1);
	pStruTimer->iTimerArrayNum = iTimerNum;
	pStruTimer->pData = (void *)struThreadHandle;
	pthread_mutex_init(&pStruTimer->struCntMutex, NULL);
	pthread_mutex_init(&pStruTimer->struTickMutex, NULL);
	ST_CALLOC(pStruTimer->pStruTimerArray, STTimerHead, iTimerNum);
	for( i = 0; i < iTimerNum; i++ )
	{
		pStruTimer->pStruTimerArray[i].iMSecond = -1;
		pStruTimer->pStruTimerArray[i].pData    = (void*)pStruTimer;
		pthread_mutex_init(&pStruTimer->pStruTimerArray[i].struMutex, NULL);
	}

	iRet = st_create_thread_table_pool(
																1, 
																4*1024, 
																NULL,
																NULL,
																st_timer_count, 
																struThreadHandle, 
																&pStruTimer->iTimerCountThreadID 
															);
	if( iRet != ST_OK )
	{
		return iRet;
	}
	iRet = st_create_thread_table_pool(
																				iThreadNum,
																				0,
																				NULL,
																				NULL,
																				st_handle_timer,
																				struThreadHandle,
																				&pStruTimer->iTimerThreadID
																			);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	st_start_timer_count(pStruTimer);

	(*pStruHandle) = (STTimerHandle)pStruTimer;

	return iRet;
}

static void
st_destroy_timer_head(
	int					iTimerNum,
	STTimerHead *pStruTH
)
{
	int i = 0;

	for( ; i < iTimerNum; i++ )	
	{
		pthread_mutex_destroy(&pStruTH[i].struMutex);
	}
}

int
st_destroy_timer(
	STTimerHandle struHandle
)
{
	STTimer *pStruTimer = (STTimer*)struHandle;
	if( !struHandle )
	{
		return ST_OK;
	}
	
	st_destroy_timer_head(pStruTimer->iTimerArrayNum, pStruTimer->pStruTimerArray);
	free(pStruTimer->pStruTimerArray);
	pStruTimer->pStruTimerArray = NULL;

	pthread_mutex_destroy(&pStruTimer->struCntMutex);
	pthread_mutex_destroy(&pStruTimer->struTickMutex);

	return ST_OK;
}

void
st_get_tick(
	STTimerHandle struHandle,
	int *piTick
)
{
	STTimer *pStruTimer = (STTimer *)struHandle;

	if( !struHandle )
	{
		return;
	}
	pthread_mutex_lock(&pStruTimer->struTickMutex);
	(*piTick) = pStruTimer->iTimerTick;
	pthread_mutex_unlock(&pStruTimer->struTickMutex);
}

int
st_add_timer(
	int iNum,
	int iMSecond,
	int iFlag,
	unsigned long ulData,
	int (*func)(unsigned long),
	void (*free_data)(unsigned long),
	STTimerHandle struHandle,
	int *piTimerID
)
{
	int i = 0;
	STTimer *pStruTimer = (STTimer *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	for( ; i < pStruTimer->iTimerArrayNum; i++ )
	{
		if( pStruTimer->pStruTimerArray[i].iMSecond == -1 )
		{
			//ST_DEBUG("--------------------------------------- i = %d\n", i);
			pthread_mutex_lock(&pStruTimer->pStruTimerArray[i].struMutex);
			pStruTimer->pStruTimerArray[i].iMSecond = iMSecond;
			pStruTimer->pStruTimerArray[i].iNum = iNum;
			pStruTimer->pStruTimerArray[i].func = func;
			pStruTimer->pStruTimerArray[i].iTick = 0;
			pStruTimer->pStruTimerArray[i].ulData = ulData;
			pStruTimer->pStruTimerArray[i].free = free_data;
			pStruTimer->pStruTimerArray[i].iTimerFlag = iFlag;
			pStruTimer->pStruTimerArray[i].iTimerStatus = ST_TIMER_READY;
			if(piTimerID)
			{
				(*piTimerID) = i;
			}
			pthread_mutex_unlock(&pStruTimer->pStruTimerArray[i].struMutex);
			pthread_mutex_lock(&pStruTimer->struCntMutex);
			pStruTimer->iTimerCnt++;
			ST_DEBUG("--------------------------------------- giTimerCnt = %d\n", giTimerCnt);
			pthread_mutex_unlock(&pStruTimer->struCntMutex);
			return ST_OK;
		}
	}

	return ST_ERR;
}

static void
__st_del_timer(
	STTimerHead *pStruTH
)
{
	pthread_mutex_lock(&pStruTH->struMutex);
	pStruTH->iTimerStatus = ST_TIMER_IDLE;
	pStruTH->iMSecond = -1;
	if( pStruTH->free )
	{
		pStruTH->free(pStruTH->ulData);
	}
	pthread_mutex_unlock(&pStruTH->struMutex);
}

int
st_del_timer(
	int iTimerID,
	STTimerHandle struHandle
)
{	
	STTimer *pStruTimer = (STTimer*)struHandle;

	if( !struHandle )
	{
		return ST_OK;
	}
	if( iTimerID >= pStruTimer->iTimerArrayNum || iTimerID < 0 )
	{
		return ST_OK;
	}

	__st_del_timer(&pStruTimer->pStruTimerArray[iTimerID]);

	st_decrease_count(pStruTimer);

	return ST_OK;
}
