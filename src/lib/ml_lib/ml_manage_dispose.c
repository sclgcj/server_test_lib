#include "ml_manage_dispose.h"

int 
ml_manager_add_dispose_node(
	struct list_head *pStruNode,
	MLHandle				 struHandle
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_add_dispose_node(pStruNode, pStruML->struDisposeHandle);
}
