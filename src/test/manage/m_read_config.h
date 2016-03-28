#ifndef M_READ_CONFIG_H
#define M_READ_CONFIG_H 1

#include "ml_manage.h"

typedef struct _MConfig
{
	int iDevType;	
	int iIpCount;
	int iTotalLink;
	int iCreateLink;
	int iPortMapCnt;
	unsigned short usEndPort;
	unsigned short usStartPort;
	struct in_addr struStartAddr;
}MConfig, *PMConfig;

enum
{
	M_DEV_TYPE_CLIENT,
	M_DEV_TYPE_SERVER,
	M_DEV_TYPE_MAX
};

void
m_get_config(
	MConfig *pStruConf,
	MLHandle struHandle
);

#endif
