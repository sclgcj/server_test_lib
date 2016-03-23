#ifndef ST_HANDLE_H
#define ST_HANDLE_H 1

#include "push_client.h"
#include "list.h"

int 
st_init_handle(int iThreadHandle);

int 
st_add_handle_node( struct list_head *pStruNode );

void
st_handle_node_free(struct list_head *pStruNode );

#endif
