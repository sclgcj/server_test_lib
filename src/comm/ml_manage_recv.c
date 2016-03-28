#include "ml_manage_recv.h"

int
ml_mamager_add_recv_node(
	void *pUserData,
	MLHandle struHandle
)
{
	ServerTest *pStruML = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	if( !pStruML->struRecvHandle )
	{
		return ML_RECV_NOT_INIT;
	}

	return ml_add_recv_node(
										pUserData,	
										pStruML->struRecvHandle
									);
}

