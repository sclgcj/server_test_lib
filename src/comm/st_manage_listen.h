#ifndef ST_MANAGE_LISTEN_H
#define ST_MANAGE_LISTEN_H 1

#include "st_manage.h"


//添加需要管理套接字
int
st_manage_add_sockfd(
	int  iEvent,
	int  iSockfd,
	void *pData,
	STHandle struHandle
);

//删除需要管理的套接字
int
st_mamage_del_sockfd(
	int iSockfd,
	STHandle struHandle
);

//改变需管理的套接字状态
int
st_manage_mod_sockfd(
	int iEvent,
	int iSockfd, 
	void *pData,
	STHandle struHandle
);

//启动套接字监听
int
st_manage_start_listener(
	STHandle	struHandle
);



#endif
