#include "ml_comm.h"
#include "ml_thread.h"

typedef struct _MLThreadMember
{
	int							iCount;					  //已经链接个数, 最大个数为ML_THREAD_LIML_LEN
	pthread_mutex_t  struThreadMutex; //每个链表的锁
	pthread_cond_t   struThreadCond;  //每个链表的条件变量
	struct list_head struThreadList;  //链表数组
	void						 *pGroup;					//所属的线程组
	struct list_head struNode;				//加入组链表
}MLThreadMember, *PMLThreadMemeber;

typedef struct _MLThreadGroup
{
	int iCount;																					//线程组成员个数
	void (*group_free)(struct list_head *pStruNode);		//节点释放函数
	int  (*group_func)(struct list_head *pStruNode);		//在分派任务到成员线程之前做的事情
	int  (*execute_func)(struct list_head *pStruNode );	//线程组执行函数
	void           *pStruTT;    												//belonging thread table
	MLThreadMember *pStruTM;			 											//线程组成员数组
	pthread_mutex_t struGroupMutex;											//组控制线程锁
	pthread_cond_t  struGroupCond;									  	//组控制线程条件变量
	struct list_head struGroupHead;											//组链表
}MLThreadGroup, *PMLThreadGroup;

typedef struct _MLThreadTable
{
	int iThreadCnt;
	int iThreadGroupNum;
	MLExitHandle  struExitHandle;
	MLThreadGroup *pStruTG;
	pthread_condattr_t struCondAttr;
}MLThreadTable, *PMLThreadTable;

static int
ml_get_thread_member_node(
	int iSecond,
	MLThreadMember *pStruSM,
	struct list_head **pStruNode
);

static int
ml_init_thread_table(
	int                iThreadGroupNum,
	pthread_condattr_t *pStruAttr,
	MLThreadTable      *pStruTT,
	MLThreadGroup      *pStruTG
)
{
	int i = 0;

	for(; i < iThreadGroupNum; i++ )
	{
		pthread_mutex_init(&pStruTG[i].struGroupMutex, NULL);
		pthread_cond_init(&pStruTG[i].struGroupCond, pStruAttr);
		INIT_LIML_HEAD(&pStruTG[i].struGroupHead);
		pStruTG[i].pStruTT = pStruTT;
	}

	return ML_OK;
}

static int 
ml_uninit_thread_group(
	int iThreadGroupNum,
	MLThreadGroup *pStruTG
)
{
	int i = 0, j = 0;
	struct list_head *pStruSL = NULL;

	for( ; i < iThreadGroupNum; i++ )
	{
		for( j = 0; j < pStruTG[i].iCount; j++ )
		{
			if( !(pStruTG[i].pStruTM) )
			{
				continue;
			}
			if( list_empty(&pStruTG[i].pStruTM[j].struThreadList) )
			{
				continue;
			}

			pStruSL = pStruTG[i].pStruTM[j].struThreadList.next;
			while(pStruSL != &pStruTG[i].pStruTM[j].struThreadList)
			{
				list_del_init(pStruSL);
				if( pStruTG[i].group_free )
				{
					pStruTG[i].group_free(
												pStruSL
											);
				}
				pStruSL = pStruTG[i].pStruTM[j].struThreadList.next;
			}

			pthread_mutex_destroy(&pStruTG[i].pStruTM[j].struThreadMutex);
			pthread_cond_destroy(&pStruTG[i].pStruTM[j].struThreadCond);
		}
		free(pStruTG[i].pStruTM);
		pStruTG[i].pStruTM = NULL;
	}

	return ML_OK;
}

static void *
ml_thread_handle_data(
	void *pArg
)
{
	int iRet = 0;
	MLThreadTable *pStruTT = NULL;
	MLThreadGroup *pStruTG = NULL;
	MLThreadMember *pStruSM = (MLThreadMember *)pArg;
	struct list_head *pStruNode = NULL;

	pStruTG = (MLThreadGroup *)pStruSM->pGroup;
	pStruTT = (MLThreadTable *)pStruTG->pStruTT;

	while(1)
	{
		pStruNode = NULL;

		iRet = ml_check_exit(pStruTT->struExitHandle);
		if( iRet == ML_OK )
		{
			break;
		}
		iRet = ml_get_thread_member_node(
																		1,
																		pStruSM,
																		&pStruNode
																	);
		if( iRet == ML_TIMEOUT || pStruNode == NULL )
		{
			continue;
		}
		else if( iRet != ML_OK )
		{
			continue;
		}
		ML_DEBUG("iRetet = %d\n", iRet);

		if( pStruTG->execute_func )
		{
			iRet = pStruTG->execute_func(pStruNode);
			if( iRet != ML_OK )
			{
				continue;
			}
		}
	}

	pthread_exit(NULL);
}

static int
ml_get_thread_node(
	struct list_head *pStruHead,
	struct list_head **ppStruNode
)
{
	if( list_empty(pStruHead) )
	{
		return ML_ERR;
	}

	(*ppStruNode) = pStruHead->next;
	list_del_init(*ppStruNode);

	return ML_OK;
}

static int
ml_get_thread_group_node(
	MLThreadGroup    *pStruTG,
	struct list_head **pStruNode
)
{
	int iRet = 0;
	struct timespec struTS;

	clock_gettime(CLOCK_MONOTONIC, &struTS);
	struTS.tv_sec += 1;
	pthread_mutex_lock(&pStruTG->struGroupMutex);
	iRet = ml_get_thread_node(&pStruTG->struGroupHead, pStruNode);
	ML_DEBUG("iRet = %d\n", iRet);
	if( iRet != ML_OK )
	{
		iRet = pthread_cond_timedwait(&pStruTG->struGroupCond, &pStruTG->struGroupMutex, &struTS);
		ML_DEBUG("iRet = %d, errno = %d, ETIMEDOUT = %d\n", iRet, errno, ETIMEDOUT);
		if( iRet == ETIMEDOUT )//list_empty(&gStruThreadTable[ID].struGroupHead) )
		{
			iRet = ML_TIMEOUT;
		}
		else 
		{
			iRet = ml_get_thread_node(&pStruTG->struGroupHead, pStruNode);
		}
	}
	pthread_mutex_unlock(&pStruTG->struGroupMutex);

	return iRet;
}

static int
ml_add_thread_member_node(
	struct list_head *pStruNode, 
	MLThreadMember *pStruMemb
)
{
	int iRet = 0;

	pthread_mutex_lock(&pStruMemb->struThreadMutex);
	ML_DEBUG("pStruMemb->iCOunt = %d\n", pStruMemb->iCount);
	
	list_add_tail(pStruNode,&pStruMemb->struThreadList);
	pthread_cond_broadcast(&pStruMemb->struThreadCond);
	pStruMemb->iCount++;
	ML_DEBUG("pStruMemb->iCOunt = %d\n", pStruMemb->iCount);
	
	pthread_mutex_unlock(&pStruMemb->struThreadMutex);

	return ML_OK;
}

static int
ml_get_thread_member_node(
	int iSecond,
	MLThreadMember *pStruSM,
	struct list_head **pStruNode
)
{
	int iRet = ML_OK;
	struct timespec struTV;

	//使用CLOCK_MONOTONIC, 采用系统开机时间所经过的秒数,可以让事件更加的准确,因为直接使用REAL_TIME形式,会出现事件不准的情况
	clock_gettime(CLOCK_MONOTONIC, &struTV);
	struTV.tv_sec += 1;

	pthread_mutex_lock(&pStruSM->struThreadMutex);
	iRet = ml_get_thread_node(&pStruSM->struThreadList, pStruNode);
	if( iRet != ML_OK )
	{
		iRet = pthread_cond_timedwait(&pStruSM->struThreadCond, &pStruSM->struThreadMutex, &struTV );	
		//ML_DEBUG("iRet = %d, errno = %d\n", iRet, errno);
		if( iRet == ETIMEDOUT )// list_empty(&pStruMemb->struThreadList) )
		{
			iRet = ML_TIMEOUT;
		}	
		else //这里没有做其他错误的判断,是因为拿不准其他错误判断, 所以统一使用是链表是否为空来判断是否可以进行处理
		{
			iRet = ml_get_thread_node(&pStruSM->struThreadList, pStruNode);
			pStruSM->iCount--;
		}
	}
	else
	{
		pStruSM->iCount--;
	}
	pthread_mutex_unlock(&pStruSM->struThreadMutex);
	
	return iRet;
}

static void *
ml_master_thread(
	void *pData
)
{
	int i = 0;
	int iRet = 0;
	int iCount = 0;
	MLThreadTable *pStruTT = NULL;
	MLThreadGroup *pStruTG = (MLThreadGroup *)pData;
	struct list_head *pStruNode = NULL;

	pStruTT = (MLThreadTable*)pStruTG->pStruTT;

	while(1)
	{
		ML_DEBUG("\n");
		iRet = ml_check_exit(pStruTT->struExitHandle);
		if( iRet == ML_OK )
		{
			break;
		}
		iRet = ml_get_thread_group_node( pStruTG, &pStruNode );
		if( iRet == ML_TIMEOUT )
		{
			continue;
		}
		else if( iRet != ML_OK )
		{
			break;
		}
		ML_DEBUG("iRet == %d\n", iRet);

		//这里用于处理线程组中,在用线程处理不同任务之前所需要共同处理的部分,这里最好尽量少花时间
		if( pStruTG->group_func ) 
		{
			iRet = pStruTG->group_func(pStruNode);
			ML_DEBUG("mater return = %d\n", iRet);
			if( iRet != ML_OK )
			{
				continue;
			}
		}
		ML_DEBUG("\n");

		ML_DEBUG("iCount = %d, i = %d\n", pStruTG->iCount, i);
		for( i = iCount; i < pStruTG->iCount; i++ )
		{
			iRet = ml_add_thread_member_node(pStruNode, &pStruTG->pStruTM[i]);
			if( iRet == ML_OK )
			{
				break;
			}
		}
		if( i + 1 == pStruTG->iCount || i == pStruTG->iCount )
		{
			i = 0;
		}
		i++;
		iCount = i;
		if( iCount == pStruTG->iCount )
		{
			iCount = 0;
		}
	}
	pthread_exit(NULL);
}

static int
ml_create_master_thread(
	pthread_attr_t *pStruAttr,
	MLThreadGroup  *pStruGrp
)
{
	pthread_t th;

	if( pthread_create(&th, pStruAttr, ml_master_thread, (void *)pStruGrp) < 0 )
	{
		ML_ERROR( "pthread_create: %s\n", strerror(errno) );
		return ML_ERR;
	}
	return ML_OK;
}

static int
ml_init_thread_member(
	pthread_condattr_t *pStruAttr,
	MLThreadGroup			 *pStruGrp,
	MLThreadMember		 *pStruTM
)
{
	pthread_mutex_init(&pStruTM->struThreadMutex, NULL);
	pthread_cond_init(&pStruTM->struThreadCond, pStruAttr);
	INIT_LIML_HEAD(&pStruTM->struThreadList);
	pStruTM->pGroup = (void *)pStruGrp;
}

static int
ml_create_member_thread(
	pthread_attr_t     *pStruAttr,
	pthread_condattr_t *pStruCondAttr,
	MLThreadGroup			 *pStruTG
)
{
	int i = 0;
	int iNum = pStruTG->iCount;
	pthread_t th;

	ML_CALLOC(pStruTG->pStruTM, MLThreadMember, iNum);
	for( ; i < iNum; i++ )	
	{
		ml_init_thread_member(
																pStruCondAttr,
																pStruTG,	
																&pStruTG->pStruTM[i]
															);
		if( pthread_create(&th, pStruAttr, ml_thread_handle_data, &pStruTG->pStruTM[i]) )
		{
			ML_ERROR( "pthread_create: %s\n", strerror(errno));
			return ML_ERR;
		}
	}

	return ML_OK;
}

static int
ml_create_thread_pool(
	int  iThreadNum,
	int  iStackSize,
	void (*group_free)(struct list_head *),
	int  (*group_func)(struct list_head *),
	int  (*execute_func)(struct list_head *),
	pthread_condattr_t *pStruCondAttr,
	MLThreadGroup *pStruTG
)
{
	int i = 0;
	int iRet = 0;
	int iNum = 0;
	int ID = 0;
	int iThreadID = 0;
	pthread_attr_t struAttr;

	if( !pStruTG )
	{
		return ML_ERR;
	}

	pStruTG->iCount      = iThreadNum;
	pStruTG->execute_func = execute_func;
	pStruTG->group_free  = group_free;
	pStruTG->group_func  = group_func;

	pthread_attr_init(&struAttr);
	//线程大小设置为128K, 感觉有点高了, 后期慢慢调整, 就目前线程使用数的上线来说,其实16K已经大大的满足了,以后可以测试一下
	pthread_attr_setstacksize(&struAttr, 8 * 1024); 
	//设置为离散状态,由系统自动回收线程资源
	pthread_attr_setdetachstate(&struAttr, PTHREAD_CREATE_DETACHED);

	iRet = ml_create_master_thread(&struAttr, pStruTG);
	if( iRet != ML_OK )
	{
		return iRet;
	}
	
	pthread_attr_setstacksize(&struAttr, iStackSize); 
	return ml_create_member_thread( &struAttr, pStruCondAttr, pStruTG );
}

static int
ml_add_thread_pool_node(
	struct list_head  *pStruNode,
	MLThreadGroup     *pStruTG
)
{
	int iRet = 0;

	pthread_mutex_lock(&pStruTG->struGroupMutex);
	list_add_tail(pStruNode, &pStruTG->struGroupHead);
	pthread_cond_broadcast(&pStruTG->struGroupCond);
	pthread_mutex_unlock(&pStruTG->struGroupMutex);

	return ML_OK;
}

int
ml_create_thread_table(
	int iThreadGroupNum,
	MLExitHandle   struExitHandle,
	MLThreadHandle *pStruHandle
)
{
	MLThreadTable *pStruTT = NULL;

	ML_CALLOC(pStruTT, MLThreadTable, 1);
	pStruTT->struExitHandle  = struExitHandle;
	pStruTT->iThreadGroupNum = iThreadGroupNum;
	pthread_condattr_init(&(pStruTT->struCondAttr));
	pthread_condattr_setclock(&(pStruTT->struCondAttr), CLOCK_MONOTONIC);
	ML_CALLOC(pStruTT->pStruTG, MLThreadGroup, pStruTT->iThreadGroupNum);	
	ml_init_thread_table(
												pStruTT->iThreadGroupNum, 
												&pStruTT->struCondAttr,
												pStruTT,
												pStruTT->pStruTG
											);

	(*pStruHandle) = (void *)pStruTT;

	return ML_OK;
}

int 
ml_destroy_thread_table(
	MLThreadHandle struHandle
)
{
	MLThreadTable *pStruTT = (MLThreadTable *)struHandle;

	if( !pStruTT )
	{
		return ML_PARAM_ERR;
	}

	ml_uninit_thread_group(
												pStruTT->iThreadGroupNum,
												pStruTT->pStruTG
											);
	free(pStruTT->pStruTG);
	pStruTT->pStruTG = NULL;
	pthread_condattr_destroy(&pStruTT->struCondAttr);

	return ML_OK;
}

int
ml_create_thread_table_pool(	
	int iThreadNum,
	int iStackSize,
	void (*group_free)(struct list_head *),
	int  (*group_func)(struct list_head *),
	int  (*execute_func)(struct list_head *),	
	MLThreadHandle struHandle,
	int  *piThreadID
)
{
	MLThreadTable *pStruTT = (MLThreadTable*)struHandle;
	if( !struHandle || !piThreadID )
	{
		return ML_PARAM_ERR;
	}
	(*piThreadID) = pStruTT->iThreadCnt++;

	return ml_create_thread_pool(
														iThreadNum, 
														iStackSize,
														group_free,
														group_func,
														execute_func,
														&pStruTT->struCondAttr,
														&pStruTT->pStruTG[(*piThreadID)]
													);
}

int
ml_add_table_thread_pool_node(
	int							 iThreadID,
	struct list_head *pStruNode,
	MLThreadHandle	 struHandle
)
{
	MLThreadTable *pStruTT = (MLThreadTable *)struHandle;

	if( !struHandle || !pStruNode )
	{
		return ML_PARAM_ERR;
	}
	if( iThreadID < 0 || iThreadID >= pStruTT->iThreadCnt )
	{
		return ML_PARAM_ERR;	
	}

	return ml_add_thread_pool_node(pStruNode, &pStruTT->pStruTG[iThreadID]);
}

