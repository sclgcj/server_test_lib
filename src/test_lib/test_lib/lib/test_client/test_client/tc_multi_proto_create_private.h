#ifndef TC_MULTI_PROTO_CREATE_PRIVATE_H
#define TC_MULTI_PROTO_CREATE_PRIVATE_H 1

int
tc_multi_proto_add(
	int  data_size,
	int  data_cnt,
	char *proto
);

void
tc_multi_proto_data_id_get(
	unsigned long user_data,
	int *id
);

void
tc_multi_proto_user_data_get(
	int id,
	char *proto,
	unsigned long *user_data
);

#endif
