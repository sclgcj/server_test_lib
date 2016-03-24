#include "st_comm.h"
#include "st_manage_recv_check.h"

int
st_manager_add_recv_check(
	void							*pUserData,
	STHandle					struHandle,
	unsigned long			*piRCID
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ST_PARAM_ERR;
	}

	if( !pStruST->struRecvCheckHandle )
	{
		return ST_RECV_CHECK_NOT_INIT;
	}

	return st_add_recv_check(pUserData, pStruST->struRecvCheckHandle, piRCID);
}
