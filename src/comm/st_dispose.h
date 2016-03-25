#ifndef ST_HANDLE_H
#define ST_HANDLE_H 1

#include "list.h"

typedef void * STDisposeHandle;
typedef void (*STDisposeFunc)(int iRecvLen, char *sRecvData, void *pUserData);

int 
st_init_handle(int iThreadHandle);

int 
st_add_handle_node( struct list_head *pStruNode );

void
st_handle_node_free(struct list_head *pStruNode );

#endif
