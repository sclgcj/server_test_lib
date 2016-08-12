#ifndef TC_LOG_H
#define TC_LOG_H 1

/*
 * 此处实现一个用于存放上下文的结构，目的是在用户需要输出信息的时候
 * 可以得知产生该错误的上下文。
 *
 */

/*
 * tc_log_start() - start store the log string
 * @direct:	if write to error message to the file directly when call tc_log_write.
 *		1 - directly write, 0 - call tc_log_end to write the log string
 * @name:	this log name
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
tc_log_start(
	int  direct,
	char *name,
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
tc_log_write(
	char *name,
	char *fmt,
	...
);

/*
 * tc_log_end() - write out currently stored log message
 * @name:	the log name
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_log_end(
	char *name
);

#endif
