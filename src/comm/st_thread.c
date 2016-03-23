#include "st_comm.h"
#include "st_exit.h"
#include "st_thread.h"

typedef struct _STThreadMember
{
	int							iCount;					  //已经链接个数, 最大个数为ST_THREAD_LIST_LEN
	pthread_mutex_t  struThreadMutex; //每个链表的锁
	pthread_cond_t   struThreadCond;  //每个链表的条件变量
	struct list_head struThreadList;  //链表数组
	void						 *pGroup;					//所属的线程组
	struct list_head struNode;				//加入组链表
}STThreadMember, *PSTThreadMemeber;

typedef struct _STThreadGroup
{
	int iCount;																					//线程组成员个数
	STThreadMember *pStruTM;			 											//线程组成员数组
	void (*group_free)(struct list_head *pStruNode);		//节点释放函数
	int  (*group_func)(struct list_head *pStruNode);		//在分派任务到成员线程之前做的事情
	int  (*execute_func)(struct list_head *pStruNode );	//线程组执行函数
	pthread_mutex_t struGroupMutex;											//组控制线程锁
	pthread_cond_t  struGroupCond;									  	//组控制线程条件变量
	struct list_head struGroupHead;											//组链表
}STThreadGroup, *PSTThreadGroup;

typedef struct _STThreadTable
{
	int iThreadCnt;
	int iThreadGroupNum;
	STThreadGroup *pStruTG;
	pthread_condattr_t struCondAttr;
}STThreadTable, *PSTThreadTable;

static int
st_get_thread_member_node(
	int iSecond,
	STThreadMember *pStruSM,
	struct list_head **pStruNode
);

static int
st_init_thread_table(
	int                iThreadGroupNum,
	pthread_condattr_t *pStruAttr,
	STThreadGroup      *pStruTG
)
{
	int i = 0;

	for(; i < iThreadGroupNum; i++ )
	{
		pthread_mutex_init(&pStruTG[i].struGroupMutex, NULL);
		pthread_cond_init(&pStruTG[i].struGroupCond, pStruAttr);
		INIT_LIST_HEAD(&pStruTG[i].struGroupHead);
	}

	return ST_OK;
}

static int 
st_uninit_thread_group(
	int iThreadGroupNum,
	STThreadGroup *pStruTG
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

	return ST_OK;
}

static void *
st_thread_handle_data(
	void *pArg
)
{
	int iRet = 0;
	STThreadGroup *pStruTG = NULL;
	STThreadMember *pStruSM = (STThreadMember *)pArg;
	struct list_head *pStruNode = NULL;

	pStruTG = (STThreadGroup *)pStruSM->pGroup;

	while(1)
	{
		pStruNode = NULL;

		iRet = st_check_exit();
		if( iRet == ST_OK )
		{
			break;
		}
		iRet = st_get_thread_member_node(
																		1,
																		pStruSM,
																		&pStruNode
																	);
		ST_DEBUG("iRetet = %d\n", iRet);
		if( iRet == ST_TIMEOUT || pStruNode == NULL )
		{
			continue;
		}
		else if( iRet != ST_OK )
		{
			continue;
		}

		if( pStruTG->execute_func )
		{
			iRet = pStruTG->execute_func(pStruNode);
			if( iRet != ST_OK )
			{
				continue;
			}
		}
	}

	pthread_exit(NULL);
}

static int
st_get_thread_node(
	struct list_head *pStruHead,
	struct list_head **ppStruNode
)
{
	if( list_empty(pStruHead) )
	{
		return ST_ERR;
	}

	(*ppStruNode) = pStruHead->next;
	list_del_init(*ppStruNode);

	return ST_OK;
}

static int
st_get_thread_group_node(
	STThreadGroup    *pStruTG,
	struct list_head **pStruNode
)
{
	int iRet = 0;
	struct timespec struTS;

	clock_gettime(CLOCK_MONOTONIC, &struTS);
	struTS.tv_sec += 1;
	pthread_mutex_lock(&pStruTG->struGroupMutex);
	iRet = st_get_thread_node(&pStruTG->struGroupHead, pStruNode);
	ST_DEBUG("iRet = %d\n", iRet);
	if( iRet != ST_OK )
	{
		iRet = pthread_cond_timedwait(&pStruTG->struGroupCond, &pStruTG->struGroupMutex, &struTS);
		ST_DEBUG("iRet = %d, errno = %d, ETIMEDOUT = %d\n", iRet, errno, ETIMEDOUT);
		if( iRet == ETIMEDOUT )//list_empty(&gStruThreadTable[ID].struGroupHead) )
		{
			iRet = ST_TIMEOUT;
		}
		else 
		{
			iRet = st_get_thread_node(&pStruTG->struGroupHead, pStruNode);
		}
	}
	pthread_mutex_unlock(&pStruTG->struGroupMutex);

	return iRet;
}

static int
st_add_thread_member_node(
	struct list_head *pStruNode, 
	STThreadMember *pStruMemb
)
{
	int iRet = 0;

	pthread_mutex_lock(&pStruMemb->struThreadMutex);
	ST_DEBUG("pStruMemb->iCOunt = %d\n", pStruMemb->iCount);
	
	list_add_tail(pStruNode,&pStruMemb->struThreadList);
	pthread_cond_broadcast(&pStruMemb->struThreadCond);
	pStruMemb->iCount++;
	ST_DEBUG("pStruMemb->iCOunt = %d\n", pStruMemb->iCount);
	
	pthread_mutex_unlock(&pStruMemb->struThreadMutex);

	return ST_OK;
}

static int
st_get_thread_member_node(
	int iSecond,
	STThreadMember *pStruSM,
	struct list_head **pStruNode
)
{
	int iRet = ST_OK;
	struct timespec struTV;

	//使用CLOCK_MONOTONIC, 采用系统开机时间所经过的秒数,可以让事件更加的准确,因为直接使用REAL_TIME形式,会出现事件不准的情况
	clock_gettime(CLOCK_MONOTONIC, &struTV);
	struTV.tv_sec += 1;

	pthread_mutex_lock(&pStruSM->struThreadMutex);
	iRet = st_get_thread_node(&pStruSM->struThreadList, pStruNode);
	if( iRet != ST_OK )
	{
		iRet = pthread_cond_timedwait(&pStruSM->struThreadCond, &pStruSM->struThreadMutex, &struTV );	
		//ST_DEBUG("iRet = %d, errno = %d\n", iRet, errno);
		if( iRet == ETIMEDOUT )// list_empty(&pStruMemb->struThreadList) )
		{
			iRet = ST_TIMEOUT;
		}	
		else //这里没有做其他错误的判断,是因为拿不准其他错误判断, 所以统一使用是链表是否为空来判断是否可以进行处理
		{
			iRet = st_get_thread_node(&pStruSM->struThreadList, pStruNode);
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
st_master_thread(
	void *pData
)
{
	int i = 0;
	int iRet = 0;
	int iCount = 0;
	STThreadGroup *pStruTG = (STThreadGroup *)pData;
	struct list_head *pStruNode = NULL;

	while(1)
	{
		ST_DEBUG("\n");
		iRet = st_check_exit();
		if( iRet == ST_OK )
		{
			break;
		}
		iRet = st_get_thread_group_node( pStruTG, &pStruNode );
		if( iRet == ST_TIMEOUT )
		{
			continue;
		}
		else if( iRet != ST_OK )
		{
			break;
		}

		//这里用于处理线程组中,在用线程处理不同任务之前所需要共同处理的部分,这里最好尽量少花时间
		if( pStruTG->group_func ) 
		{
			iRet = pStruTG->group_func(pStruNode);
			ST_DEBUG("mater return = %d\n", iRet);
			if( iRet != ST_OK )
			{
				continue;
			}
		}
		ST_DEBUG("\n");

		for( i = iCount; i < pStruTG->iCount; i++ )
		{
			iRet = st_add_thread_member_node(pStruNode, &pStruTG->pStruTM[i]);
			if( iRet == ST_OK )
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
st_create_master_thread(
	pthread_attr_t *pStruAttr,
	STThreadGroup  *pStruGrp
)
{
	pthread_t th;

	if( pthread_create(&th, pStruAttr, st_master_thread, (void *)pStruGrp) < 0 )
	{
		ST_ERROR( "pthread_create: %s\n", strerror(errno) );
		return ST_ERR;
	}
	return ST_OK;
}

static int
st_init_thread_member(
	pthread_condattr_t *pStruAttr,
	STThreadGroup			 *pStruGrp,
	STThreadMember		 *pStruTM
)
{
	pthread_mutex_init(&pStruTM->struThreadMutex, NULL);
	pthread_cond_init(&pStruTM->struThreadCond, pStruAttr);
	INIT_LIST_HEAD(&pStruTM->struThreadList);
	pStruTM->pGroup = (void *)pStruGrp;
}

static int
st_create_member_thread(
	pthread_attr_t     *pStruAttr,
	pthread_condattr_t *pStruCondAttr,
	STThreadGroup			 *pStruTG
)
{
	int i = 0;
	int iNum = pStruTG->iCount;
	pthread_t th;

	ST_CALLOC(pStruTG->pStruTM, STThreadMember, iNum);
	for( ; i < iNum; i++ )	
	{
		st_init_thread_member(
																pStruCondAttr,
																pStruTG,	
																&pStruTG->pStruTM[i]
															);
		if( pthread_create(&th, pStruAttr, st_thread_handle_data, &pStruTG->pStruTM[i]) )
		{
			ST_ERROR( "pthread_create: %s\n", strerror(errno));
			return ST_ERR;
		}
	}

	return ST_OK;
}

static int
st_create_thread_pool(
	int  iThreadNum,
	int  iStackSize,
	void (*group_free)(struct list_head *),
	int  (*group_func)(struct list_head *),
	int  (*execute_func)(struct list_head *),
	pthread_condattr_t *pStruCondAttr,
	STThreadGroup *pStruTG
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
		return ST_ERR;
	}

	if( iThreadNum <= 0 )
	{
		iThreadNum = 50;
	}
	if( iStackSize <= 0 )
	{
		iStackSize = 32 * 1024;
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

	iRet = st_create_master_thread(&struAttr, pStruTG);
	if( iRet != ST_OK )
	{
		return iRet;
	}
	
	pthread_attr_setstacksize(&struAttr, iStackSize); 
	return st_create_member_thread( &struAttr, pStruCondAttr, pStruTG );
}

static int
st_add_thread_pool_node(
	struct list_head  *pStruNode,
	STThreadGroup     *pStruTG
)
{
	int iRet = 0;

	pthread_mutex_lock(&pStruTG->struGroupMutex);
	list_add_tail(pStruNode, &pStruTG->struGroupHead);
	pthread_cond_broadcast(&pStruTG->struGroupCond);
	pthread_mutex_unlock(&pStruTG->struGroupMutex);

	return ST_OK;
}

int
st_create_thread_table(
	int iThreadGroupNum,
	STThreadHandle *pStruHandle
)
{
	STThreadTable *pStruTT = NULL;

	ST_CALLOC(pStruTT, STThreadTable, 1);
	pStruTT->iThreadGroupNum = iThreadGroupNum;
	pthread_condattr_init(&(pStruTT->struCondAttr));
	pthread_condattr_setclock(&(pStruTT->struCondAttr), CLOCK_MONOTONIC);
	ST_CALLOC(pStruTT->pStruTG, STThreadGroup, pStruTT->iThreadGroupNum);	
	st_init_thread_table(
												pStruTT->iThreadGroupNum, 
												&pStruTT->struCondAttr,
												pStruTT->pStruTG
											);

	(*pStruHandle) = (void *)pStruTT;

	return ST_OK;
}

int 
st_destroy_thread_table(
	STThreadHandle struHandle
)
{
	STThreadTable *pStruTT = (STThreadTable *)struHandle;

	if( !pStruTT )
	{
		return ST_PARAM_ERR;
	}

	st_uninit_thread_group(
												pStruTT->iThreadGroupNum,
												pStruTT->pStruTG
											);
	free(pStruTT->pStruTG);
	pStruTT->pStruTG = NULL;
	pthread_condattr_destroy(&pStruTT->struCondAttr);

	return ST_OK;
}

int
st_create_thread_table_pool(	
	int iThreadNum,
	int iStackSize,
	void (*group_free)(struct list_head *),
	int  (*group_func)(struct list_head *),
	int  (*execute_func)(struct list_head *),	
	STThreadHandle struHandle,
	int  *piThreadID
)
{
	STThreadTable *pStruTT = (STThreadTable*)struHandle;
	if( !struHandle || !piThreadID )
	{
		return ST_PARAM_ERR;
	}
	(*piThreadID) = pStruTT->iThreadCnt++;

	return st_create_thread_pool(
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
st_add_table_thread_pool_node(
	int							 iThreadID,
	struct list_head *pStruNode,
	STThreadHandle	 struHandle
)
{
	STThreadTable *pStruTT = (STThreadTable *)struHandle;

	if( !struHandle || !pStruNode )
	{
		return ST_PARAM_ERR;
	}
	if( iThreadID < 0 || iThreadID >= pStruTT->iThreadCnt )
	{
		return ST_PARAM_ERR;	
	}

	return st_add_thread_pool_node(pStruNode, &pStruTT->pStruTG[iThreadID]);
}

