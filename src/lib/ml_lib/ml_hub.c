#include <time.h>

#include "ml_hub.h"

#define ML_TABLE_SIZE 150000

#define ML_HUB_MLATUS_IDLE    0
#define ML_HUB_MLATUS_RUNNING 1

//typedef void (*MLHubHandleFunc)(char *sRecvData, int )

typedef struct MLHubTable
{
	int iSendTimes;
	int iExpireTime;
	int iStatus;
	struct list_head struHead;
}MLHubTable, *PMLHubTable;

typedef struct _MLHub
{
	int iTimerID;
	int iThreadID;
	int iListTableNum;
	int iMinInterval;
	unsigned int uiTimes;
	unsigned int uiCount;
	unsigned int uiOffset;
	unsigned int uiMaxInterval;
	unsigned int uiMinInterval;
	unsigned int uiHubIntervalTime;
	unsigned int uiHubIntervalOffset;
	MLHubTable *pStruListTable;	
	MLTimerHandle struTimerHandle;
	MLThreadHandle struThreadHandle;
	pthread_mutex_t struMaxIntervalMutex;
	pthread_mutex_t *pStruListTableMutex;
	MLHubFunc pHubFunc;
}MLHub, *PMLHub;

typedef struct _MLHubListData
{
	int   iExpireTime;
	MLHub *pStruHub;
	struct list_head struHead;
	struct list_head struNode;
}MLHubListData, *PMLHubListData;



static void ml_free_hub_list(struct list_head *pStruHead);
static int ml_send_hub_list(struct list_head *pStruHead);
static int ml_add_send_hub( unsigned long ulData );

int
ml_create_hub(
	int iHubNum,
	int iThreadNum,
	int iStackSize,
	MLHubFunc pHubFunc,
	MLHubHandleFunc pHubHandleFunc,
	MLTimerHandle struTHandle,
	MLThreadHandle struThreadHandle,
	MLHubHandle *pStruHandle
)
{
	int i = 0;
	int iRet = 0;
	MLHub *pStruH = NULL;	


	ML_CALLOC(pStruH, MLHub, 1);
	ML_CALLOC(pStruH->pStruListTable, MLHubTable, iHubNum);
	ML_CALLOC(pStruH->pStruListTableMutex, pthread_mutex_t, 1);
	pthread_mutex_init(&pStruH->struMaxIntervalMutex, NULL);
	//pthread_mutex_init(&pStruH->struCountMutex);
	pStruH->struTimerHandle = struTHandle;
	pStruH->pHubFunc = pHubFunc;
	pStruH->struThreadHandle = struThreadHandle;

	for( ; i < ML_TABLE_SIZE; i++ )
	{
		pStruH->pStruListTable[i].iSendTimes = 0;
		pStruH->pStruListTable[i].iExpireTime = 0;
		INIT_LIST_HEAD(&pStruH->pStruListTable[i].struHead);
		pthread_mutex_init(&pStruH->pStruListTableMutex[i], NULL);
	}

	ml_add_timer(1, 1, ML_TIMER_MLATUS_CONMLANT,  (unsigned long)pStruH, ml_add_send_hub, NULL, struTHandle, &pStruH->iTimerID);

	iRet = ml_create_thread_table_pool(
															iThreadNum, 
															iStackSize,
															ml_free_hub_list, 
															NULL,
															ml_send_hub_list,
															struThreadHandle,
															&pStruH->iThreadID
														);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	(*pStruHandle) = (MLHubHandle)pStruH;

	return iRet;
}

void
ml_destroy_hub(
	MLHubHandle struHandle
)
{
	int i = 0;
	MLHub *pStruH = (MLHub *)struHandle;

	if( !struHandle )
	{
		return;
	}

	for( ; i < pStruH->iListTableNum; i++ )	
	{
		pthread_mutex_lock(&pStruH->pStruListTableMutex[i]);
		if( !list_empty(&pStruH->pStruListTable[i].struHead) )
		{
			ml_free_hub_list(&pStruH->pStruListTable[i].struHead);
		}
		pthread_mutex_unlock(&pStruH->pStruListTableMutex[i]);
	}

	ML_FREE(pStruH);
}

static void
ml_free_hub_list(
		struct list_head *pStruHead
)
{
	struct list_head *pStruSL = pStruHead->next;
	MLCommNode *pStruCN = NULL;

	while( pStruSL != pStruHead )
	{
		pStruCN = list_entry(pStruSL, MLCommNode, struNode);
		list_del_init(&pStruCN->struNode);
		pStruSL = pStruHead->next;
		free(pStruCN);
	}
}

static void
ml_set_hub_interval(
	int iIntervalTime,
	MLHub *pStruH
)
{
	pthread_mutex_lock(&pStruH->struMaxIntervalMutex);
	if( pStruH->uiMinInterval > iIntervalTime )
	{
		pStruH->uiMinInterval = iIntervalTime;
	}
	if( pStruH->uiMaxInterval <= iIntervalTime )
	{
		pStruH->uiMaxInterval = iIntervalTime + 1;
	}
	pStruH->uiOffset = (pStruH->uiMaxInterval - pStruH->uiMinInterval) % pStruH->uiMinInterval;
	pthread_mutex_unlock(&pStruH->struMaxIntervalMutex);
}

static void
ml_add_hub_node(
	int				 iHubID,
	int				 iExpireTime,
	MLCommNode *pStruCN,
	MLHub			 *pStruHub
)
{
	pthread_mutex_lock(&pStruHub->pStruListTableMutex[iHubID]);
	pStruHub->pStruListTable[iHubID].iExpireTime = iExpireTime;
	list_add_tail(&pStruCN->struNode, &pStruHub->pStruListTable[iHubID].struHead);
	pthread_mutex_unlock(&pStruHub->pStruListTableMutex[iHubID]);
}

int
ml_add_hub(
	void *pData,
	unsigned long *pulHubID,
	MLHubHandle struHandle
)
{
	int iOffset = 0;
	int iIntervalTime = 0;
	time_t Curtime = time(NULL);
	MLCommNode *pStruCN = NULL;
	MLHub *pStruH = (MLHub *)struHandle;

	if( pStruH->uiCount == 0 )
	{
		//重置计时器
		//ml_reset_timer();
		pStruH->uiCount = Curtime;
	}
	else
	{
		iOffset = Curtime - pStruH->uiCount;
	}

	ML_DEBUG("time = %d\n", (unsigned int)Curtime);
	ML_CALLOC(pStruCN, MLCommNode, 1);

	iIntervalTime = pStruH->uiHubIntervalTime + iOffset;
	pStruCN->pUserData = pData;
	pStruCN->ulData = iIntervalTime;
	(*pulHubID) = (unsigned long)pStruCN;
	//pStruH->ulData = (unsigned long)pStruNode;	
	//clock_gettime(CLOCK_MONOTONIC, &pStruPC->struCreateTime);

	ML_ERROR("iIntervalTime = %d\n", iIntervalTime);
	ml_set_hub_interval(iIntervalTime, pStruH);
	
	ml_add_hub_node( 
							iIntervalTime , 
							pStruH->uiHubIntervalTime, 
							pStruCN,
							pStruH
						);

	return ML_OK;	
}

static int
ml_send_hub_list(
	struct list_head *pStruNode
)
{
	int iRet = ML_OK;
	int iCount = 0;
	int iExpireTime = 0;
	struct list_head *pStruSL = NULL;
	MLHub *pStruH = NULL;
	MLCommNode *pStruCN = NULL;
	MLHubListData *pStruHLD = NULL;

	if(!pStruNode)
	{
		return ML_OK;
	}
	pStruHLD = list_entry(pStruNode, MLHubListData, struNode);		

	ML_DEBUG("iExpire = %d\n", pStruHLD->iExpireTime);
	pStruH = pStruHLD->pStruHub;
	pStruSL = pStruHLD->struHead.next;
	//本意是包发完之后再送回的,因为这样子加锁的时间会少点,但是发现这么做会导致结果不稳定,所以先弄回去,这让人感觉在分配是的链表转移有点多余
	pthread_mutex_lock(&pStruH->pStruListTableMutex[pStruHLD->iExpireTime]);
	list_splice_init(&pStruHLD->struHead, &pStruH->pStruListTable[pStruHLD->iExpireTime].struHead);
	pthread_mutex_unlock(&pStruH->pStruListTableMutex[pStruHLD->iExpireTime]);
	ML_ERROR("send data \n");
	while( pStruSL != &pStruH->pStruListTable[pStruHLD->iExpireTime].struHead )
	{
		pStruCN = list_entry(pStruSL, MLCommNode, struNode);

		if( pStruH->pHubFunc )
		{
			pStruH->pHubFunc((unsigned long)pStruCN->pUserData);
		}
		//ml_send_thread(pStruHN);

		pthread_mutex_lock(&pStruH->pStruListTableMutex[pStruHLD->iExpireTime]);
		pStruSL = pStruSL->next;
		pthread_mutex_unlock(&pStruH->pStruListTableMutex[pStruHLD->iExpireTime]);
		pStruCN = NULL;
	}
	pStruH->pStruListTable[pStruHLD->iExpireTime].iStatus = ML_HUB_MLATUS_IDLE;
out:
	free(pStruHLD);
	pStruHLD = NULL;

	return iRet;
}

static int 
ml_add_send_hub(
	unsigned long ulData
)
{
	int i = 0;
	int iRet = 0;
	int iCurPos = 0;
	MLHub *pStruH = (MLHub *)ulData;
	MLHubListData *pStruData = NULL;

	ml_get_tick(pStruH->struTimerHandle, &iCurPos);
	if( iCurPos == 0 )
	{
		return ML_OK;
	}
	pthread_mutex_lock(&pStruH->struMaxIntervalMutex);
	pStruH->iMinInterval = pStruH->uiMinInterval;
	pthread_mutex_unlock(&pStruH->struMaxIntervalMutex);
	/*
	 * 这里的循环是为了解决最大值是最小值的的倍数的情况,
	 */
	for( i = pStruH->iMinInterval; i <= iCurPos; i++ )
	{
		if( pStruH->pStruListTable[i].iSendTimes + i > iCurPos || 
				pStruH->pStruListTable[i].iExpireTime == 0  || 
				pStruH->pStruListTable[i].iStatus == ML_HUB_MLATUS_RUNNING)
		{
			continue;
		}
		pStruH->pStruListTable[i].iStatus = ML_HUB_MLATUS_RUNNING;

		ML_ERROR("i = %d, iCurpos = %d, expire = %d\n", i, iCurPos, pStruH->pStruListTable[i].iExpireTime);
		pthread_mutex_lock(&pStruH->pStruListTableMutex[i]);
		if( list_empty(&pStruH->pStruListTable[i].struHead) )
		{
			goto next;
		}
		pthread_mutex_unlock(&pStruH->pStruListTableMutex[i]);
		ML_CALLOC(pStruData, MLHubListData, 1);

		pStruData->iExpireTime = i;
		pStruData->pStruHub    = pStruH;
		INIT_LIST_HEAD(&pStruData->struHead);
		{
			pStruH->pStruListTable[i].iSendTimes += pStruH->pStruListTable[i].iExpireTime;
			pthread_mutex_lock(&pStruH->pStruListTableMutex[i]);
			list_splice_init(&pStruH->pStruListTable[i].struHead, &pStruData->struHead); 
			pthread_mutex_unlock(&pStruH->pStruListTableMutex[i]);
			ml_add_table_thread_pool_node(pStruH->iThreadID, &pStruData->struNode, pStruH->struThreadHandle);
			continue;
		}
next:
		pthread_mutex_unlock(&pStruH->pStruListTableMutex[i]);
	}

	return iRet;
}

int
ml_del_hub(
	unsigned long ulHubID,
	MLHubHandle struHandle
)
{
	int iExpireTime = 0;
	MLHub *pStruH = (MLHub *)struHandle;
	MLCommNode *pStruCN = (MLCommNode*)ulHubID;
	
	if( !struHandle || !ulHubID )
	{
		return ML_PARAM_ERR;	
	}

	pStruCN = (MLCommNode *)ulHubID;
	iExpireTime = pStruCN->ulData; 

	pthread_mutex_lock(&pStruH->pStruListTableMutex[iExpireTime]);
	list_del_init(&pStruCN->struNode);
	pthread_mutex_unlock(&pStruH->pStruListTableMutex[iExpireTime]);

	ML_FREE(pStruCN);

	return ML_OK;
}

