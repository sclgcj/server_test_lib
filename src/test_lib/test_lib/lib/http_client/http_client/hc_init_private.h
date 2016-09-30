#ifndef HC_INIT_PRIVATE_H
#define HC_INIT_PRIVATE_H 

int
hc_local_init_register(
	int (*init)()
);

int
hc_local_uninit_register(
	int (*uninit)()
);

int
hc_init();

int
hc_uninit();

#endif
