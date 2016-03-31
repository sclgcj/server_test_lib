#include "ml_comm.h"
#include "ml_data.h"
#include "ml_file_lock.h"
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct MLData
{
	int  iMapFd;
	int  iMapSize;
	int  iTypeSize;
	void *pMap;
}MLData, *PMLData;

typedef struct _MLDataTable
{
	int iMLDataNum;
	int iMLDataCnt;
	MLData *pStruMD;
}MLDataTable, *PMLDataTable;

void
ml_create_data_handle(
	int iMLDataNum,
	MLDataHandle *pStruHandle
)
{
	MLDataTable *pStruDT = NULL;

	ML_CALLOC(pStruDT, MLDataTable, 1);
	pStruDT->iMLDataNum = iMLDataNum;
	ML_CALLOC(pStruDT->pStruMD, MLData, iMLDataNum);	

	(*pStruHandle) = (MLDataHandle)pStruDT;
}

void
ml_destroy_special_data(
	int iID,
	MLDataHandle struHandle
)
{
	MLDataTable *pStruDT = (MLDataTable *)struHandle;

	if( !struHandle || iID >= pStruDT->iMLDataNum )
	{
		return;
	}

	munmap(pStruDT->pStruMD[iID].pMap, pStruDT->pStruMD[iID].iMapSize);
	close(pStruDT->pStruMD[iID].iMapFd);
}

void
ml_destroy_data_handle(
	MLDataHandle struHandle
)
{
	int i = 0;
	MLDataTable *pStruDT = (MLDataTable *)struHandle;

	if( !struHandle )
	{
		return;
	}

	for( ; i < pStruDT->iMLDataNum; i++ )
	{
		ml_destroy_special_data(i, struHandle);
	}
	
	ML_FREE(pStruDT->pStruMD);
	ML_FREE(pStruDT);
}

static void
m_create_data_file(
	char *sFile,
	int  iDataNum,
	int  iTypeSize
)
{
	int iUnit = 1024 * 1024;
	int iSize = 0;
	char sCmd[512] = { 0 };

	iSize = (iDataNum * iTypeSize) / iUnit;
	if( iSize == 0 )
	{
		iSize = 1;
	}

	sprintf(sCmd, "dd if=/dev/zero of=%s bs=%dM count=0 seek=1", sFile, iSize);

	system(sCmd);
}

int
ml_add_mmap_data(
	char				 *sFileName,
	int					 iFlag,
	int					 iDataNum,
	int					 iTypeSize,
	MLDataHandle struHandle,
	int					 *piID
)
{
	struct stat struBuf;
	MLData *pStruMD = NULL;
	MLDataTable *pStruDT = (MLDataTable *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	if( pStruDT->iMLDataCnt >= pStruDT->iMLDataNum )
	{
		return ML_SPACE_FULL;
	}

	if( access(sFileName, R_OK) )
	{
		m_create_data_file(sFileName, iDataNum, iTypeSize);
	}

	(*piID) = pStruDT->iMLDataCnt++;
	pStruMD = &pStruDT->pStruMD[(*piID)];

	memset(pStruMD, 0, sizeof(MLData));
	memset(&struBuf, 0, sizeof(struBuf));
	stat(sFileName, &struBuf); 

	pStruMD->iTypeSize = iTypeSize;
	pStruMD->iMapSize  = struBuf.st_size;
	pStruMD->iMapFd = open(sFileName, O_RDWR);
	if( pStruMD->iMapFd < 0 )
	{
		ML_ERROR("open file %s error: %s\n", sFileName, strerror(errno));
		exit(0);
	}
	pStruMD->pMap	= mmap(NULL, struBuf.st_size, PROT_READ | PROT_WRITE, iFlag, pStruMD->iMapFd, 0);
	if( pStruMD->pMap == MAP_FAILED )
	{
		ML_ERROR("mmap error: %s\n", strerror(errno));
		close(pStruMD->iMapFd);
		exit(0);
	}

	return ML_OK;
}

int
ml_get_mmap_data(
	int					 iID,
	int					 iOffset,
	MLDataHandle struHandle,
	void				 *pData
)
{
	MLData *pStruMD = NULL;
	MLDataTable *pStruDT = (MLDataTable *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	if( iID >= pStruDT->iMLDataCnt )
	{
		return ML_SPACE_FULL;
	}

	pStruMD = &pStruDT->pStruMD[iID];

	ml_file_read_lock_by_fd(pStruMD->iMapFd, iOffset, pStruMD->iTypeSize);
	memcpy( pData, ((char*)pStruMD->pMap) + iOffset, pStruMD->iTypeSize );
	ml_file_unlock_by_fd(pStruMD->iMapFd, iOffset, pStruMD->iTypeSize); 

	return ML_OK;
}

int
ml_set_mmap_data(
	int					iID,
	int					iOffset,
	void				*pData,
	MLDataHandle struHandle
)
{
	MLData *pStruMD = NULL;
	MLDataTable *pStruDT = (MLDataTable *)struHandle;

	if( !struHandle )
	{
		return ML_PARAM_ERR;
	}

	if( iID >= pStruDT->iMLDataCnt )
	{
		return ML_SPACE_FULL;
	}

	pStruMD = &pStruDT->pStruMD[iID];

	ml_file_read_lock_by_fd(pStruMD->iMapFd, iOffset, pStruMD->iTypeSize);
	memcpy( ((char*)pStruMD->pMap) + iOffset, pData, pStruMD->iTypeSize );
	msync(((char *)pStruMD->pMap) + iOffset, pStruMD->iTypeSize, MS_SYNC);
	ml_file_unlock_by_fd(pStruMD->iMapFd, iOffset, pStruMD->iTypeSize); 

	return ML_OK;
}

