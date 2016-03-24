#ifndef ST_MANAGE_RECV_CHECK_H
#define ST_MANAGE_RECV_CHECK_H 1

#include "st_manage.h"

int
st_manager_add_recv_check(
	void							*pUserData,
	STRecvCheckHandle struHandle,
	unsigned long			*piRCID
);

#endif
