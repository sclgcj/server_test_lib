#include "manage.h"
#include "m_handle_opt.h"

typedef void (*MHandleOptFunc)(char *sVal, unsigned long ulData);

static void
m_get_opt_config_val(
	char *sName,
	int  iType,
	unsigned long ulData,
	MHandleOptFunc pFunc,
	MLHandle struHandle
)
{
	int iLen = 0;
	int iRet = 0;
	char *sTmp = NULL;

	iRet = ml_manager_get_opt_val(sName, &sTmp, struHandle);
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
m_get_daemon(
	char *sVal,
	unsigned long ulData
)
{
	*(int*)ulData = 1;
}

void
m_get_opt_config( 
	MOptConfig *pStruMOC, 
	MLHandle   struHandle
)
{
	int iRet = 0;
	char *sTmp = NULL; 

	memset(pStruMOC, 0, sizeof(MOptConfig));

	m_get_opt_config_val("d", M_CTYPE_FUNC, (unsigned long)&pStruMOC->iMakeDaemon, m_get_daemon, struHandle);
	m_get_opt_config_val("f", M_CTYPE_STRING, (unsigned long)pStruMOC->sConfigFile, NULL, struHandle);

	ML_ERROR("iDaemon = %d\n", pStruMOC->iMakeDaemon);
	ML_ERROR("sCOnfigFile = %s\n", pStruMOC->sConfigFile);
}

void
m_handle_opt(
	MOptConfig *pStruMOC
)
{
	if( pStruMOC->iMakeDaemon == 1 )
	{
		daemon(1, 1);
	}
}

