#include "ml_comm.h"

void
ml_create_comm_node(
	unsigned long ulData,
	void					*pUserData,
	MLCommNode    **ppStruCN
)
{
	ML_CALLOC((*ppStruCN), MLCommNode, 1);
	(*ppStruCN)->ulData = ulData;
	(*ppStruCN)->pUserData = pUserData;
}

void
ml_destroy_comm(
	MLCommNode *pStruCN
)
{
	ML_FREE(pStruCN);
}

void
ml_destroy_comm_node(
	struct list_head *pStruNode
)
{
	MLCommNode *pStruCN = NULL;

	pStruCN = list_entry(pStruNode, MLCommNode, struNode);

	ML_FREE(pStruCN);
}

