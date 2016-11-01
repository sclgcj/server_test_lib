#ifndef TC_PARAM_MANAGE_PRIVATE_H
#define TC_PARAM_MANAGE_PRIVATE_H

#include "tc_param_manage.h"
#include "tc_hash.h"

// parameter manage operation structure
struct tc_create_link_data;
struct tc_param_oper {
	//All these callback must be implemented
	int (*param_oper)(int oper_cmd, struct tc_param *); 
	void (*param_set)(struct tc_param *new_param, struct tc_param *old_param); 
	void (*param_destroy)(struct tc_param *param);
	char *(*param_value_get)(struct tc_create_link_data *cl_data, struct tc_param *param); 
	struct tc_param *(*param_copy)(struct tc_param *param); 
};

struct tc_param_manage {
	tc_hash_handle_t param_hash;
	tc_hash_handle_t param_type_hash;
	/*
	 * pm_set() - set the parameter's configure
	 * @param_name:  the name of the parameter which has been added to the parameter
	 *		 management with the tc_param_add function
	 * @param:	 the parameter configure needed to set
	 * @md:		the object pointer to struct tc_param_manage
	 *
	 * When upstream changes the parameter's  configure, it should call this function 
	 * to update its configure in the parameter management module.
	 *
	 * Return: 0 if successful or -1 if not and errno will be set
	 */
	int (*pm_set)(char *param_name, struct tc_param *param, 
				struct tc_param_manage *md);
	/*
	 * pm_config_get() - get the parameter's configure
	 * @param_name:	the name of the parameter which has bee added to the parameter
	 *		management with the tc_param_add function
	 * @md:		the object pointer to struct tc_param_manage
	 *
	 * When changing or showing the parameter configure, upstreams should call this 
	 * function to get its current configure. Of course, if they don't care current
	 * configure, they also can ignore this function.
	 *
	 * Return: the pointer of struct tc_param if successful or NULL if not and errno 
	 *	   will be set
	 */
	struct tc_param* (*pm_config_get)(char *param_name, 
					  struct tc_param_manage *md);

	/*
	 * pm_value_get() - get_the parameter's current value
	 * @param_name:	the name of the parameter which has bee added to the parameter
	 *		management with the tc_param_add function
	 * @user_data:  upsteam data, which is been set in prepare_data_get callback
	 * @md:		the object pointer to struct tc_param_manage
	 *
	 * We will automatically set the parameter value as the param going. Of course,
	 * different parameter type will have different paramter value fetching operation.
	 * The introduction of each parameter type will be shown in each parameter type 
	 * document.
	 *
	 * Return: the string pointer to the param value if successful or NULL if not and 
	 *	   errno will be set
	 */
	char *(*pm_value_get)(char *param_name, struct tc_param_manage *md);
	/*
	 * pm_oper() - some operation of the parameter.
	 * @oper_cmd:	the operation command of the parameter.
	 * @param_name:	the name of the parameter which has bee added to the parameter
	 *		management with the tc_param_add function
	 * @md:		the object pointer to struct tc_param_manage
	 * 
	 * We can do some specifical operation on the parameter type. Each 
	 * parameter type may provide its own operation command, and these
	 * will Provide in their own document. 
	 *
	 * Return: 0 if successful, -1 if not and errno will be set.
	 */
	int (*pm_oper)(int oper_cmd, char *param_name, 
			struct tc_param_manage *md);
};

// add parameter type
int
tc_param_type_add(
	char *param_type,
	struct tc_param_oper *oper
);

/*
 * tc_param_manage_add() - add the map of parameter name and parameter type 
 * @param_name:		the name of the parameter
 * @param_type:		the type of the parameter, its value will depend
 *			on the amount of the parameter type implementations.
 *			We will update its content when a new parameter 
 *			type has been implementated. The type we supported 
 *			is belowing:
 *				file
 * @param:		the parameter configure structure of the parameter type
 *
 * This function used to add a parameter whose name is param_name to the 
 * parameter management. If the upstream want to parameterize a parameter, it 
 * should call this function first, then it can call other function to do other
 * operations.
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_param_manage_add(
	char *param_name,
	char *param_type,
	struct tc_param *param
);


/*
 * tc_param_manage_del() - delete the parameter's configure from the parameter management
 * @param_name:	the name of the parameter which has bee added to the parameter
 *		management with the tc_param_add function
 *
 * When upstream doesn't want to use a parameter any more, it can call this 
 * function to delete it
 *
 * Return: 0 if successful or -1 if not and errno will be set
 */
int
tc_param_manage_del(
	char *param_name
);

/*
 * tc_param_manage_list_get() - get the parameter map list 
 * @plist:	pointer to struct tc_param_manage_list which contains 
 *		the number of parameters and an array to store
 *		each paramter's name
 *			struct tc_param_manage_list {
 *				int param_num;
 *				char **param_name;
 *			}
 *
 * This function will return all the paramter names added to the 
 * param_management. Will calloc new space fo param_name, and the 
 * caller should call tc_param_list_free() to free these space
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_param_manage_list_get(struct tc_param_manage_list *plist);

/*
 * tc_param_manage_list_free() - free the struct tc_param_manage_list data
 * @plist:	the pointer to the space needed to be freed
 *
 * Return: no
 */
void
tc_param_manage_list_free(
	struct tc_param_manage_list *plist
);

struct tc_param_manage *
tc_param_manage_create();

void
tc_param_manage_destroy(
	struct tc_param_manage *pm
);

int
tc_param_manage_oper(
	int oper_cmd,
	char *param_name,
	struct tc_param_manage *pm
);

char *
tc_param_manage_value_get(
	char *param_name,
	struct tc_param_manage *pm
);

struct tc_param *
tc_param_manage_config_get(
	char *param_name,
	struct tc_param_manage *pm
);

int
tc_param_manage_set(
	char *param_name,
	struct tc_param *param,
	struct tc_param_manage *pm
);

#endif
