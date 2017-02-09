#include "jc_private.h"
#include "jc_iteration_ctrl_private.h"

struct jc_iteration_ctrl {
	int cur_iteration;
};

static struct jc_iteration_ctrl global_iteration_ctrl;

int
jc_iteration_ctrl_init()
{
	global_iteration_ctrl.cur_iteration = 1;
	return JC_OK;
}

int
jc_iteration_ctrl_uninit()
{
	return JC_OK;
}

void
jc_iteration_ctrl_increase()
{
	global_iteration_ctrl.cur_iteration++;
}

int
jc_iteration_ctrl_get()
{
	return global_iteration_ctrl.cur_iteration;
}
