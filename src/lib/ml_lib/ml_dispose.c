#include "ml_comm.h"
#include "ml_hub.h"
#include "ml_dispose.h"

typedef struct _MLDispose
{
	int iThreadID;	
	MLDisposeFunc  pDisposeFunc;
	MLThreadHandle struThreadHandle;
}MLDispose, *PMLDispose;

static int
ml_dispose_data(
	struct list_head *pStruNode
)
{
	MLDispose  *pStruD = NULL;
	MLCommNode *pStruCN = NULL;

	ML_ERROR("\n");
	pStruCN = list_entry(pStruNode, MLCommNode, struNode);
	pStruD  = (MLDispose *)pStruCN->ulData;

	if( pStruD->pDisposeFunc )
	{
	ML_ERROR("\n");
		pStruD->pDisposeFunc(pStruCN->iRecvLen, pStruCN->sRecvData, pStruCN->pUserData);
	}

	ML_ERROR("\n");
	if( pStruCN->sRecvData )
	{
		ML_FREE(pStruCN->sRecvData);
	}
	ML_FREE(pStruCN);
	ML_ERROR("\n");

	return ML_OK;
}

int
ml_add_dispose_node(
	struct list_head *pStruNode,
	MLDisposeHandle  struDisposeHandle
)
{
	MLCommNode *pStruCN = NULL;
	MLDispose *pStruD = (MLDispose *)struDisposeHandle;

	pStruCN = list_entry(pStruNode, MLCommNode, struNode);
	pStruCN->ulData = (unsigned long)pStruD;

	ml_add_table_thread_pool_node(
												pStruD->iThreadID,
												pStruNode, 
												pStruD->struThreadHandle
											);

	return 0;
}

void
ml_create_dispose_handle(
	int iThreadNum,
	int iStackSize,
	MLDisposeFunc pFunc,
	MLThreadHandle   struThreadHandle,
	MLDisposeHandle  *pStruHandle
)
{
	MLDispose *pStruD = NULL;

	ML_CALLOC(pStruD, MLDispose, 1);
	pStruD->pDisposeFunc = pFunc;
	pStruD->struThreadHandle = struThreadHandle;
	
	ml_create_thread_table_pool(
												iThreadNum,
												iStackSize,
												ml_destroy_comm_node,
												NULL,
												ml_dispose_data,
												struThreadHandle,
												&pStruD->iThreadID
											);

	(*pStruHandle) = (MLDisposeHandle)pStruD;
}

