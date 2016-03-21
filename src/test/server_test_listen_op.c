#include "server_test_comm.h"
#include "server_test_manage.h"

static void 
server_test_epollin(
	void *pEventData
)
{
}

static void
server_test_epollout(
	void *pEventData
)
{
}

static void
server_test_epollerr(
	void *pEventData
)
{
}

static void
server_test_epollpri(
	void *pEventData
)
{
}

static void
server_test_epollhup(
	void *pEventData
)
{
}

static void
server_test_epollrdhup(
	void *pEventData
)
{
}

int 
server_test_set_listen_op(
	STListenOp *pStruOp
)
{
	if( !pStruOp )
	{
		return SERVER_TEST_PARAM_ERR;
	}
	pStruOp->pEpollInFunc  = server_test_epollin;
	pStruOp->pEpollOutFunc = server_test_epollout;
	pStruOp->pEpollErrFunc = server_test_epollerr;
	pStruOp->pEpollPriFunc = server_test_epollpri;
	pStruOp->pEpollHupFunc = server_test_epollhup;
	pStruOp->pEpollRDHupFunc = server_test_epollrdhup;

	return SERVER_TEST_OK;
}
