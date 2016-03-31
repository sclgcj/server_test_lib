#ifndef M_READ_CONFIG_H
#define M_READ_CONFIG_H 1

#include "ml_manage.h"

typedef struct _MConfig
{
	char sProjFilePath[256];
	char sResultPath[256];
	int iDevType;	
	int iIpCount;
	int iClearFile;
	int iTotalLink;
	int iClientNum;
	int iMLDataNum;
	int iProjNum;
	int iCreateLink;
	int iPortMapCnt;
	int iUnixListen;
	int iDurationTime;
	int iHubThreadNum;
	int iRecvThreadNum;
	int iSendThreadNum;
	int iTimerThreadNum;
	int iDisposeThreadNum;
	int iPushDataThreadNum;
	int iRecvCheckThreadNum;
	unsigned short usRes;
	unsigned short usEndPort;
	unsigned short usStartPort;
	unsigned short usServerPort;
	struct in_addr struStartAddr;
	struct in_addr struServerAddr;
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
