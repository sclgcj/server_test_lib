#include "manage.h"
#include "m_handle_opt.h"

void
m_get_opt_config(
	MOptConfig *pStruMOC,
	MLHandle   struHandle
)
{
	int iRet = 0;
	char *sTmp = NULL; 

	memset(pStruMOC, 0, sizeof(MOptConfig));
	iRet = ml_manager_get_opt_val("d", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		pStruMOC->iMakeDaemon = 1;
	}

	iRet = ml_manager_get_opt_val("f", &sTmp, struHandle);
	if( iRet == ML_OK )
	{
		memcpy(pStruMOC->sConfigFile, sTmp, strlen(sTmp));
	}
	ML_ERROR("sCOnfigFile = %s\n", pStruMOC->sConfigFile);
}

