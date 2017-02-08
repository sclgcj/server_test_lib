#ifndef TC_JC_PRIVATE_2017_01_19_09_28_32_H
#define TC_JC_PRIVATE_2017_01_19_09_28_32_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include "list.h"
#include "tc_err.h"
#include "cJSON.h"

#define JC_ERR		TC_ERR
#define JC_OK		TC_OK
#define JC_OBJ  	1
#define JC_ARR  	2
#define JC_NORMAL	3
#define JC_ARR_CNT	4

#define JC_MOD_INIT(func) \
	int __attribute__((constructor)) func()
#define JC_MOD_EXIT(func) \
	int __attribute__((constructor)) func()

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
	//id为当前处理的id, depth为深度， handle 为处理类型，user_data为用户数据 
	//root为需要处理的json结构
	cJSON* (*walk_cb)(int id, int depth, unsigned long user_data, cJSON *root); 
};

typedef char* (*user_func)(char *data);
/*
 * 模块:
 * iteration	-- 用于获取迭代数  优先级1
 * mode		-- 用于判断当前是否满足获取下一个值的条件，满足则返回OK，不满足则返回ERR,  优先级 100
 * count	-- 用于获取count值, 主要用于记录数组的个数  优先级200
 * result	-- 用于获取处理类型，json 或者 get, 优先级200
 * value	-- 用于获取当前配置中value的值, 优先级 300
 * extra	-- 用于获取外部处理函数，如果存在使用该函数，从而忽略后续操作, 优先级 400
 * type		-- 根据具体类型对value进行处理, 并获取返回值, 优先级 500
 * handle	-- 处理value的返回值，现在有两种形式: json参数，get参数, 优先级 600
 */
struct jc_oper {
	/* 简单说明一下返回值:
	 * JC_OK  -- 执行成功
	 * JC_ERR -- 说明出错
	 * */
	int (*jc_init)(
		char *node_name, 
		cJSON *obj, 
		unsigned long user_data,
		struct jc_comm *jcc);
	int (*jc_copy)(unsigned int data_num);
	int (*jc_func_add)(int level, int (*extra_func)(unsigned long user_data));
	/* 参数说明 :
	 * node_name:	该json节点的名称
	 * type:	该json节点处理的类型
	 * user_data:	用户数据
	 * jcc:		公用数据
	 */
	int (*jc_execute)(
			char *node_name, 
			unsigned long user_data, 
			struct jc_comm *jcc);
};

int
jc_module_add(
	char *name,
	int  level,
	struct jc_oper *js_oper
);


int
jc_init();

int
jc_rename_add(
	char *orignal_name,
	char *new_name
);

/*
 * 主要初始化每个节点配置，不产生任何数据
 */

int 
jc_param_init(
	int id,
	unsigned long user_data,
	cJSON *root
);

/*
 * 把配置的json数据转换成相应的数据
 */
char *
jc_to_param(
	int id,
	unsigned long user_data,
	cJSON *root
);

#endif
