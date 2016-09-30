#ifndef TC_RENDEZVOUS_H
#define TC_RENDEZVOUS_H

typedef void* tc_rendezvous_t;
#define TC_RENDEVOUS_ERR (void*)-1

tc_rendezvous_t
tc_rendezvous_create(
	int total_link,
	char *name
);

void
tc_rendezvous_destroy(
	tc_rendezvous_t handle
);

int
tc_rendezvous_set(
	tc_rendezvous_t handle
);



#endif
