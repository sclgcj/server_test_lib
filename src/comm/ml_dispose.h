#ifndef ML_HANDLE_H
#define ML_HANDLE_H 1

#include "list.h"

typedef void * MLDisposeHandle;
typedef void (*MLDisposeFunc)(int iRecvLen, char *sRecvData, void *pUserData);

int 
ml_init_handle(int iThreadHandle);

int 
ml_add_handle_node( struct list_head *pStruNode );

void
ml_handle_node_free(struct list_head *pStruNode );

#endif
