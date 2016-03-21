#include <time.h>

#include "st_hub.h"

#define ST_TABLE_SIZE 150000

#define ST_HUB_STATUS_IDLE    0
#define ST_HUB_STATUS_RUNNING 1

//typedef void (*STHubHandleFunc)(char *sRecvData, int )

typedef struct STHubTable
{
	int iSendTimes;
	int iExpireTime;
	int iStatus;
	struct list_head struHead;
}STHubTable, *PSTHubTable;

typedef struct _STHub
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
	STHubTable *pStruListTable;	
	STTimerHandle struTimerHandle;
	STThreadHandle struThreadHandle;
	pthread_mutex_t struMaxIntervalMutex;
	pthread_mutex_t *pStruListTableMutex;
	STHubFunc pHubFunc;
}STHub, *PSTHub;

typedef struct _STHubListData
{
	int   iExpireTime;
	STHub *pStruHub;
	struct list_head struHead;
	struct list_head struNode;
}STHubListData, *PSTHubListData;



static void st_free_hub_list(struct list_head *pStruHead);
static int st_send_hub_list(struct list_head *pStruHead);
static int st_add_send_hub( unsigned long ulData );

int
st_create_hub(
	int iHubNum,
	int iThreadNum,
	int iStackSize,
	STHubFunc pHubFunc,
	STTimerHandle struTHandle,
	STThreadHandle struThreadHandle,
	STHubHandle *pStruHandle
)
{
	int i = 0;
	int iRet = 0;
	STHub *pStruH = NULL;	

	if( iHubNum <= 0 )
	{
		iHubNum = ST_TABLE_SIZE;
	}
	if( iStackSize <= 0 )
	{
		iStackSize = 32 * 1024;
	}
	ST_CALLOC(pStruH, STHub, 1);
	ST_CALLOC(pStruH->pStruListTable, STHubTable, iHubNum);
	ST_CALLOC(pStruH->pStruListTableMutex, pthread_mutex_t, 1);
	pthread_mutex_init(&pStruH->struMaxIntervalMutex, NULL);
	//pthread_mutex_init(&pStruH->struCountMutex);
	pStruH->struTimerHandle = struTHandle;
	pStruH->pHubFunc = pHubFunc;
	pStruH->struThreadHandle = struThreadHandle;

	for( ; i < ST_TABLE_SIZE; i++ )
	{
		pStruH->pStruListTable[i].iSendTimes = 0;
		pStruH->pStruListTable[i].iExpireTime = 0;
		INIT_LIST_HEAD(&pStruH->pStruListTable[i].struHead);
		pthread_mutex_init(&pStruH->pStruListTableMutex[i], NULL);
	}

	st_add_timer(1, 1, ST_TIMER_STATUS_CONSTANT,  (unsigned long)pStruH, st_add_send_hub, NULL, struTHandle, &pStruH->iTimerID);

	iRet = st_create_thread_table_pool(
															iThreadNum, 
															iStackSize,
															st_free_hub_list, 
															NULL,
															st_send_hub_list,
															struThreadHandle,
															&pStruH->iThreadID
														);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	(*pStruHandle) = (STHubHandle)pStruH;

	return iRet;
}

void
st_destroy_hub(
	STHubHandle struHandle
)
{
	int i = 0;
	STHub *pStruH = (STHub *)struHandle;

	if( !struHandle )
	{
		return;
	}

	for( ; i < pStruH->iListTableNum; i++ )	
	{
		pthread_mutex_lock(&pStruH->pStruListTableMutex[i]);
		if( !list_empty(&pStruH->pStruListTable[i].struHead) )
		{
			st_free_hub_list(&pStruH->pStruListTable[i].struHead);
		}
		pthread_mutex_unlock(&pStruH->pStruListTableMutex[i]);
	}

	ST_FREE(pStruH);
}

static void
st_free_hub_list(
		struct list_head *pStruHead
)
{
	struct list_head *pStruSL = pStruHead->next;
	STCommNode *pStruCN = NULL;

	while( pStruSL != pStruHead )
	{
		pStruCN = list_entry(pStruSL, STCommNode, struNode);
		list_del_init(&pStruCN->struNode);
		pStruSL = pStruHead->next;
		free(pStruCN);
	}
}

static void
st_set_hub_interval(
	int iIntervalTime,
	STHub *pStruH
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
st_add_hub_node(
	int				 iHubID,
	int				 iExpireTime,
	STCommNode *pStruCN,
	STHub			 *pStruHub
)
{
	pthread_mutex_lock(&pStruHub->pStruListTableMutex[iHubID]);
	pStruHub->pStruListTable[iHubID].iExpireTime = iExpireTime;
	list_add_tail(&pStruCN->struNode, &pStruHub->pStruListTable[iHubID].struHead);
	pthread_mutex_unlock(&pStruHub->pStruListTableMutex[iHubID]);
}

int
st_add_hub(
	int iSockfd,
	void *pData,
	unsigned long *pulHubID,
	STHubHandle struHandle
)
{
	int iOffset = 0;
	int iIntervalTime = 0;
	time_t Curtime = time(NULL);
	STCommNode *pStruCN = NULL;
	STHub *pStruH = (STHub *)struHandle;

	if( pStruH->uiCount == 0 )
	{
		//重置计时器
		//st_reset_timer();
		pStruH->uiCount = Curtime;
	}
	else
	{
		iOffset = Curtime - pStruH->uiCount;
	}

	PC_DEBUG("time = %d\n", (unsigned int)Curtime);
	ST_CALLOC(pStruCN, STCommNode, 1);

	iIntervalTime = pStruH->uiHubIntervalTime + iOffset;
	pStruCN->pUserData = pData;
	pStruCN->ulData = iIntervalTime;
	(*pulHubID) = (unsigned long)pStruCN;
	//pStruH->ulData = (unsigned long)pStruNode;	
	//clock_gettime(CLOCK_MONOTONIC, &pStruPC->struCreateTime);

	PC_ERROR("iIntervalTime = %d\n", iIntervalTime);
	st_set_hub_interval(iIntervalTime, pStruH);
	
	st_add_hub_node( 
							iIntervalTime , 
							pStruH->uiHubIntervalTime, 
							pStruCN,
							pStruH
						);

	return ST_OK;	
}

static int
st_send_hub_list(
	struct list_head *pStruNode
)
{
	int iRet = ST_OK;
	int iCount = 0;
	int iExpireTime = 0;
	struct list_head *pStruSL = NULL;
	STHub *pStruH = NULL;
	STCommNode *pStruCN = NULL;
	STHubListData *pStruHLD = NULL;

	if(!pStruNode)
	{
		return ST_OK;
	}
	pStruHLD = list_entry(pStruNode, STHubListData, struNode);		

	PC_DEBUG("iExpire = %d\n", pStruHLD->iExpireTime);
	pStruH = pStruHLD->pStruHub;
	pStruSL = pStruHLD->struHead.next;
	//本意是包发完之后再送回的,因为这样子加锁的时间会少点,但是发现这么做会导致结果不稳定,所以先弄回去,这让人感觉在分配是的链表转移有点多余
	pthread_mutex_lock(&pStruH->pStruListTableMutex[pStruHLD->iExpireTime]);
	list_splice_init(&pStruHLD->struHead, &pStruH->pStruListTable[pStruHLD->iExpireTime].struHead);
	pthread_mutex_unlock(&pStruH->pStruListTableMutex[pStruHLD->iExpireTime]);
	PC_ERROR("send data \n");
	while( pStruSL != &pStruH->pStruListTable[pStruHLD->iExpireTime].struHead )
	{
		pStruCN = list_entry(pStruSL, STCommNode, struNode);

		if( pStruH->pHubFunc )
		{
			pStruH->pHubFunc((unsigned long)pStruCN->pUserData);
		}
		//st_send_thread(pStruHN);

		pthread_mutex_lock(&pStruH->pStruListTableMutex[pStruHLD->iExpireTime]);
		pStruSL = pStruSL->next;
		pthread_mutex_unlock(&pStruH->pStruListTableMutex[pStruHLD->iExpireTime]);
		pStruCN = NULL;
	}
	pStruH->pStruListTable[pStruHLD->iExpireTime].iStatus = ST_HUB_STATUS_IDLE;
out:
	free(pStruHLD);
	pStruHLD = NULL;

	return iRet;
}

static int 
st_add_send_hub(
	unsigned long ulData
)
{
	int i = 0;
	int iRet = 0;
	int iCurPos = 0;
	STHub *pStruH = (STHub *)ulData;
	STHubListData *pStruData = NULL;

	st_get_tick(pStruH->struTimerHandle, &iCurPos);
	if( iCurPos == 0 )
	{
		return ST_OK;
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
				pStruH->pStruListTable[i].iStatus == ST_HUB_STATUS_RUNNING)
		{
			continue;
		}
		pStruH->pStruListTable[i].iStatus = ST_HUB_STATUS_RUNNING;

		ST_ERROR("i = %d, iCurpos = %d, expire = %d\n", i, iCurPos, pStruH->pStruListTable[i].iExpireTime);
		pthread_mutex_lock(&pStruH->pStruListTableMutex[i]);
		if( list_empty(&pStruH->pStruListTable[i].struHead) )
		{
			goto next;
		}
		pthread_mutex_unlock(&pStruH->pStruListTableMutex[i]);
		ST_CALLOC(pStruData, STHubListData, 1);

		pStruData->iExpireTime = i;
		pStruData->pStruHub    = pStruH;
		INIT_LIST_HEAD(&pStruData->struHead);
		{
			pStruH->pStruListTable[i].iSendTimes += pStruH->pStruListTable[i].iExpireTime;
			pthread_mutex_lock(&pStruH->pStruListTableMutex[i]);
			list_splice_init(&pStruH->pStruListTable[i].struHead, &pStruData->struHead); 
			pthread_mutex_unlock(&pStruH->pStruListTableMutex[i]);
			st_add_table_thread_pool_node(pStruH->iThreadID, &pStruData->struNode, pStruH->struThreadHandle);
			continue;
		}
next:
		pthread_mutex_unlock(&pStruH->pStruListTableMutex[i]);
	}

	return iRet;
}

int
st_del_hub(
	unsigned long ulHubID,
	STHubHandle struHandle
)
{
	int iExpireTime = 0;
	STHub *pStruH = (STHub *)struHandle;
	STCommNode *pStruCN = (STCommNode*)ulHubID;
	
	if( !struHandle || !ulHubID )
	{
		return ST_PARAM_ERR;	
	}

	pStruCN = (STCommNode *)ulHubID;
	iExpireTime = pStruCN->ulData; 

	pthread_mutex_lock(&pStruH->pStruListTableMutex[iExpireTime]);
	list_del_init(&pStruCN->struNode);
	pthread_mutex_unlock(&pStruH->pStruListTableMutex[iExpireTime]);

	ST_FREE(pStruCN);

	return ST_OK;
}

