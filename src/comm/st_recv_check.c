#include "list.h"
#include "st_comm.h"
#include "st_timer.h"
#include "st_recv_check.h"

struct _STRecvCheck;
typedef struct _STRecvCheck STRecvCheck,*PSTRecvCheck;

typedef struct _STCheckHead
{
	STRecvCheck			 *pStruRC;
	struct list_head struCheckList;	
	pthread_mutex_t  struCheckListMutex;
}STCheckHead, *PSTCheckHead;

struct _STRecvCheck
{
	int iNodeCnt;
	int iRCCount;
	int iTotalCnt;
	int iSaveTime;
	int iTotalLink;
	int iRecvTimeout;
	int	iCheckListNum;
	struct timespec  struTS;					//记录时间
	STCheckHead			 *pStruCH;
	pthread_mutex_t  struSaveMutex;
	STTimerHandle    struTimerHandle;
	STRecvCheckFailFunc pFailFunc;
};

typedef struct _PCCheckNode
{
	int    iCheckStop;
	void   *pUserData;
	void   *pFunc;
	struct timespec struTS;
	struct list_head struNode;
	pthread_mutex_t struMutex;
}STCheckNode, *PSTCheckNode;

static int
st_recv_check(
	unsigned long ulData
)
{
	int iRet = 0;
	int iCheckStop = 0;
	struct list_head *pStruHead = NULL;
	struct timespec struTS;
	struct timespec struT1;
	STCheckHead *pStruCH = (STCheckHead*)ulData;
	STCheckNode *pStruCN = NULL;
	STRecvCheck *pStruRC = NULL;
	
	iRet = pthread_mutex_trylock(&pStruCH->struCheckListMutex);
	if( iRet != 0 )
	{
		if( iRet == EBUSY )
		{
			return ST_OK;
		}
		ST_ERROR("pthread_mutex_trylock: %s\n", strerror(errno));
		exit(0);
	}
	pStruHead = &pStruCH->struCheckList;
	pStruRC = pStruCH->pStruRC;	
	clock_gettime(CLOCK_MONOTONIC, &struTS);
	list_for_each_entry(pStruCN, pStruHead, struNode)
	{
		pthread_mutex_lock(&pStruCN->struMutex);
		iCheckStop = pStruCN->iCheckStop;
		struT1     = pStruCN->struTS;
		pthread_mutex_unlock(&pStruCN->struMutex);
		if( iRet == ST_ERR )
		{
			if( (iCheckStop) == ST_CHECK_TICK_STOP )
			{
				continue;
			}
			if( struTS.tv_sec - struT1.tv_sec > pStruRC->iRecvTimeout ||
					(struTS.tv_sec - struT1.tv_sec == pStruRC->iRecvTimeout && struTS.tv_nsec - struT1.tv_nsec > 0))
			{
				ST_ERROR("addres = %p, starttime = %ld.%ld, endtime = %ld.%ld\n", pStruCN, struT1.tv_sec, struT1.tv_nsec, struTS.tv_sec, struTS.tv_nsec);
				if( pStruRC->pFailFunc )
				{
					pStruRC->pFailFunc(pStruCN->pUserData);
				}
			}
		}
	}
	pthread_mutex_unlock(&pStruCH->struCheckListMutex);

	return ST_OK;
}



int
st_create_recv_check(
	int iTotalLink,	
	int iRecvTimeout,
	int iCheckListNum,
	STTimerHandle struHandle,
	STRecvCheckFailFunc pFunc,
	STRecvCheckHandle *pStruHandle
)
{
	int i = 0;
	STRecvCheck *pStruRC = NULL;	

	ST_CALLOC(pStruRC, STRecvCheck, 1);
	pStruRC->iTotalLink    = iTotalLink;
	pStruRC->iRecvTimeout  = iRecvTimeout;
	pStruRC->iCheckListNum = pStruRC->iCheckListNum;
	pStruRC->pFailFunc     = pFunc;
	pthread_mutex_init(&pStruRC->struSaveMutex, NULL);
	ST_CALLOC(pStruRC->pStruCH, STCheckHead, iCheckListNum);
	for( ; i < iCheckListNum; i++ )
	{
		pStruRC->pStruCH[i].pStruRC = pStruRC;
		INIT_LIST_HEAD(&pStruRC->pStruCH[i].struCheckList);
		pthread_mutex_init(&pStruRC->pStruCH[i].struCheckListMutex, NULL);
	}

	return ST_OK;
}

static void
st_free_recv_check_list(
	struct list_head *pStruHead
)
{
	struct list_head *pStruSL = pStruHead->next;
	STCheckNode *pStruNode = NULL;

	while(pStruSL != pStruHead)
	{
		pStruNode = list_entry(pStruSL, STCheckNode, struNode);
		list_del_init(pStruSL);
		free(pStruNode);
		pStruSL = pStruHead->next;
		pthread_mutex_destroy(&pStruNode->struMutex);
	}
}

void
st_destroy_recv_check(
	STRecvCheckHandle struHandle
)
{
	int i = 0;
	STRecvCheck *pStruRC = (STRecvCheck*)struHandle;

	if( !struHandle )
	{
		return;
	}

	pthread_mutex_destroy(&pStruRC->struSaveMutex);

	for( ; i < pStruRC->iCheckListNum; i++ )	
	{	
		pthread_mutex_lock(&pStruRC->pStruCH[i].struCheckListMutex);
		st_free_recv_check_list(&pStruRC->pStruCH[i].struCheckList);
		pthread_mutex_unlock(&pStruRC->pStruCH[i].struCheckListMutex);
		pthread_mutex_destroy(&pStruRC->pStruCH[i].struCheckListMutex);
	}
	ST_FREE(pStruRC->pStruCH);
	ST_FREE(pStruRC);
}

int
st_add_recv_check(
	void              *pUserData,
	STRecvCheckHandle struHandle,
	unsigned long     *piRCID
)
{
	int iRet = 0;
	int iCurID = 0;
	int iIntervalTime = 0;
	struct timespec struT2;
	STRecvCheck *pStruRC = (STRecvCheck*)struHandle;
	STCheckNode *pStruCN = NULL;
		
	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	if( pStruRC->struTS.tv_sec == 0 )
	{
		clock_gettime(CLOCK_MONOTONIC, &pStruRC->struTS);
	}
	pthread_mutex_lock(&pStruRC->struSaveMutex);
	iCurID = pStruRC->iRCCount;
	pthread_mutex_unlock(&pStruRC->struSaveMutex);

	ST_CALLOC(pStruCN, STCheckNode, 1);
	clock_gettime(CLOCK_MONOTONIC, &struT2);
	iIntervalTime = struT2.tv_sec - pStruRC->struTS.tv_sec;
	pStruCN->pUserData = pUserData;
	clock_gettime(CLOCK_MONOTONIC, &pStruCN->struTS);
	pthread_mutex_init(&pStruCN->struMutex, NULL);

	pthread_mutex_lock(&pStruRC->pStruCH[iCurID].struCheckListMutex);
	pthread_mutex_lock(&pStruRC->struSaveMutex);
	if( pStruRC->iSaveTime != iIntervalTime || pStruRC->iTotalCnt == pStruRC->iTotalLink - 1 )
	{
		if( pStruRC->iSaveTime != -1 || pStruRC->iTotalCnt == pStruRC->iTotalLink - 1)
		{
			if( pStruRC->iNodeCnt == 0 )
			{
				pStruRC->iNodeCnt = 1;
			}
			if( pStruRC->iSaveTime == -1 )
			{
				pStruRC->iSaveTime = 0;
			}
			st_add_timer(
						pStruRC->iNodeCnt, 
						1, 
						ST_TIMER_STATUS_CONSTANT, 
						(unsigned long)(&pStruRC->pStruCH[iCurID]),
						st_recv_check,
						NULL, 
						pStruRC->struTimerHandle, 
						NULL 
					);
			pStruRC->iRCCount++;
		}
		pStruRC->iNodeCnt = 0;
		pStruRC->iSaveTime = iIntervalTime;
	}
	list_add_tail(&pStruCN->struNode, &pStruRC->pStruCH[iCurID].struCheckList);
	pStruRC->iNodeCnt++;
	pStruRC->iTotalCnt++;
	pthread_mutex_unlock(&pStruRC->struSaveMutex);
	pthread_mutex_unlock(&pStruRC->pStruCH[iCurID].struCheckListMutex);

	return ST_OK;
}

void
st_stop_recv_check(
	unsigned long ulRCID,
	STRecvCheckHandle struHandle
)
{
	STCheckNode *pStruCN = (STCheckNode *)ulRCID;

	pthread_mutex_lock(&pStruCN->struMutex);
	pStruCN->iCheckStop = ST_CHECK_TICK_STOP;
	pthread_mutex_unlock(&pStruCN->struMutex);
}

void
st_start_recv_check(
	unsigned long ulRCID,
	STRecvCheckHandle struHandle
)
{
	STCheckNode *pStruCN = (STCheckNode *)ulRCID;
	pthread_mutex_lock(&pStruCN->struMutex);
	pStruCN->iCheckStop = ST_CHECK_TICK_START;
	clock_gettime(CLOCK_MONOTONIC, &pStruCN->struTS);
	pthread_mutex_unlock(&pStruCN->struMutex);
}


