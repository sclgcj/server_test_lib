#ifndef TC_HUB_H
#define TC_HUB_H 1

/*
 * tc_hub_add() - add a harbor to a link
 * @extra_data:	the data downstream passed to upstreams by extra_data_set function defined by 
 *		upstream
 *
 * Return: 0 if successful, -1 if not
 */
int
tc_hub_add(
	unsigned long extra_data
);

#endif
