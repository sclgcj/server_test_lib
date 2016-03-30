#ifndef ML_MANAGE_DATA_H
#define ML_MANAGE_DATA_H 1

#include "ml_manage.h"

int
ml_manager_add_mmap_data(
	char		 *sFileName,
	int			 iFlag,
	int			 iTypeSize,
	MLHandle struHandle,
	int			 *piID
);


int
ml_manager_get_mmap_data(
	int			 iID,
	int			 iOffset,
	MLHandle struHandle,
	void		 *pData
);

int
ml_manager_set_mmap_data(
	int			 iID,
	int			 iOffset,
	void		 *pData,
	MLHandle struHandle
);

#endif
