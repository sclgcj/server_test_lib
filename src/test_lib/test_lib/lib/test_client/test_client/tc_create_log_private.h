#ifndef TC_CREATE_LOG_PRIVATE_H
#define TC_CREATE_LOG_PRIVATE_H

typedef void * tc_log_t;

tc_log_t
tc_log_create(	
	char *app_proto
);

void
tc_log_destroy(
	unsigned long user_data
);

void
tc_log_address_print(
	tc_log_t *log_data
);

void
tc_log_output_type_get(
	int  type,
	char **name,
	char **color
);

#endif
