#include "ml_hub.h"
#include "ml_recv.h"
#include "ml_dispose.h"

#include <sys/epoll.h>

typedef struct MLRecv
{
	int iThreadID;
	MLRecvFunc pRecvFunc;
	MLThreadHandle struThreadHandle;
	MLDisposeHandle struDisposeHandle;
}MLRecv, *PMLRecv;

static int
ml_recv(
	struct list_head *pStruNode
)
{
	int iRet = 0;
	MLRecv *pStruRecv = NULL;
	MLCommNode *pStruCN = NULL;

	pStruCN = list_entry(pStruNode, MLCommNode, struNode);
	pStruRecv = (MLRecv *)pStruCN->ulData;

	if( pStruRecv->pRecvFunc )
	{
		iRet = pStruRecv->pRecvFunc(pStruCN->pUserData, &pStruCN->iRecvLen, &pStruCN->sRecvData);
	}

	if( iRet == ML_OK )
	{
		pStruCN->ulData = 0;
		ml_add_dispose_node(&pStruCN->struNode, pStruRecv->struDisposeHandle);
	}
	
	ML_FREE(pStruCN);

	return ML_OK;
}

int
ml_add_recv_node(
	void *pUserData,
	MLRecvHandle struHandle
)
{
	MLRecv *pStruR = (MLRecv *)struHandle;
	MLCommNode *pStruCN = NULL;

	ML_CALLOC(pStruCN, MLCommNode, 1);
	pStruCN->ulData     = (unsigned long)pStruR;
	pStruCN->pUserData  = pUserData;

	return ml_add_table_thread_pool_node(pStruR->iThreadID, &pStruCN->struNode, pStruR->struThreadHandle);
}

int
ml_create_recv_handle(
	int iThreadNum,
	int iStackSize,
	MLRecvFunc		 pFunc,
	MLThreadHandle struThreadHandle,
	MLDisposeHandle struDisposeHandle,
	MLRecvHandle *pStruHandle	
)
{
	int iRet = 0;
	MLRecv *pStruRecv = NULL;

	ML_CALLOC(pStruRecv, MLRecv, 1);
	pStruRecv->pRecvFunc = pFunc;
	pStruRecv->struThreadHandle = struThreadHandle;
	pStruRecv->struDisposeHandle = struDisposeHandle;

	iRet = ml_create_thread_table_pool(
																iThreadNum,
																iStackSize,
																ml_destroy_comm_node,
																NULL,
																ml_recv,
																struThreadHandle,
																&pStruRecv->iThreadID
															);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	(*pStruHandle) = (MLRecvHandle)pStruRecv;

	return ML_OK;
}

void
ml_destroy_recv(
	MLRecvHandle struRecvHandle
)
{
	MLRecv *pStruRecv = (MLRecv*)struRecvHandle;

	if( !struRecvHandle )
	{
		return;
	}

	ML_FREE(pStruRecv);
}

