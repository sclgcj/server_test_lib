#include "st_hub.h"
#include "st_recv.h"

#include <sys/epoll.h>

typedef struct STRecv
{
	int iThreadID;
	STRecvFunc pRecvFunc;
	STThreadHandle struThreadHandle;
	STuDisposeHandle struDisposeHandle;
}STRecv, *PSTRecv;

static int
st_recv(
	struct list_head *pStruNode
)
{
	STRecv *pStruRecv = NULL;
	STCommNode *pStruCN = NULL;

	pStruCN = list_entry(pStruNode, STCommNode, struNode);
	pStruRecv = (STRecv *)pStruCN->ulData;

	if( pStruRecv->pRecvFunc )
	{
		iRet = pStruRecv->pRecvFunc(pStruCN->pUserData, &pStruCN->iRecvLen, &pStruCN->pRecvData);
	}

	if( iRet == ST_OK )
	{
		st_add_dispose_node(&pStruCN->struNode, pStruRecv->struDisposeHandle);
	}

	ST_FREE(pStruCN);

	return ST_OK;
}

int
st_add_recv_node(
	void *pUserData,
	STRecvHandle struHandle
)
{
	STRecv *pStruR = (STRecv *)struHandle;
	STCommNode *pStruCN = NULL;

	ST_CALLOC(pStruCN, STCommNode, 1);
	pStruCN->ulData     = (unsigned long)pStruR;
	pStruCN->pUserData  = pUserData;

	return st_add_table_thread_pool_node(pStruR->iThreadID, &pStruCN->struNode, pStruR->struThreadHandle);
}

int
st_create_recv_handle(
	int iThreadNum,
	int iStackSize,
	STRecvFunc		 pFunc,
	STThreadHandle struThreadHandle,
	STDisposeHandle struDisposeHandle,
	STRecvHandle *pStruHandle	
)
{
	int iRet = 0;
	STRecv *pStruRecv = NULL;

	ST_CALLOC(pStruRecv, STRecv, 1);
	pStruRecv->pRecvFunc = pFunc;
	pStruRecv->struThreadHandle = struThreadHandle;
	pStruRecv->struDisposeHandle = struDisposeHandle;

	iRet = st_create_thread_table_pool(
																iThreadNum,
																iStackSize,
																st_destroy_comm_node,
																NULL,
																st_recv,
																struThreadHandle,
																&pStruRecv->iThreadID
															);
	if( iRet != ST_OK )
	{
		return iRet;
	}

	(*pStruHandle) = (STRecvHandle)pStruRecv;

	return ST_OK;
}

void
st_destroy_recv(
	STRecvHandle struRecvHandle
)
{
	STRecv *pStruRecv = (STRecv*)struRecvHandle;

	if( !struRecvHandle )
	{
		return;
	}

	ST_FREE(pStruRecv);
}

