#ifndef ST_SEND_H
#define ST_SEND_H 1

int
st_init_recv(
	int iThreadNum
);

int
st_uninit_recv();

int
st_add_send_node(
	PCEventData *pStruED
);

#endif
