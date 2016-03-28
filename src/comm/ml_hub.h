#ifndef ML_HUB_H
#define ML_HUB_H 1

#include "list.h"

#include "ml_timer.h"
#include "ml_thread.h"

typedef void * MLHubHandle;
typedef void (*MLHubFunc)(unsigned long ulData);
typedef void (*MLHubHandleFunc)(unsigned long ulData);

int
ml_create_hub(
	int iHubNum,
	int iThreadNum,
	int iStackSize,
	MLHubFunc pHubFunc,
	MLHubHandleFunc pHubHandleFunc,
	MLTimerHandle struTHandle,
	MLThreadHandle struThreadHandle,
	MLHubHandle *pStruHandle
);

void
ml_destroy_hub(
	MLHubHandle struHandle
);

int
ml_add_hub(
	void *pData,
	unsigned long *pulHubID,
	MLHubHandle struHandle
);

int
ml_del_hub(
	unsigned long ulHubID,
	MLHubHandle struHandle
);

#endif
