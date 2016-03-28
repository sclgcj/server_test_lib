#include "ml_comm.h"
#include "ml_manage_recv_check.h"

int
ml_manager_add_recv_check(
	void							*pUserData,
	MLHandle					struHandle,
	unsigned long			*piRCID
)
{
	ServerTest *pStruML = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	if( !pStruML->struRecvCheckHandle )
	{
		return ML_RECV_CHECK_NOT_INIT;
	}

	return ml_add_recv_check(pUserData, pStruML->struRecvCheckHandle, piRCID);
}

int
ml_manager_stop_recv_check(
	unsigned long  ulRCID,
	MLHandle			 struHandle
)
{
	ServerTest *pStruST = (ServerTest *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}
	
	return ml_stop_recv_check(ulRCID, pStruST->struRecvCheckHandle);
}

int
ml_manager_start_recv_check(
	unsigned long ulRCID,
	MLHandle      struHandle
)
{
	ServerTest *pStruST = (ServerTest*)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	return ml_start_recv_check(ulRCID, pStruST->struRecvCheckHandle);
}
