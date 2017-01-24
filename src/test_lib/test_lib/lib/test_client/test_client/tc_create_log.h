#ifndef TC_CREATE_LOG_H
#define TC_CREATE_LOG_H  1

#include "tc_log.h"

enum{
	TC_LOG_INFO,		//信息 日志
	TC_LOG_WARN,		//警告 日志
	TC_LOG_DEBUG,		//调试 日志
	TC_LOG_FATAL,		//致命 日志
	TC_LOG_SYSTEM,		//系统 日志
	TC_LOG_MAX
};

enum {
	TC_LOG_WRITE_DIRECTLY,
	TC_LOG_WRITE_INDIRECTLY
};

/*
 * tc_log_start() - start store the log string
 * @user_data:  the upstream data
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
tc_log_start(
	unsigned long user_data,
	char *file
);

/*
 * tc_log_write() - write the log data 
 * @user_data:  the upstream data
 * @name:	the log name
 * @fmt:	the write format
 * @...:	the variable paremeters
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_log_write(
	unsigned long user_data,
	int  direct,
	int  type,
	char *fmt,
	...
);

/*
 * TC_LOG_INFO, TC_LOG_DEBUG, TC_LOG_WARN, TC_LOG_FATAL,TC_LOG_SYSTEM are 
 * used to write each message directly to the log file
 */
#define TC_INFO(d, f, args...)	\
		tc_log_write(d, TC_LOG_WRITE_DIRECTLY, TC_LOG_INFO, \
				"[%s->%s:%d] - "f"\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#define TC_DEBUG(d, f, args...) \
		tc_log_write(d, TC_LOG_WRITE_DIRECTLY, TC_LOG_DEBUG, \
				"[%s->%s:%d] - "f"\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#define TC_WARN(d, f, args...) \
		tc_log_write(d, TC_LOG_WRITE_DIRECTLY, TC_LOG_WARN, \
				"[%s->%s:%d] - "f"\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#define TC_FATAL(d, f, args...)	\
		tc_log_write(d, TC_LOG_WRITE_DIRECTLY, TC_LOG_FATAL, \
				"[%s->%s:%d] - "f"\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#define TC_SYSTEM(d, f, args...) \
		tc_log_write(d, TC_LOG_WRITE_DIRECTLY, TC_LOG_SYSTEM, \
				"[%s->%s:%d] - "f"\n", __FILE__, __FUNCTION__, __LINE__, ##args)


/*
 * tc_log_end() - write out currently stored log message
 * @user_data:  the upstream data
 * @id:	the log id, used to distinguish link logs
 *
 * Return: 0 if successful, -1 if not and errno will be set
 */
int
tc_log_end(
	unsigned long user_data
);

#endif
