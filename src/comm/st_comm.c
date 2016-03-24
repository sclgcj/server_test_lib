#include "st_comm.h"

void
st_create_comm_node(
	unsigned long ulData,
	void					*pUserData,
	STCommNode    **ppStruCN
)
{
	ST_CALLOC((*ppStruCN), STCommNode, 1);
	(*ppStruCN)->ulData = ulData;
	(*ppStruCN)->pUserData = pUserData;
}

void
st_destroy_comm(
	STCommNode *pStruCN
)
{
	ST_FREE(pStruCN);
}

void
st_destroy_comm_node(
	struct list_head *pStruNode
)
{
	STCommNode *pStruCN = NULL;

	pStruCN = list_entry(pStruNode, STCommNode, struNode);

	ST_FREE(pStruCN);
}

