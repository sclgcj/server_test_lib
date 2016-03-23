#ifndef ST_RECV_H
#define ST_RECV_H 1

#include "push_client.h"

/*
 *	初始化接收模块
 */
int
st_init_recv( int iThreadNum );

/*
 *	反初始化接收模块
 */
int
st_uninit_recv();

/*
 *	添加接收节点
 */
int
st_add_recv_node(PCEventData *pStruED);
//st_add_recv_node(int iSockfd, void *pData);


#endif
