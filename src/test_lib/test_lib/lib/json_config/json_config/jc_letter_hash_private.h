#ifndef JSON_CONFIG_LETTER_HASH_PRIVATE_H
#define JSON_CONFIG_LETTER_HASH_PRIVATE_H

#include "json_config_private.h"
#include "tc_hash.h"

typedef void * jc_letter_t;

struct jc_letter_param {
	char *name;
	unsigned long user_data;
};

jc_letter_t
jc_letter_create(
		int (*letter_hash)(unsigned long user_data, unsigned long hash_data),
		int (*letter_get)(unsigned long user_data, unsigned long cmp_data),
		int (*letter_destroy)(unsigned long data));

int
jc_letter_add(char *name,
	      unsigned long user_data,
	      jc_letter_t letter);

unsigned long
jc_letter_get(struct jc_letter_param *jlp,
	      jc_letter_t letter);

int
jc_letter_destroy(jc_letter_t letter);

int
jc_letter_traversal(
	unsigned long user_data,
	jc_letter_t letter,
	int (*jl_traversal)(unsigned long jl_data, unsigned long data)
);

#endif
