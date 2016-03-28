#include "ml_comm.h"
#include "ml_manage.h"

static void
ml_epollerr(
	void *pEventData
)
{
}

static void
ml_epollpri(
	void *pEventData
)
{
}

static void
ml_epollhup(
	void *pEventData
)
{
}

static void
ml_epollrdhup(
	void *pEventData
)
{
}

int 
ml_set_listen_op(
	MLListenOp *pStruOp
)
{
	if( !pStruOp )
	{
		return ML_PARAM_ERR;
	}
	pStruOp->pEpollErrFunc = ml_epollerr;
	pStruOp->pEpollPriFunc = ml_epollpri;
	pStruOp->pEpollHupFunc = ml_epollhup;
	pStruOp->pEpollRDHupFunc = ml_epollrdhup;

	return ML_OK;
}
