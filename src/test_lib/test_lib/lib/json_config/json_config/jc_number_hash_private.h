#ifndef JSON_CONFIG_NUMBER_HASH_PRIVATE_H

#define JSON_CONFIG_NUMBER_HASH_PRIVATE_H

#include "tc_hash.h"

typedef void* jc_number_t;

jc_number_t
json_config_number_hash_create(
	int total,
	int (*number_get)(unsigned long user_data, unsigned long cmp_data),
	int (*number_destroy)(unsigned long user_data)
);

int
json_config_number_hash_destroy(
	jc_number_t jnumber
);

int
jc_number_add(
	int id,	
	unsigned long user_data,
	jc_number_t jnumber
);

unsigned long
jc_number_get(
	int id,
	unsigned long cmp_data,
	jc_number_t jnumber
);

int
jc_number_traversal(
	unsigned long user_data,
	jc_number_t jnumber,
	int (*jn_traversal)(unsigned long jn_data, unsigned long data)
);

#endif
