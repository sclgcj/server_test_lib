#ifndef ML_MANAGE_LIMLEN_H
#define ML_MANAGE_LIMLEN_H 1

#include "ml_manage.h"


//添加需要管理套接字
int
ml_manager_add_sockfd(
	int  iEvent,
	int  iSockfd,
	void *pData,
	MLHandle struHandle
);

//删除需要管理的套接字
int
ml_manager_del_sockfd(
	int iSockfd,
	MLHandle struHandle
);

//改变需管理的套接字状态
int
ml_manager_mod_sockfd(
	int iEvent,
	int iSockfd, 
	void *pData,
	MLHandle struHandle
);

//启动套接字监听
int
ml_manager_start_listener(
	MLHandle	struHandle
);



#endif
