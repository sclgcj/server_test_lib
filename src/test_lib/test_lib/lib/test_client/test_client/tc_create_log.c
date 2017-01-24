#include "tc_create_log.h"
#include "tc_comm.h"
#include "tc_create_private.h"
#include "tc_log_private.h"
#include "tc_create_log_private.h"
#include <stdarg.h>


#define NONE		"\e[0m"
#define BLACK		"\e[0;30m"
#define L_BLACK		"\e[1;30m"
#define RED		"\e[0;31m"
#define L_RED		"\e[1,31m"
#define GREEN		"\e[0;32m"
#define L_GREEN		"\e[1,32m"
#define BROWN		"\e[0,33m"

struct tc_log {
	unsigned short  open_log;	//开启日志
	unsigned short  open_all_log;	//打开所有日志
	unsigned short  open_info_log;	//开启info日志
	unsigned short  open_debug_log;	//开启debug日志
	unsigned short  open_fatal_log;	//开启fatal(致命错误)日志
	unsigned short  open_system_log;	//开启系统日志
	unsigned short  open_warning_log;	//开启警告日志
	unsigned short  terminal_output;	//终端输出
	unsigned short	head_set;		//是否设置了头，一次输出多个打印
	tc_log_data_t   log_data;
};

struct tc_log_print{
	char *name;
	char *color;
};

static struct tc_log_print global_log_print[] = {
	{"info", RED},
	{"warning", L_RED},
	{"debug", GREEN},
	{"fatal", L_GREEN},
	{"system", BROWN},
	{"default", NONE}
};

void
tc_log_output_type_get(
	int  type,
	char **name,
	char **color
)
{
	if (type >= TC_LOG_MAX) 
		type  = TC_LOG_MAX;

	*name = global_log_print[type].name;
	*color = global_log_print[type].color;
}

#define TC_LOG_CHECK(l, memb, type) \
	case type: \
		if (l->memb == 0) { \
			PRINT("type = %d\n", type);\
			return TC_ERR;  \
		}\
		break;

static int
tc_log_open(
	tc_log_t	data,	
	int		type	
)
{
	struct tc_log *log_data = NULL;

	log_data = (struct tc_log*)data;

	if (log_data->open_log == 0)
		return TC_ERR;

	switch (type) {
		TC_LOG_CHECK(log_data, open_info_log, TC_LOG_INFO);
		TC_LOG_CHECK(log_data, open_warning_log, TC_LOG_WARN);
		TC_LOG_CHECK(log_data, open_debug_log, TC_LOG_DEBUG);
		TC_LOG_CHECK(log_data, open_fatal_log, TC_LOG_FATAL);
		TC_LOG_CHECK(log_data, open_system_log, TC_LOG_SYSTEM);
	}

	return TC_OK;
}

int
tc_log_start(
	unsigned long user_data,
	char *file
)
{
	int ret = 0;
	struct tc_log *log = NULL;
	struct tc_create_link_data *cl_data = NULL;

	if (!user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	cl_data = tc_list_entry(user_data, struct tc_create_link_data, data);
	tc_log_address_print(cl_data->cl_hnode->log_data);
	log = (struct tc_log*)cl_data->cl_hnode->log_data;
	ret = tc_log_open(log, TC_LOG_MAX);
	if (ret != TC_OK)
		return ret;
	if (!file || !file[0])
		log->terminal_output = 1;
	return tc_log_data_start(
			log->log_data, 
			cl_data->private_link_data.link_id,
			file);
}

static int
tc_log_head_set(
	struct tc_log *log,
	struct tc_create_link_data *cl_data,
	char *fmt,
	...
)
{
	int ret = 0;
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	ret = tc_log_data_write(
			log->log_data, 
			cl_data->private_link_data.link_id, 
			fmt,
			arg_ptr);
	va_end(arg_ptr);

	return ret;
}

int
tc_log_write(
	unsigned long user_data,
	int direct,
	int type,
	char *fmt,
	...
)
{
	int ret = 0;
	char *head_fmt = NULL;
	va_list arg_ptr;
	time_t t = 0;
	struct tm *tm = NULL;
	struct tc_log *log = NULL;
	struct tc_create_link_data *cl_data = NULL;

	if (!user_data) {
		TC_ERRNO_SET(TC_PARAM_ERROR);
		return TC_ERR;
	}
	cl_data = tc_list_entry(user_data, struct tc_create_link_data, data);

	log = (tc_log_t)cl_data->cl_hnode->log_data;
	ret = tc_log_open(log, type);
	if (ret != TC_OK)
		return  TC_OK;	

	if (log->head_set == 0) {
		time(&t);
		tm = localtime(&t);
		if (log->terminal_output == 1) {
			ret = tc_log_head_set(log, cl_data, "%s[%s]"NONE"[%d-%d-%d %d:%d:%d]:", 
					type < TC_LOG_MAX ? global_log_print[type].color:
							    global_log_print[TC_LOG_MAX].color,
					type < TC_LOG_MAX ? global_log_print[type].name:
							    global_log_print[TC_LOG_MAX].name, 
					tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, 
					tm->tm_hour, tm->tm_min, tm->tm_sec);
		} else {
			ret = tc_log_head_set(log, cl_data, "[%s][%d-%d-%d %d:%d:%d]:", 
					type < TC_LOG_MAX ? global_log_print[type].name:
							    global_log_print[TC_LOG_MAX].name, 
					tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, 
					tm->tm_hour, tm->tm_min, tm->tm_sec);
		}
		if (ret != TC_OK)
			return ret;
	}

	va_start(arg_ptr, fmt);
	ret = tc_log_data_write(
			log->log_data,
			cl_data->private_link_data.link_id, 
			fmt, 
			arg_ptr);
	va_end(arg_ptr);
	if (ret != TC_OK)
		return ret;

	if (direct == TC_LOG_WRITE_DIRECTLY) {
		log->head_set = 0;
		return tc_log_data_end(
				log->log_data, 
				cl_data->private_link_data.link_id);
	}
	else
		log->head_set = 1;

	return TC_OK;
}

tc_log_t
tc_log_create(	
	char *app_proto
)
{
	int total_link = 0;
	cJSON *obj = NULL;
	struct tc_log *data = NULL;

	data = (struct tc_log*)calloc(1, sizeof(*data));
	if (!data) {
		TC_PANIC("calloc %d bytes error: %s\n", sizeof(*data), strerror(errno));
	}

	obj = tc_config_read_get(app_proto);

	CR_INT(obj, "total_link", total_link);
	CR_USHORT(obj, "open_log", data->open_log);
	if (data->open_log == 0)
		return (tc_log_t)data;
	CR_USHORT(obj, "open_info_log", data->open_info_log);
	CR_USHORT(obj, "open_debug_log", data->open_debug_log);
	CR_USHORT(obj, "open_fatal_log", data->open_fatal_log);
	CR_USHORT(obj, "open_system_log", data->open_system_log);
	CR_USHORT(obj, "open_warning_log", data->open_warning_log);

	data->log_data = tc_log_data_create(total_link);
	if (!data->log_data) {
		TC_FREE(data);
		return NULL;
	}

	return (tc_log_t)data;
}

void
tc_log_destroy(
	unsigned long user_data
)
{
	int ret = 0;
	struct tc_log *log = NULL;
	struct tc_create_link_data *cl_data = NULL;

	if (!user_data)
		return;
	cl_data = tc_list_entry(user_data, struct tc_create_link_data, data);
	log = (struct tc_log*)cl_data->cl_hnode->log_data;
	cl_data->cl_hnode->log_data = NULL;
	if (!log)
		return;
	ret = tc_log_open(log, TC_LOG_MAX);
	if (ret == TC_OK)
		tc_log_data_destroy(log->log_data);
	TC_FREE(log->log_data);
	TC_FREE(log);
}

void
tc_log_address_print(
	tc_log_t *log_data
)
{
	int ret = 0;
	struct tc_log *log = NULL;

	log = (struct tc_log*)log_data;

	tc_log_data_address(log->log_data);
}

