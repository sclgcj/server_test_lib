#include <sys/mman.h>
#include "m_error.h"
#include "m_proj.h"

#include "ml_manage_data.h"

/*
 * At preasent, we can obey a truth that we won't operate this very fast.
 */

int
m_create_proj_file(
	int				 iProjNum,
	int				 iClearFile,
	char			 *sResultPath,
	char			 *sProjFilePath,
	MLHandle   struHandle,
	MProjArray *pStruPA
)
{
	int i = 0;
	int iRet = 0;
	char sCmd[512] = { 0 };
	char sProjPath[256] = { 0 };
	MProj struProj;

	if( iClearFile )
	{
		if( sProjFilePath[0] )
		{
			sprintf(sCmd, "rm -rf %s", sProjFilePath);
			system(sCmd);
		}
	}

	if( iProjNum == 0 )
	{
		iProjNum = M_DEFAULT_PROJECT_NUM;
	}

	pStruPA->struHandle = struHandle;
	pStruPA->iProjNum   = iProjNum;
	if( sResultPath && sResultPath[0] != '\0' )
	{
		memcpy(pStruPA->sResultPath, sResultPath, strlen(sResultPath));
	}
	else
	{
		memcpy(pStruPA->sResultPath, M_DEFAULT_RESULT_PATH, strlen(M_DEFAULT_RESULT_PATH));
	}

	if( !sProjFilePath || sProjFilePath[0] == '\0' )
	{
		memcpy(sProjPath, M_DEFAULT_PROJECT_PATH, strlen(M_DEFAULT_PROJECT_PATH));
	}
	else
	{
		memcpy(sProjPath, sProjFilePath, strlen(sProjFilePath));
	}
	ML_ERROR("proj file = %s\n", sProjPath);
	iRet = ml_manager_add_mmap_data(
														sProjPath,
														MAP_PRIVATE,
														iProjNum,
														sizeof(MProj),
														pStruPA->struHandle,
														&pStruPA->iProjID
													);
	if( iRet != ML_OK )
	{
		ML_ERROR("add mmap data error\n");
		return iRet;
	}

	for( ; i < iProjNum; i++ )
	{
		memset(&struProj, 0, sizeof(struProj));
		ml_manager_get_mmap_data(pStruPA->iProjID, i, struHandle, &struProj);
		if( struProj.iInit == 0 )
		{
			break;
		}
		pStruPA->iProjCnt++;
	}

	return ML_OK;
}

static int
m_get_project_id_by_name(
	char       *sName,
	MProjArray *pStruPA,
	int				 *piID	
)
{
	int i = 0;
	int iRet = 0;
	MProj struProj;

	for( ; i< pStruPA->iProjCnt; i++ )
	{
		memset(&struProj, 0, sizeof(struProj));
		ml_manager_get_mmap_data(pStruPA->iProjID, i, pStruPA->struHandle, &struProj);
		ML_CMP_DATA(sName, struProj.sName, iRet);
		if( iRet == ML_OK )
		{
			(*piID) = i;
			return ML_OK;
		}
	}

	return ML_ERR;
}

int
m_get_project_by_id(
	int				 iID,
	MProjArray *pStruPA,
	MProj		   *pStruProj
)
{
	if( !pStruProj || !pStruPA )	
	{
		return ML_PARAM_ERR;
	}
	if( iID >= pStruPA->iProjCnt )
	{
		return ML_PARAM_ERR;
	}

	ml_manager_get_mmap_data(pStruPA->iProjID, iID, pStruPA->struHandle, pStruProj);

	return ML_OK;
}

int
m_set_project_by_id(
	int				 iID,
	MProjArray *pStruPA,
	MProj			 *pStruProj
)
{
	if( !pStruProj || !pStruPA )	
	{
		return ML_PARAM_ERR;
	}
	if( iID >= pStruPA->iProjCnt )
	{
		return ML_PARAM_ERR;
	}

	ml_manager_set_mmap_data(pStruPA->iProjID, iID, pStruProj, pStruPA->struHandle);

	return ML_OK;
}

int
m_get_projct_by_name(
	char			 *sName,
	MProjArray *pStruPA,
	MProj      *pStruProj
)
{
	int iID = 0;
	int iRet = 0;

	if( !pStruPA || !sName )
	{
		return ML_PARAM_ERR;
	}

	iRet = m_get_project_id_by_name(sName, pStruPA, &iID);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	ml_manager_get_mmap_data(pStruPA->iProjID, iID, pStruPA->struHandle, pStruProj);

	return ML_OK;
}

int
m_set_proj_by_name(
	char  *sName,
	MProj *pStruProj,
	MProjArray *pStruPA
)
{
	int iID = 0;
	int iRet = 0;

	iRet = m_get_project_id_by_name(sName, pStruPA, &iID);
	if( iRet != ML_OK )
	{
		return iRet;
	}

	ml_manager_set_mmap_data(pStruPA->iProjID, iID, pStruProj, pStruPA->struHandle);

	return ML_OK;
}

int
m_add_new_proj(
	char *sName,
	MProjArray *pStruPA
)
{
	int iID = 0;
	int iRet = 0;
	MProj struProj;

	if( !pStruPA || !sName )
	{
		return ML_PARAM_ERR;
	}

	if( pStruPA->iProjNum <= pStruPA->iProjCnt )		
	{
		return ML_SPACE_FULL;	
	}

	pStruPA->iProjCnt++;

	memset(&struProj, 0, sizeof(struProj));
	memcpy(struProj.sName, sName, strlen(sName));	

	time(&struProj.tCreateTime);

	return ml_manager_set_mmap_data(pStruPA->iProjID, iID, &struProj, pStruPA->struHandle);
}

int
m_start_proj(
	char			 *sName,
	MProjArray *pStruPA	
)
{
	int iID = 0;
	int iRet = 0;
	MProj struProj;

	iRet = m_get_project_id_by_name(sName, pStruPA, &iID);
	if( iRet != ML_OK )
	{
		return M_NO_SUCH_PROJ;
	}

	memset(&struProj, 0, sizeof(struProj));
	ml_manager_get_mmap_data(pStruPA->iProjID, iID, &struProj, pStruPA->struHandle);
	struProj.iRunCnt++;
	time(&struProj.tCurRunStartTime);
	struProj.tCurRunDurationTime = 0;
	ml_manager_set_mmap_data(pStruPA->iProjID, iID, &struProj, pStruPA->struHandle);

	return ML_OK;
}

int
m_stop_proj(
	char *sName,
	MProjArray *pStruPA
)
{
	int iID = 0;
	int iRet = 0;
	MProj struProj;

	iRet = m_get_project_id_by_name(sName, pStruPA, &iID);
	if( iRet != ML_OK )
	{
		return M_NO_SUCH_PROJ;
	}

	memset(&struProj, 0, sizeof(struProj));
	ml_manager_get_mmap_data(pStruPA->iProjID, iID, &struProj, pStruPA->struHandle);
	time(&struProj.tLastRunEndTime);
	struProj.tLastRunStartTime = struProj.tCurRunStartTime;
	ml_manager_get_mmap_data(pStruPA->iProjID, iID, &struProj, pStruPA->struHandle);

	return ML_OK;
}

void
m_add_proj_node(
	MProj *pStruNew,
	MProj *pStruHead
)
{
	pStruNew->pStruNext  = pStruHead->pStruNext;
	pStruHead->pStruNext = pStruNew;
}

