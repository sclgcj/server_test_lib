#ifndef ST_TIMER_H
#define ST_TIMER_H 1

#include "st_thread.h"

#define ST_DEFAULT_TIMER_ARRAY  110000  //这是为了防止使用每一秒只产生10个连接造成的

typedef void * STTimerHandle;

enum
{
	ST_TIMER_STATUS_CONSTANT,			//永久存在
	ST_TIMER_STATUS_INSTANT,				//执行一次就消除
	ST_TIMER_STATUS_MAX
};

/*
 *	创建定时器管理
 */
int
st_create_timer(
	int						 iThreadNum,
	int						 iTimerNum,
	STThreadHandle struThreadHandle,
	STTimerHandle  *pStruHandle
);

/*
 * 添加定时器
 */
int 
st_add_timer(
										int iNum,														//可以被销毁几次, 主要是用于异步形式,当一个定时器处理比较多的事物,而且事物是异步的,无法确定什么时候完成的情况
										int iSecond,												//间隔多少秒执行
										int iFlag,													//是一直存在还是只执行一次
										unsigned long ulData,								//定时器参数, 这里不建议使用很多参数,可以把多个参数封装成结构体传进来
										int (*func)(unsigned long),					//执行的函数
										void (*free_data)(unsigned long),   //销毁函数,用于销毁ulData, 主要用于当定时器退出时,通知上层销毁参数
										STTimerHandle struHandle,
										int *piTimerID											//返回的定时器id
									);

/*
 * 删除定时器
 */
int 
st_destroy_timer(STTimerHandle struHandle);

/* 
 * 获取当前滴答数
 */
void
st_get_tick(STTimerHandle struHandle, int *piTick);

#endif
