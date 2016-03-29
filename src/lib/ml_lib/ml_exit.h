#ifndef ML_EXIT_H
#define ML_EXIT_H 1

typedef void* MLExitHandle;

void
ml_create_exit_handle(
	int iDurationTime,
	MLExitHandle *pStruHandle
);

void
ml_destroy_exit_handle(
	MLExitHandle struHandle
);

int
ml_check_exit(
	MLExitHandle struHandle
);

int
ml_check_exit_duration(
	int					 iTick,
	MLExitHandle struHandle
);

//该函数,目前没什么用,可以不调用
int 
ml_init_exit();

int
ml_set_exit();

int
ml_check_exit();

#endif


