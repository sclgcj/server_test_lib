#ifndef ML_COMM_H 
#define ML_COMM_H 1

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "list.h"
#include "ml_error.h"

//some default values
#define ML_DEFAULT_HUB_TABLE_SIZE			150000
#define ML_DEFAULT_THREAD_MLACK_SIZE	32 * 1024
#define ML_DEFAULT_THREAD_NUM					1 
#define ML_DEFAULT_TIMER_NUM					110000
#define ML_DEFAULT_GROUP_NUM					10
#define ML_DEFAULT_CHECK_SIZE					150000
#define ML_DEFAULT_DURATION_TIME      10
#define ML_DEFAULT_CLIENT_NUM					10000
#define ML_DEFAULT_MLDATA_NUM					10
#define ML_DEFAULT_PROJECT_NUM				1000

#if 1
#define ML_ERROR(fmt, args...)  fprintf(stderr, "[%s:%d] - "fmt, __FUNCTION__, __LINE__, ##args)
#define ML_DEBUG(fmt, args...)  //fprintf(stderr, "[%s:%d] - "fmt, __FUNCTION__, __LINE__, ##args);
#endif

//使用calloc分配空间
#define ML_CALLOC(mem, type, num) do{\
	(mem) = (type*)calloc((num), sizeof(type)); \
	if( !(mem) )	 \
	{ \
		ML_ERROR("calloc error: %s\n", strerror(errno)); \
		exit(0); \
	} \
}while(0)

//清空
#define ML_BZERO(mem, type, num) do{ \
	int istb_size = sizeof(type) * num; \
	memset(mem, 0, istb_size); \
}

#define ML_FREE( str ) do{ \
	if( str ) \
	{ \
		free(str); \
		str = NULL; \
	} \
}while(0)

//比较数据
#define ML_CMP_DATA(src, dst, ret) do{ \
	int iLen1 = 0, iLen2 = 0; \
	iLen1 = strlen(src); \
	iLen2 = strlen(dst); \
	if( iLen1 == iLen2 && !strncmp(src, dst, iLen1)) \
	{ \
		ret = ML_OK; \
	} \
	else \
	{\
		ret = ML_ERR;  \
	}\
}while(0)

typedef struct _MLCommNode
{
	unsigned long ulData;
	void					*pUserData;
	int						iRecvLen;
	char					*sRecvData;
	struct list_head struNode;
}MLCommNode, *PMLCommNode;

void
ml_create_comm_node(
	unsigned long ulData,
	void					*pUserData,
	MLCommNode    **ppStruCN
);

void
ml_destroy_comm(MLCommNode *pStruCN);

void
ml_destroy_comm_node(struct list_head *pStruNode);

#endif
