#include "st_comm.h"
#include "st_manage.h"

static void
st_epollerr(
	void *pEventData
)
{
}

static void
st_epollpri(
	void *pEventData
)
{
}

static void
st_epollhup(
	void *pEventData
)
{
}

static void
st_epollrdhup(
	void *pEventData
)
{
}

int 
st_set_listen_op(
	STListenOp *pStruOp
)
{
	if( !pStruOp )
	{
		return ST_PARAM_ERR;
	}
	pStruOp->pEpollErrFunc = st_epollerr;
	pStruOp->pEpollPriFunc = st_epollpri;
	pStruOp->pEpollHupFunc = st_epollhup;
	pStruOp->pEpollRDHupFunc = st_epollrdhup;

	return ST_OK;
}
