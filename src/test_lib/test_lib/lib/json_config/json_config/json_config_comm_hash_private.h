#include "json_config_private.h"
#include "json_config_comm_hash_private.h"

#include "tc_hash.h"

#if 0
struct jc_comm_hash_node {
	int content_size; 
	unsigned long content;
	unsigned long user_data;
	struct hlist_node node;
};

struct jc_comm_hash {
	char *name;
	tc_hash_handle_t  handle;
	struct hlist_node node;
};

tc_hash_handle_t
jc_comm_hash_create(
	char *hash_name;
	int (*user_destroy)(unsigned long user_data)
)
{
}

int
jc_comm_hash_add(
	char *hash_name,
	unsigned long user_data
)
{
	
}


int
jc_comm_hash_get(
	char *hash_name,

)
{
}
#endif
