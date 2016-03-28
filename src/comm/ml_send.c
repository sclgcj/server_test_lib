#include "ml_comm.h"
#include "ml_send.h"
#include "ml_thread.h"

typedef struct _MLSend
{
	int iThreadID;	
	MLSendFunc pSendFunc;
	MLThreadHandle struThreadHandle;
}MLSend, *PMLSend;

static int
ml_send(
	struct list_head *pStruNode
)
{
	MLSend  *pStruS = NULL;
	MLCommNode *pStruCN = NULL;

	pStruCN = list_entry(pStruNode, MLCommNode, struNode);
	pStruS = (MLSend *)pStruCN->ulData;

	if( pStruS->pSendFunc )
	{
		pStruS->pSendFunc(pStruCN->pUserData);
	}

	ML_FREE(pStruS);

	return ML_OK;
}

int
ml_add_send_node(
	void *pUserData,
	MLSendHandle struSendHandle
)
{
	MLSend *pStruS = (MLSend*)struSendHandle;
	MLCommNode *pStruCN = NULL;

	if( !struSendHandle )
	{
		return ML_PARAM_ERR;
	}

	ML_CALLOC(pStruCN, MLCommNode, 1);
	pStruCN->ulData = (unsigned long)pStruS;
	pStruCN->pUserData = pUserData;

	ml_add_table_thread_pool_node(pStruS->iThreadID, &pStruCN->struNode, pStruS->struThreadHandle);

	return ML_OK;
}

void
ml_create_send_handle(
	int iThreadNum,
	int iStackSize,
	MLSendFunc pFunc,
	MLThreadHandle struThreadHandle,
	MLSendHandle *pStruSendHandle
)
{
	MLSend *pStruS = NULL;

	ML_CALLOC(pStruS, MLSend, 1);
	pStruS->pSendFunc = pFunc;

	ML_ERROR("iThreadNum = %d\n", iThreadNum);
	ml_create_thread_table_pool(
													iThreadNum,
													iStackSize,
													ml_destroy_comm_node,
													NULL,
													ml_send,
													struThreadHandle,
													&pStruS->iThreadID
												);

	(*pStruSendHandle) = (MLSendHandle)pStruS;
}

void
ml_destroy_send_handle(
	MLSendHandle struSendHandle
)
{
	MLSend *pStruS = (MLSend *)struSendHandle;

	if( !struSendHandle )
	{
		return;
	}

	ML_FREE(pStruS);
}

