#include "m_list_manage.h"

static void
m_init_mblist(
	MBList *pStruMBL
)
{
	ML_ERROR("ererer\n");
	INIT_LIST_HEAD(&pStruMBL->struHead);
	pthread_mutex_init(&pStruMBL->struMutex, NULL);
}

static void
__m_get_mblist_manage(
	int					  iType,
	MBListManage  *pStruMBM,
	unsigned long *pulDataArray
)
{
	int i = 0;
	struct list_head *pStruNode = NULL;

	pthread_mutex_lock(&pStruMBM->struMBList[iType].struMutex);

	list_for_each(pStruNode, &pStruMBM->struMBList[iType].struHead)
	{
		(pulDataArray)[i] = (unsigned long)pStruNode;
		i++;
	}

	pthread_mutex_unlock(&pStruMBM->struMBList[iType].struMutex);
}

static int
m_get_mblist_manage(
	int					  iType,
	MBListManage  *pStruMBM,
	int						*iArrayLen,
	unsigned long **ppulDataArray
)
{
	int i = 0;
	int iSize = 0;
	struct list_head *pStruNode = NULL;

	if( !pStruMBM || iType >= M_LIST_MAX )
	{
		return ML_PARAM_ERR;
	}

	if( iType < M_LIST_MAX )
	{
		(*iArrayLen) = pStruMBM->struMBList[iType].iNum;
		if( (*iArrayLen) == 0 )
		{
			goto out;
		}
		ML_CALLOC((*ppulDataArray), unsigned long, (*iArrayLen));
		__m_get_mblist_manage(iType, pStruMBM, (*ppulDataArray));
		
		return ML_OK;
	}

	for( ; i < M_LIST_MAX; i++ )
	{
		(*iArrayLen) += pStruMBM->struMBList[i].iNum;
	}
	if( (*iArrayLen) == 0 )
	{
		goto out;
	}
	ML_CALLOC((*ppulDataArray), unsigned long, (*iArrayLen));
	iSize = 0;
	for( i = 0; i < M_LIST_MAX; i++ )
	{
		__m_get_mblist_manage(i, pStruMBM, &((*ppulDataArray)[iSize]));
		iSize += pStruMBM->struMBList[i].iNum;
	}
	
out:
	return ML_OK;
}


static int
m_add_mblist(
	int iType,
	MBListManage *pStruMBM,
	struct list_head *pStruNew
)
{
	if( !pStruMBM || !pStruNew || iType >= M_LIST_MAX )
	{
		return ML_PARAM_ERR;
	}

	pthread_mutex_lock(&pStruMBM->struMBList[iType].struMutex);
	list_add_tail(pStruNew, &pStruMBM->struMBList[iType].struHead);
	pStruMBM->struMBList[iType].iNum++;
	pthread_mutex_unlock(&pStruMBM->struMBList[iType].struMutex);

	return ML_OK;
}

static int
m_del_mblist(
	int iType,
	MBListManage *pStruMBM,
	struct list_head *pStruNode
)
{
	if( !pStruMBM || !pStruNode || iType >= M_LIST_MAX )
	{
		return ML_PARAM_ERR;
	}

	pthread_mutex_lock(&pStruMBM->struMBList[iType].struMutex);
	if( !list_empty(pStruNode) )
	{
		list_del_init(pStruNode);
		pStruMBM->struMBList[iType].iNum--;
	}
	pthread_mutex_unlock(&pStruMBM->struMBList[iType].struMutex);

	return ML_OK;
}

static void
m_free_mblist(
	int iType,
	MBListManage *pStruMBM,
	void (*pMBListFreeFunc)(struct list_head *pStruNode)
)
{
	struct list_head *pStruHead = NULL;
	struct list_head *pStruNode = NULL;

	if( !pStruMBM || iType >= M_LIST_MAX )
	{
		return;
	}

	pStruHead = &pStruMBM->struMBList[iType].struHead;
	pStruNode = pStruHead->next;

	pthread_mutex_lock(&pStruMBM->struMBList[iType].struMutex);
	while(pStruNode)
	{
		if( pMBListFreeFunc )
		{
			pMBListFreeFunc(pStruNode);
		}
		pStruNode = pStruHead->next;
	}
	pthread_mutex_unlock(&pStruMBM->struMBList[iType].struMutex);
}

static int
m_check_list_empty(
	int iType,
	MBListManage *pStruMBM
)
{
	int iRet = 0;
	if( !pStruMBM || iType >= M_LIST_MAX )
	{
		return ML_PARAM_ERR;
	}

	pthread_mutex_lock(&pStruMBM->struMBList[iType].struMutex);
	if( pStruMBM->struMBList[iType].iNum )
	{
		iRet = ML_ERR;
	}
	else
	{
		iRet = ML_OK;
	}
	pthread_mutex_unlock(&pStruMBM->struMBList[iType].struMutex);

	return iRet;
}

void
m_init_mblist_manage(
	MBListManage *pStruMBM
)
{
	int i = 0;

	for( ; i < M_LIST_MAX; i++ )
	{
		m_init_mblist(&pStruMBM->struMBList[i]);
	}

	pStruMBM->pAddListFunc = m_add_mblist;
	pStruMBM->pDelListFunc = m_del_mblist;
	pStruMBM->pFreeListFunc = m_free_mblist;
	pStruMBM->pGetListFunc  = m_get_mblist_manage;
	pStruMBM->pCheckListEmpty = m_check_list_empty;
}

void 
m_uninit_mblist_manage(
	MBListManage *pStruMBM,
	void (*pMBListFreeFunc)(struct list_head *pStruNode)
)
{
	int i = 0;

	if( !pStruMBM )
	{
		return;
	}

	for( ; i < M_LIST_MAX; i++ )
	{
		pStruMBM->pFreeListFunc(i, pStruMBM, pMBListFreeFunc);
		pthread_mutex_destroy(&pStruMBM->struMBList[i].struMutex);
	}
}


