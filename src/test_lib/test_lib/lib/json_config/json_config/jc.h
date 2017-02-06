#ifndef JC_H
#define JC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include "list.h"
#include "tc_err.h"
#include "cJSON.h"

//提供了一个所有模块可共用的结构
//每个模块会根据优先级一次执行，
//需要共享的数据可以先存储在该结
//构中。
#define JC_GET_VALUE		1
#define JC_IGNORE_VALUE		2

struct jc_comm {
	int id;			//当前处理的用户id
	int end;		//停止继续执行下面的模块
	int depth;		//当前深度，由jc_to_param开始的时候设置，其他模块中使用
	int count;		//数组个数，count模块赋值，array模块使用
	int getvalue;		//是否获取值，mode模块赋值, type模块使用
	int iteration;		//当前迭代数，由iteration赋值, 其他模块使用
	unsigned char *mode_type; //mode模块的当前类型，在mode模块赋值，type模块使用
	unsigned char *next_row; //获取下一行的方式，有next_row模块赋值，type模块使用
	unsigned char *result;  //处理类型, 由result模块赋值，由handle模块处理
	unsigned char *retval;  //返回值, 由type模块赋值，handle模块使用, 如果需要其他模块的返回值，可以另外定义
	unsigned long out_data; //存放最终结果的地方, 在jc_to_param开始时赋值，由handle模块处理
	unsigned long module_private;  //模块的私有数据
	cJSON *conf_val;	//value模块赋值，type模块使用
	//如果是object类型的话，则会调用此回调函数进行处理。
	//id为当前处理的id, handle 为处理类型，user_data为用户数据 
	//root为需要处理的json结构
	cJSON* (*walk_cb)(int id, int depth, unsigned long user_data, cJSON *root); 
};



#endif
