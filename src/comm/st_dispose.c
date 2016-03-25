#include "st_comm.h"
#include "st_hub.h"
#include "st_dispose.h"

typedef struct _STDispose
{
	int iThreadID;	
	STDisposeFunc  pDisposeFunc;
	STThreadHandle struThreadHandle;
}STDispose, *PSTDispose;

static int
st_dispose_data(
	struct list_head *pStruNode
)
{
	STDispose  *pStruD = NULL;
	STCommNode *pStruCN = NULL;

	pStruCN = list_entry(pStruNode, STCommNode, struNode);
	pStruD  = (STDispose *)pStruCN->ulData;
	
	if( pStruD->pDisposeFunc )
	{
		pStruD->pDisposeFunc(pStruCN->iRecvLen, pStruCN->sRecvData, pStruCN->pUserData);
	}

	if( pStruCN->sRecvData )
	{
		ST_FREE(pStruCN->sRecvData);
	}
	ST_FREE(pStruCN);

	return ST_OK;
}

int
st_add_dispose_node(
	struct list_head *pStruNode,
	STDisposeHandle  struDisposeHandle
)
{
	STCommNode *pStruCN = NULL;
	STDispose *pStruD = (STDispose *)struDisposeHandle;

	pStruCN = list_entry(pStruNode, STCommNode, struNode);
	pStruCN->ulData = (unsigned long)pStruD;

	st_add_table_thread_pool_node(
												pStruD->iThreadID,
												pStruNode, 
												pStruD->struThreadHandle
											);

	return 0;
}

void
st_create_dispose_handle(
	int iThreadNum,
	int iStackSize,
	STDisposeFunc pFunc,
	STThreadHandle   struThreadHandle,
	STDisposeHandle  *pStruHandle
)
{
	STDispose *pStruD = NULL;

	ST_CALLOC(pStruD, STDispose, 1);
	pStruD->pDisposeFunc = pFunc;
	pStruD->struThreadHandle = struThreadHandle;
	
	st_create_thread_table_pool(
												iThreadNum,
												iStackSize,
												st_destroy_comm_node,
												NULL,
												st_dispose_data,
												struThreadHandle,
												&pStruD->iThreadID
											);

	(*pStruHandle) = (STDisposeHandle)pStruD;
}

