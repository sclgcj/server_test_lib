#ifndef ST_COMM_H 
#define ST_COMM_H 1

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

enum
{
	ST_OK,
	ST_ERR,
	ST_PARAM_ERR,
	ST_TIMEOUT,
	ST_MAX
};

#if 1
#define ST_ERROR(fmt, args...)  fprintf(stderr, "[%s:%d] - "fmt, __FUNCTION__, __LINE__, ##args)
#define ST_DEBUG(fmt, args...)  //fprintf(stderr, "[%s:%d] - "fmt, __FUNCTION__, __LINE__, ##args);
#endif

//使用calloc分配空间
#define ST_CALLOC(mem, type, num) do{\
	(mem) = (type*)calloc((num), sizeof(type)); \
	if( !(mem) )	 \
	{ \
		ST_ERROR("calloc error: %s\n", strerror(errno)); \
		exit(0); \
	} \
}while(0)

//清空
#define ST_BZERO(mem, type, num) do{ \
	int istb_size = sizeof(type) * num; \
	memset(mem, 0, istb_size); \
}

#define ST_FREE( str ) do{ \
	if( str ) \
	{ \
		free(str); \
		str = NULL; \
	} \
}while(0)

//比较数据
#define ST_CMP_DATA(src, dst, ret) do{ \
	int iLen1 = 0, iLen2 = 0; \
	iLen1 = strlen(src); \
	iLen2 = strlen(dst); \
	if( iLen1 == iLen2 && !strncmp(src, dst, iLen1)) \
	{ \
		ret = ST_OK; \
	} \
	else \
	{\
		ret = ST_ERR;  \
	}\
}while(0)


typedef struct _STCommNode
{
	unsigned long ulData;
	struct list_head struNode;
}STCommNode, *PSTCommNode;

#endif
