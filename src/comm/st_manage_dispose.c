#include "st_manage_dispose.h"

int 
st_manager_add_dispose_node(
	struct list_head *pStruNode,
	STHandle				 struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	return st_add_dispose_node(pStruNode, pStruST->struDisposeHandle);
}
