#ifndef ML_MANAGE_RECV_CHECK_H
#define ML_MANAGE_RECV_CHECK_H 1

#include "ml_manage.h"

int
ml_manager_add_recv_check(
	void							*pUserData,
	MLHandle					struHandle,
	unsigned long			*piRCID
);

int
ml_manager_start_recv_check(
	unsigned long ulRCID,
	MLHandle      struHandle
);

int
ml_manager_stop_recv_check(
	unsigned long  ulRCID,
	MLHandle			 struHandle
);

#endif
