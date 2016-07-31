#ifndef TC_RENDEZVOUS_PRIVATE_H
#define TC_RENDEZVOUS_PRIVATE_H

typedef void* tc_rendezvous_t;
#define RENDEVOUS_ERR (void*)-1

tc_rendezvous_t
tc_rendezvoud_create(
	int total_link,
	char *name
);

void
tc_rendezvous_destroy(
	tc_rendezvous_t handle
);

void
tc_rendezvous_destroy(
	tc_rendezvous_t handle
);

#endif
