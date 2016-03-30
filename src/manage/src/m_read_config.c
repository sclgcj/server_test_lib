#include "manage.h"
#include "m_read_config.h"

typedef  void (*MRCFunc)(char *sVal, unsigned long ulData);

static void
m_get_dev_type(
	char *sType,
	unsigned long ulData
)
{
	int iRet = 0;

	ML_CMP_DATA("server", sType, iRet);
	if( iRet == ML_OK )
	{
		*(int*)ulData = M_DEV_TYPE_SERVER;
	}
	else
	{
		*(int*)ulData = M_DEV_TYPE_CLIENT;
	}
}

static void
ml_manger_get_duration_time(
	char *sDurationTime, 
	unsigned long ulData
)
{
	int day, hour, min, sec;

	sscanf(sDurationTime, "%d:%d:%d:%d", &day, &hour, &min, &sec);

	if( hour >= 24 || min >= 60 || sec >= 60 )
	{
		ML_ERROR("Wrong duration time set : %s, set it to default value\n", sDurationTime);
		*(int*)ulData = ML_DEFAULT_DURATION_TIME;
		return;
	}

	*(int*)ulData = day * 3600 * 24 + hour * 3600 + min * 60 + sec;
}

static void
m_get_config_val(
	char *sName,
	int  iType,
	unsigned long ulData,
	MRCFunc pFunc,
	MLHandle struHandle
)
{
	int iLen = 0;
	int iRet = 0;
	char *sTmp = NULL;

	iRet = ml_manager_read_config_val(sName, &sTmp, struHandle);
	if( iRet != ML_OK )
	{
		return;
	}

	switch(iType)
	{
		case M_CTYPE_INT:
			*(int *)ulData = atoi(sTmp);
			break;
		case M_CTYPE_UINT:
			*(unsigned int *)ulData = (unsigned int)atoi(sTmp);
			break;
		case M_CTYPE_CHAR:
			*(char *)ulData = sTmp[0];
			break;
		case M_CTYPE_UCHAR:
			*(unsigned char *)ulData = (unsigned char)sTmp[0];
			break;
		case M_CTYPE_SHORT:
			*(short *)ulData = (short)atoi(sTmp);
			break;
		case M_CTYPE_USHORT:
			*(unsigned short *)ulData = (unsigned short)atoi(sTmp);
			break;
		case M_CTYPE_STRING:
			iLen = strlen(sTmp);
			memcpy((void*)ulData, sTmp, iLen);
			break;
		case M_CTYPE_IPADDR:
			*(unsigned int*)ulData = inet_addr(sTmp);
			break;
		case M_CTYPE_FUNC:
			if( pFunc )
			{
				pFunc(sTmp, ulData);
			}
			break;
	}
}

static void
m_config_print( 
	MConfig *pStruConf 
)
{
	ML_ERROR("dev_type = %d\n", pStruConf->iDevType);
	ML_ERROR("start_ip = %s\n", inet_ntoa(pStruConf->struStartAddr));
	ML_ERROR("start port = %d\n", pStruConf->usStartPort);
	ML_ERROR("end port = %d\n", pStruConf->usEndPort);
	ML_ERROR("port_map = %d\n", pStruConf->iPortMapCnt);
	ML_ERROR("iTotalLink = %d\n", pStruConf->iTotalLink);
	ML_ERROR("iIpCount = %d\n", pStruConf->iIpCount);
	ML_ERROR("server_ip = %s\n", inet_ntoa(pStruConf->struServerAddr));
	ML_ERROR("server_port = %d\n", pStruConf->usServerPort);
	ML_ERROR("iDurationTime = %d\n", pStruConf->iDurationTime);
	ML_ERROR("client_num = %d\n", pStruConf->iClientNum);
	ML_ERROR("unix_listen = %d\n", pStruConf->iUnixListen);
	ML_ERROR("hub_thread_num = %d\n", pStruConf->iHubThreadNum);
	ML_ERROR("recv_thread_num = %d\n", pStruConf->iRecvThreadNum);
	ML_ERROR("send_thread_num = %d\n", pStruConf->iSendThreadNum);
	ML_ERROR("timer_thread_num = %d\n", pStruConf->iTimerThreadNum);
	ML_ERROR("dispose_thread_num = %d\n", pStruConf->iDisposeThreadNum);
	ML_ERROR("push_data_thread_num = %d\n", pStruConf->iPushDataThreadNum);
	ML_ERROR("recv_check_thread_num = %d\n", pStruConf->iRecvCheckThreadNum);
}

void 
m_get_config(
	MConfig *pStruConf,
	MLHandle struHandle
)
{
	memset(pStruConf, 0, sizeof(MConfig));
	
	m_get_config_val("dev_type", M_CTYPE_FUNC, (unsigned long)&pStruConf->iDevType, m_get_dev_type, struHandle);
	m_get_config_val("start_ip", M_CTYPE_IPADDR, (unsigned long)&pStruConf->struStartAddr.s_addr, NULL, struHandle);
	m_get_config_val("start_port", M_CTYPE_USHORT, (unsigned long)&pStruConf->usStartPort, NULL, struHandle);
	m_get_config_val("end_port", M_CTYPE_USHORT, (unsigned long)&pStruConf->usEndPort, NULL, struHandle);
	m_get_config_val("port_map", M_CTYPE_INT, (unsigned long)&pStruConf->iPortMapCnt, NULL, struHandle);
	m_get_config_val("total_link", M_CTYPE_INT, (unsigned long)&pStruConf->iTotalLink, NULL,struHandle);
	m_get_config_val("ip_count", M_CTYPE_INT, (unsigned long)&pStruConf->iIpCount, NULL, struHandle);
	m_get_config_val("server_ip", M_CTYPE_IPADDR, (unsigned long)&pStruConf->struServerAddr.s_addr, NULL, struHandle);
	m_get_config_val("server_port", M_CTYPE_USHORT, (unsigned long)&pStruConf->usServerPort, NULL, struHandle);
	m_get_config_val("client_num", M_CTYPE_INT, (unsigned long)&pStruConf->iClientNum, NULL, struHandle);
	m_get_config_val("unix_listen", M_CTYPE_INT, (unsigned long)&pStruConf->iUnixListen, NULL, struHandle);
	m_get_config_val("duration_time", M_CTYPE_FUNC, (unsigned long)&pStruConf->iDurationTime, ml_manger_get_duration_time, struHandle);
	m_get_config_val("hub_thread_num", M_CTYPE_INT, (unsigned long)&pStruConf->iHubThreadNum, NULL, struHandle);
	m_get_config_val("recv_thread_num", M_CTYPE_INT, (unsigned long)&pStruConf->iRecvThreadNum, NULL, struHandle);
	m_get_config_val("send_thread_num", M_CTYPE_INT, (unsigned long)&pStruConf->iSendThreadNum, NULL, struHandle);
	m_get_config_val("timer_thread_num", M_CTYPE_INT, (unsigned long)&pStruConf->iTimerThreadNum, NULL, struHandle);
	m_get_config_val("dispose_thread_num", M_CTYPE_INT, (unsigned long)&pStruConf->iDisposeThreadNum, NULL, struHandle);
	m_get_config_val("push_data_thread_num", M_CTYPE_INT, (unsigned long)&pStruConf->iPushDataThreadNum, NULL, struHandle);
	m_get_config_val("recv_check_thread_num", M_CTYPE_INT, (unsigned long)&pStruConf->iRecvCheckThreadNum, NULL, struHandle);
	m_get_config_val("ml_data", M_CTYPE_INT, (unsigned long)&pStruConf->iMLDataNum, NULL, struHandle);
	m_get_config_val("proj_num", M_CTYPE_INT, (unsigned long)&pStruConf->iProjectNum, NULL, struHandle);
	m_get_config_val("proj_file", M_CTYPE_STRING, (unsigned long)pStruConf->sProjFilePath, NULL, struHandle);
	m_get_config_val("clear_file", M_CTYPE_INT, (unsigned long)&pStruConf->iClearFile, NULL, struHandle);
	m_get_config_val("result_file", M_CTYPE_STRING, (unsigned long)pStruConf->sResultPath, NULL, struHandle);

	m_config_print(pStruConf);
}

