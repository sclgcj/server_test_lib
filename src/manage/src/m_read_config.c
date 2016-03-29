#include "manage.h"
#include "m_read_config.h"

static void
m_get_dev_type(
	char *sType,
	int  *piDevType
)
{
	int iRet = 0;

	ML_CMP_DATA("server", sType, iRet);
	if( iRet == ML_OK )
	{
		(*piDevType) = M_DEV_TYPE_SERVER;
	}
	else
	{
		(*piDevType) = M_DEV_TYPE_CLIENT;
	}
}

void
ml_manger_get_duration_time(
	char *sDurationTime, 
	int  *piDurationTime 
)
{
	int day, hour, min, sec;

	sscanf(sDurationTime, "%d:%d:%d:%d", &day, &hour, &min, &sec);

	if( hour >= 24 || min >= 60 || sec >= 60 )
	{
		ML_ERROR("Wrong duration time set : %s, set it to default value\n", sDurationTime);
		(*piDurationTime) = ML_DEFAULT_DURATION_TIME;
		return;
	}

	(*piDurationTime) = day * 3600 * 24 + hour * 3600 + min * 60 + sec;
}

void
m_get_config(
	MConfig *pStruConf,
	MLHandle struHandle
)
{
	int iRet = 0;
	char *sTmp = NULL;	

	memset(pStruConf, 0, sizeof(MConfig));
	iRet = ml_manager_read_config_val("dev_type", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		m_get_dev_type(sTmp, &pStruConf->iDevType);
	}
	ML_ERROR("dev_type = %d\n", pStruConf->iDevType);

	iRet = ml_manager_read_config_val("start_ip", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->struStartAddr.s_addr = inet_addr(sTmp);
	}
	{
		struct in_addr struAddr;
		struAddr.s_addr = pStruConf->struStartAddr.s_addr;
		ML_ERROR("start_ip = %s\n", inet_ntoa(struAddr));
	}

	iRet = ml_manager_read_config_val("start_port", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->usStartPort = (unsigned short)atoi(sTmp);
	}
	ML_ERROR("start port = %d\n", pStruConf->usStartPort);

	iRet = ml_manager_read_config_val("end_port", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->usEndPort = (unsigned short)atoi(sTmp);
	}
	ML_ERROR("end port = %d\n", pStruConf->usEndPort);

	iRet = ml_manager_read_config_val("port_map", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->iPortMapCnt = atoi(sTmp);
	}
	ML_ERROR("port_map = %d\n", pStruConf->iPortMapCnt);

	iRet = ml_manager_read_config_val("total_link", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->iTotalLink = atoi(sTmp);
	}
	ML_ERROR("iTotalLink = %d\n", pStruConf->iTotalLink);

	iRet = ml_manager_read_config_val("ip_count", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->iIpCount = atoi(sTmp);
	}
	ML_ERROR("iIpCount = %d\n", pStruConf->iIpCount);

	iRet = ml_manager_read_config_val("server_ip", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->struServerAddr.s_addr = inet_addr(sTmp);
	}
	ML_ERROR("server_ip = %s\n", inet_ntoa(pStruConf->struServerAddr));

	iRet = ml_manager_read_config_val("server_port", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->usServerPort = (unsigned short)atoi(sTmp);
	}
	ML_ERROR("server_port = %d\n", pStruConf->usServerPort);

	iRet = ml_manager_read_config_val("duration_time", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		ml_manger_get_duration_time(sTmp, &pStruConf->iDurationTime);
	}
	ML_ERROR("iDurationTime = %d\n", pStruConf->iDurationTime);

	iRet = ml_manager_read_config_val("client_num", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruConf->iClientNum = atoi(sTmp);
	}
}

