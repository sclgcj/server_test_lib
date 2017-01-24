#ifndef TC_GLOBAL_LOG_PRIVATE_H
#define TC_GLOBAL_LOG_PRIVATE_H

#include "tc_create_log.h"

int
tc_global_log_write(
	int type,
	char *fmt,
	...
);

#define TC_GINFO(f, args...) \
		tc_global_log_write(TC_LOG_INFO, "[%s->%s:%d] - f\n", \
				__FILE__, __FUNCTION__, __LINE__, ##args)

#define TC_GDEBUG(f, args...) \
		tc_global_log_write(TC_LOG_DEBUG, "[%s->%s:%d] - f\n", \
				__FILE__, __FUNCTION__, __LINE__, ##args)

#define TC_GWARN(f, args...) \
		tc_global_log_write(TC_LOG_WARN, "[%s->%s:%d] - f\n", \
				__FILE__, __FUNCTION__, __LINE__, ##args)

#define TC_GFATAL(f, args...) \
		tc_global_log_write(TC_LOG_FATAL, "[%s->%s:%d] - f\n", \
				__FILE__, __FUNCTION__, __LINE__, ##args)

#define TC_GSYSTEM(f, args...) \
		tc_global_log_write(TC_LOG_SYSTEM, "[%s->%s:%d] - f\n", \
				__FILE__, __FUNCTION__, __LINE__, ##args)

#endif
