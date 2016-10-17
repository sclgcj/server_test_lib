#ifndef HC_INTERFACE_PRIVATE_H
#define HC_INTERFACE_PRIVATE_H 1

#include "http_client.h"
#include "hc_config_private.h"

struct hc_create_link_data {
	int total_len;
	int is_chunked;
	int first_recv;
	int circle_cnt;
	int  recv_cnt;
	char *recv_data;
	pthread_cond_t interface_cond;
	pthread_mutex_t interface_mutex;
	pthread_mutex_t data_mutex;
	struct hc_oper *oper;
	char data[0];
};

int
hc_interface_init();

int
hc_interface_destroy();

int
hc_interface_func_execute(
	struct hc_create_link_data *cl_data
);

/*
cJSON *
hc_interface_param_get(
	char *interface
);

void
hc_interface_param_set(
	char *name
);
*/

#endif
