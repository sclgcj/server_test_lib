#ifndef TC_PARAM_MANAGE_H
#define TC_PARAM_MANAGE_H

struct tc_param {
	char data[0];
};


struct tc_param_list {
	int param_num;
	char **param_name;
};

/*
 * tc_param_add() - add the map of parameter name and parameter type 
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
tc_param_add(
	char *param_name,
	char *param_type,
	struct tc_param *param
);

/*
 * tc_param_set() - set the parameter's configure
 * @param_name:  the name of the parameter which has been added to the parameter
 *		 management with the tc_param_add function
 * @param:	 the parameter configure needed to set
 *
 * When upstream changes the parameter's  configure, it should call this function 
 * to update its configure in the parameter management module.
 *
 * Return: 0 if successful or -1 if not and errno will be set
 */
int 
tc_param_set(
	char *param_name,
	struct tc_param *param
);

/*
 * tc_param_del() - delete the parameter's configure from the parameter management
 * @param_name:	the name of the parameter which has bee added to the parameter
 *		management with the tc_param_add function
 *
 * When upstream doesn't want to use a parameter any more, it can call this 
 * function to delete it
 *
 * Return: 0 if successful or -1 if not and errno will be set
 */
int
tc_param_del(
	char *param_name
);

/*
 * tc_param_config_get() - get the parameter's configure
 * @param_name:	the name of the parameter which has bee added to the parameter
 *		management with the tc_param_add function
 *
 * When changing or showing the parameter configure, upstreams should call this 
 * function to get its current configure. Of course, if they don't care current
 * configure, they also can ignore this function.
 *
 * Return: the pointer of struct tc_param if successful or NULL if not and errno 
 *	   will be set
 */
struct tc_param *
tc_param_config_get(
	char *param_name
);

/*
 * tc_param_value_get() - get_the parameter's current value
 * @param_name:	the name of the parameter which has bee added to the parameter
 *		management with the tc_param_add function
 *
 * We will automatically set the parameter value as the param going. Of course,
 * different parameter type will have different paramter value fetching operation.
 * The introduction of each parameter type will be shown in each parameter type 
 * document.
 *
 * Return: the string pointer to the param value if successful or NULL if not and 
 *	   errno will be set
 */
char *
tc_param_value_get(
	char *param_name
);

/*
 * tc_param_oper() - some operation of the parameter.
 * @oper_cmd:	the operation command of the parameter.
 * @param_name:	the name of the parameter which has bee added to the parameter
 *		management with the tc_param_add function
 * 
 * We can do some specified operation on the parameter type. Each 
 * parameter type may provide its own operation command, and these
 * will Provide in their own document. 
 *
 * Return: 0 if successful, -1 if not and errno will be set.
 */
int
tc_param_oper(
	int  oper_cmd,
	char *param_name
);

/*
 * tc_param_list_get() - get the parameter map list 
 * @plist:	pointer to struct tc_param_list which contains 
 *		the number of parameters and an array to store
 *		each paramter's name
 *			struct tc_param_list {
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
tc_param_list_get(struct tc_param_list *plist);

/*
 * tc_param_list_free() - free the struct tc_param_list data
 * @plist:	the pointer to the space needed to be freed
 *
 * Return: no
 */
void
tc_param_list_free(
	struct tc_param_list *plist
);

#endif
