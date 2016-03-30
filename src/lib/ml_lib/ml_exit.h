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

void
ml_set_exit();

#endif


