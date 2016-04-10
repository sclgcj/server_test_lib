#include "list.h"
#include "ml_comm.h"
#include "ml_timer.h"
#include "ml_recv_check.h"

struct _MLRecvCheck;
typedef struct _MLRecvCheck MLRecvCheck,*PMLRecvCheck;

typedef struct _MLCheckHead
{
	MLRecvCheck			 *pStruRC;
	struct list_head struCheckList;	
	pthread_mutex_t  struCheckListMutex;
}MLCheckHead, *PMLCheckHead;

struct _MLRecvCheck
{
	int iNodeCnt;
	int iRCCount;
	int iTotalCnt;
	int iSaveTime;
	int iTotalLink;
	int iRecvTimeout;
	int	iCheckListNum;
	struct timespec  struTS;					//记录时间
	MLCheckHead			 *pStruCH;
	pthread_mutex_t  struSaveMutex;
	MLTimerHandle    struTimerHandle;
	MLRecvCheckFailFunc pFailFunc;
};

typedef struct _PCCheckNode
{
	int    iCheckStop;
	void   *pUserData;
	void   *pFunc;
	struct timespec struTS;
	struct list_head struNode;
	pthread_mutex_t struMutex;
}MLCheckNode, *PMLCheckNode;

static int
ml_recv_check(
	unsigned long ulData
)
{
	int iRet = 0;
	int iCheckStop = 0;
	struct list_head *pStruHead = NULL;
	struct timespec struTS;
	struct timespec struT1;
	MLCheckHead *pStruCH = (MLCheckHead*)ulData;
	MLCheckNode *pStruCN = NULL;
	MLRecvCheck *pStruRC = NULL;
	
	iRet = pthread_mutex_trylock(&pStruCH->struCheckListMutex);
	if( iRet != 0 )
	{
		if( iRet == EBUSY )
		{
			return ML_OK;
		}
		ML_ERROR("pthread_mutex_trylock: %s\n", strerror(errno));
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
		if( iRet == ML_ERR )
		{
			if( (iCheckStop) == ML_CHECK_TICK_MLOP )
			{
				continue;
			}
			if( struTS.tv_sec - struT1.tv_sec > pStruRC->iRecvTimeout ||
					(struTS.tv_sec - struT1.tv_sec == pStruRC->iRecvTimeout && struTS.tv_nsec - struT1.tv_nsec > 0))
			{
				ML_ERROR("addres = %p, starttime = %ld.%ld, endtime = %ld.%ld\n", pStruCN, struT1.tv_sec, struT1.tv_nsec, struTS.tv_sec, struTS.tv_nsec);
				if( pStruRC->pFailFunc )
				{
					pStruRC->pFailFunc(pStruCN->pUserData);
				}
			}
		}
	}
	pthread_mutex_unlock(&pStruCH->struCheckListMutex);

	return ML_OK;
}



int
ml_create_recv_check(
	int iTotalLink,	
	int iRecvTimeout,
	int iCheckListNum,
	MLTimerHandle struHandle,
	MLRecvCheckFailFunc pFunc,
	MLRecvCheckHandle *pStruHandle
)
{
	int i = 0;
	MLRecvCheck *pStruRC = NULL;	

	ML_CALLOC(pStruRC, MLRecvCheck, 1);
	pStruRC->iTotalLink    = iTotalLink;
	pStruRC->iRecvTimeout  = iRecvTimeout;
	pStruRC->iCheckListNum = pStruRC->iCheckListNum;
	pStruRC->pFailFunc     = pFunc;
	pthread_mutex_init(&pStruRC->struSaveMutex, NULL);
	ML_CALLOC(pStruRC->pStruCH, MLCheckHead, iCheckListNum);
	for( ; i < iCheckListNum; i++ )
	{
		pStruRC->pStruCH[i].pStruRC = pStruRC;
		INIT_LIST_HEAD(&pStruRC->pStruCH[i].struCheckList);
		pthread_mutex_init(&pStruRC->pStruCH[i].struCheckListMutex, NULL);
	}

	return ML_OK;
}

static void
ml_free_recv_check_list(
	struct list_head *pStruHead
)
{
	struct list_head *pStruSL = pStruHead->next;
	MLCheckNode *pStruNode = NULL;

	while(pStruSL != pStruHead)
	{
		pStruNode = list_entry(pStruSL, MLCheckNode, struNode);
		list_del_init(pStruSL);
		free(pStruNode);
		pStruSL = pStruHead->next;
		pthread_mutex_destroy(&pStruNode->struMutex);
	}
}

void
ml_destroy_recv_check(
	MLRecvCheckHandle struHandle
)
{
	int i = 0;
	MLRecvCheck *pStruRC = (MLRecvCheck*)struHandle;

	if( !struHandle )
	{
		return;
	}

	pthread_mutex_destroy(&pStruRC->struSaveMutex);

	for( ; i < pStruRC->iCheckListNum; i++ )	
	{	
		pthread_mutex_lock(&pStruRC->pStruCH[i].struCheckListMutex);
		ml_free_recv_check_list(&pStruRC->pStruCH[i].struCheckList);
		pthread_mutex_unlock(&pStruRC->pStruCH[i].struCheckListMutex);
		pthread_mutex_destroy(&pStruRC->pStruCH[i].struCheckListMutex);
	}
	ML_FREE(pStruRC->pStruCH);
	ML_FREE(pStruRC);
}

int
ml_add_recv_check(
	void              *pUserData,
	MLRecvCheckHandle struHandle,
	unsigned long     *piRCID
)
{
	int iRet = 0;
	int iCurID = 0;
	int iIntervalTime = 0;
	struct timespec struT2;
	MLRecvCheck *pStruRC = (MLRecvCheck*)struHandle;
	MLCheckNode *pStruCN = NULL;
		
	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	if( pStruRC->struTS.tv_sec == 0 )
	{
		clock_gettime(CLOCK_MONOTONIC, &pStruRC->struTS);
	}
	pthread_mutex_lock(&pStruRC->struSaveMutex);
	iCurID = pStruRC->iRCCount;
	pthread_mutex_unlock(&pStruRC->struSaveMutex);

	ML_CALLOC(pStruCN, MLCheckNode, 1);
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
			ml_add_timer(
						pStruRC->iNodeCnt, 
						1, 
						ML_TIMER_MLATUS_CONMLANT, 
						(unsigned long)(&pStruRC->pStruCH[iCurID]),
						ml_recv_check,
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

	return ML_OK;
}

void
ml_stop_recv_check(
	unsigned long ulRCID,
	MLRecvCheckHandle struHandle
)
{
	MLCheckNode *pStruCN = (MLCheckNode *)ulRCID;

	pthread_mutex_lock(&pStruCN->struMutex);
	pStruCN->iCheckStop = ML_CHECK_TICK_MLOP;
	pthread_mutex_unlock(&pStruCN->struMutex);
}

void
ml_start_recv_check(
	unsigned long ulRCID,
	MLRecvCheckHandle struHandle
)
{
	MLCheckNode *pStruCN = (MLCheckNode *)ulRCID;
	pthread_mutex_lock(&pStruCN->struMutex);
	pStruCN->iCheckStop = ML_CHECK_TICK_MLART;
	clock_gettime(CLOCK_MONOTONIC, &pStruCN->struTS);
	pthread_mutex_unlock(&pStruCN->struMutex);
}


