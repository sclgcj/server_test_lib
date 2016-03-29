#ifndef M_CLIENT_DATA_H
#define M_CLIENT_DATA_H 1

typedef void * MLDataHandle;

void
ml_create_client_data(
	int iClientNum,
	MLDataHandle *pStruDHandle
);

void
ml_destroy_client_data(
	MLDataHandle struHandle
);

int
ml_add_client_data(
	void			   *pUserData,
	MLDataHandle struHandle,
	int					 *piID
);

int
ml_get_client_data(
	int					 iID,
	MLDataHandle struHandle,
	void				 **pUserData
);

#endif
