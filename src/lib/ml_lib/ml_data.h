#ifndef ML_DATA_H
#define ML_DATA_H 1

typedef void * MLDataHandle;

void
ml_create_data_handle(
	int iMLDataNum,
	MLDataHandle *pStruHandle
);

void
ml_destroy_special_data(
	int iID,
	MLDataHandle struHandle
);

void
ml_destroy_data_handle(
	MLDataHandle struHandle
);

int
ml_add_mmap_data(
	char				 *sFileName,
	int					 iFlag,
	int					 iDataNum,
	int					 iTypeSize,
	MLDataHandle struHandle,
	int					 *piID
);

int
ml_get_mmap_data(
	int					 iID,
	int					 iOffset,
	MLDataHandle struHandle,
	void				 *pData
);

int
ml_set_mmap_data(
	int					iID,
	int					iOffset,
	void				*pData,
	MLDataHandle struHandle
);

#endif
