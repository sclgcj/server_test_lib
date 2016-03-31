#include "m_proj.h"

int
m_create_proj_file(
	MBase *pStruMB
)
{
	char sCmd[512] = { 0 };

	if( pStruMB->struConf.iClearFile )
	{
		if( pStruMB->struConf.sProjFilePath[0] )
		{
			sprintf(sCmd, "rm -rf %s", pStruMB->struConf.sProjFilePath);
			system(sCmd);
		}
	}
	
}
