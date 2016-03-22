#ifndef ST_HUB_H
#define ST_HUB_H 1

#include "list.h"

#include "st_timer.h"
#include "st_thread.h"

typedef void * STHubHandle;
typedef void (*STHubFunc)(unsigned long ulData);
typedef void (*STHubHandleFunc)(unsigned long ulData);

int
st_create_hub(
	int iHubNum,
	int iThreadNum,
	int iStackSize,
	STHubFunc pHubFunc,
	STHubHandleFunc pHubHandleFunc,
	STTimerHandle struTHandle,
	STThreadHandle struThreadHandle,
	STHubHandle *pStruHandle
);

void
st_destroy_hub(
	STHubHandle struHandle
);

int
st_add_hub(
	void *pData,
	unsigned long *pulHubID,
	STHubHandle struHandle
);

int
st_del_hub(
	unsigned long ulHubID,
	STHubHandle struHandle
);

#endif
