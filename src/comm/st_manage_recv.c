#include "st_manage_recv.h"

int
st_mamager_add_recv_node(
	void *pUserData,
	STHandle struHandle
)
{
	ServerTest *pStruST = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	if( !pStruST->struRecvHandle )
	{
		return ST_RECV_NOT_INIT;
	}

	return st_add_recv_node(
										pUserData,	
										pStruST->struRecvHandle
									);
}
