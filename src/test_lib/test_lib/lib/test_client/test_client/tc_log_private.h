#ifndef TC_LOG_PRIVATE_H
#define TC_LOG_PRIVATE_H

typedef void* tc_log_data_t;

tc_log_data_t
tc_log_data_create(
	int total_link
);

int
tc_log_data_destroy(
	tc_log_data_t data
);

/*
 * 此处实现一个用于存放上下文的结构，目的是在用户需要输出信息的时候
 * 可以得知产生该错误的上下文。
 *
 */

/*
 * tc_log_data_start() - start store the log string
 * @direct:	if write to error message to the file directly when call tc_log_write.
 *		1 - directly write, 0 - call tc_log_end to write the log string
 * @id:		the log id, used to distinguish link logs
 * @file:	the file to write the log message
 *
 * This function tells the log system that we need to write something as log. we provide 
 * two way to handle logs: first is to write the log directly to the file(if file is null, 
 * we will write to the stderr); second is to store the log message at first, and write 
 * them when calling tc_log_end() fucntion. 
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_log_data_start(
	tc_log_data_t data,
	int  id,
	char *file
);

/*
 * tc_log_write() - write the log data 
 * @name:	the log name
 * @fmt:	the write format
 * @...:	the variable paremeters
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_log_data_write(
	tc_log_data_t	data,
	int		id,
	char		*fmt,
	va_list		arg_ptr
/*	char *name,
	char *fmt,
	...*/
);

/*
 * tc_log_end() - write out currently stored log message
 * @id:	the log id, used to distinguish link logs
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_log_data_end(
	tc_log_data_t data,
	int id
);

void
tc_log_data_address(
	tc_log_data_t data
);

#endif
