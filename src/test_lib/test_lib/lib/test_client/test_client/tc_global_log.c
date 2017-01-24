#include "tc_global_log_private.h"
#include "tc_comm.h"
#include "tc_init.h"
#include "tc_create_log.h"
#include "tc_create_log_private.h"
#include "tc_log_private.h"
#include <stdarg.h>

struct tc_global_log {
	int open_log;
	int global_info_log;
	int global_warn_log;
	int global_debug_log;
	int global_fatal_log;
	int global_system_log;
	int terminal_output;
	int head_set;
	tc_log_data_t log;
};

static struct tc_global_log global_log;

#define TC_LOG_CHECK2(l, memb, type) \
	case type: \
		if (l.memb == 0) \
			return TC_ERR; 

static int
tc_global_log_open(
	int		type	
)
{
	if (global_log.open_log == 0)
		return TC_ERR;

	switch (type) {
		TC_LOG_CHECK2(global_log, global_info_log, TC_LOG_INFO);
		TC_LOG_CHECK2(global_log, global_warn_log, TC_LOG_WARN);
		TC_LOG_CHECK2(global_log, global_debug_log, TC_LOG_DEBUG);
		TC_LOG_CHECK2(global_log, global_fatal_log, TC_LOG_FATAL);
		TC_LOG_CHECK2(global_log, global_system_log, TC_LOG_SYSTEM);
		default:
			return TC_ERR;
	}

	return TC_OK;
}

static int
tc_global_log_head_set(
	char *fmt,
	...
)
{
	int ret = 0;
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	ret = tc_log_data_write(
			global_log.log, 
			0,
			fmt,
			arg_ptr);
	va_end(arg_ptr);

	return ret;
}

int
tc_global_log_write(
	int type,
	char *fmt,
	...
)
{
	int ret = 0;
	char *name = NULL, *color = NULL;
	time_t t;
	struct tm *tm = NULL;
	va_list arg_ptr;

	ret = tc_global_log_open(type);
	if (ret != TC_OK)
		return  TC_OK;

	tc_log_output_type_get(type, &name, &color);	
	if (ret != TC_OK)
		return ret;

	if (global_log.head_set) {
		time(&t);
		tm = localtime(&t);
		if (global_log.terminal_output == 1) {
			ret = tc_global_log_head_set("%s[%s]\e[0m][%d-%d-%d %d:%d:%d]:", 
				color, name,
				tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, 
				tm->tm_hour, tm->tm_min, tm->tm_sec);
		} else {
			ret = tc_global_log_head_set( "[%s][%d-%d-%d %d:%d:%d]:", 
					name,
					tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, 
					tm->tm_hour, tm->tm_min, tm->tm_sec);
		}
	}
	va_start(arg_ptr, fmt);
	ret = tc_log_data_write(
			global_log.log,
			0,
			fmt, 
			arg_ptr);
	va_end(arg_ptr);
	if (ret != TC_OK)
		return ret;

	return tc_log_data_end(
			global_log.log, 
			0);
}

static int
tc_global_log_setup()
{
	int total_link = 0;
	cJSON *obj = NULL;
	char file[256] = { 0 };

	obj = tc_config_read_get("general");

	CR_INT(obj, "total_link", total_link);
	CR_USHORT(obj, "global_open_log", global_log.open_log);
	if (global_log.open_log == 0)
		return TC_OK;
	CR_USHORT(obj, "global_info_log", global_log.global_info_log);
	CR_USHORT(obj, "global_debug_log", global_log.global_debug_log);
	CR_USHORT(obj, "global_fatal_log", global_log.global_fatal_log);
	CR_USHORT(obj, "global_system_log", global_log.global_system_log);
	CR_USHORT(obj, "global_warning_log", global_log.global_warn_log);
	CR_STR(obj, "global_log_file", file);

	global_log.log = tc_log_data_create(1);
	if (!global_log.log) 
		TC_ERR;

	if (!file[0])
		global_log.terminal_output = 1;

	return  tc_log_data_start(global_log.log, 0, file);
}

static int
tc_global_log_uninit()
{
	tc_log_data_destroy(global_log.log);

	return TC_OK;
}

int
tc_global_log_init()
{
	int ret = 0;

	ret = tc_init_register(tc_global_log_setup);
	if (ret != TC_OK)
		return ret;
	return tc_uninit_register(tc_global_log_uninit);
}

TC_MOD_INIT(tc_global_log_init);

