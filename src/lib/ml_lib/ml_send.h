#ifndef ML_SEND_H
#define ML_SEND_H 1

#include "ml_thread.h"

typedef void * MLSendHandle;
typedef void (*MLSendFunc)(void *pUserData);

void
ml_create_send_handle(
	int iThreadNum,
	int iStackSize,
	MLSendFunc pFunc,
	MLThreadHandle struThreadHandle,
	MLSendHandle *pStruSendHandle
);

void
ml_destroy_send_handle(
	MLSendHandle struSendHandle
);

int
ml_add_send_node(
	void *pUserData,
	MLSendHandle struSendHandle
);

#endif
