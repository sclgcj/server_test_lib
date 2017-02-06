#ifndef TC_JC_PRIVATE_2017_01_19_09_28_32_H
#define TC_JC_PRIVATE_2017_01_19_09_28_32_H

#include "jc.h"

#define JC_ERR		TC_ERR
#define JC_OK		TC_OK
#define JC_OBJ  	1
#define JC_ARR  	2
#define JC_NORMAL	3
#define JC_ARR_CNT	4

#include "jc.h"

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
jc_init();

int
jc_module_add(
	char *name,
	int  level,
	struct jc_oper *js_oper
);

int
jc_rename_add(
	char *orignal_name,
	char *new_name
);

cJSON *
jc_param_init(
	int id,
	unsigned long user_data,
	cJSON *root
);

char *
jc_to_param(
	int id,
	unsigned long user_data,
	cJSON *root
);

/*
 * JC_NODE_HANDLE: a macro function
 * json: a json structure
 * d: user data
 * jc: struct jc_comm, not a pointer
 * mem: jc_execute or jc_init
 * r: return value
 */
#define JC_NODE_HANDLE(json, d, jc, mem, r) do{\
	char *mod = NULL; \
	cJSON *obj = NULL; \
	struct jc_rename *jr = NULL; \
	struct jc_module *jm = NULL; \
	list_for_each_entry(jm, &global_jc.json_module_list, node) { \
		jr = jc_rename_get_by_orignal_name(jm->name); \
		if (jr) \
			mod = jr->new_name; \
		else \
			mod = jm->name; \
		obj = cJSON_GetObjectItem(json, mod); \
		if (!obj) { \
			fprintf(stderr, "no object named %s whose orignal name is %s\n", mod, jm->name); \
			break; \
		} \
		if (jm->oper.mem) { \
			r = jm->oper.mem(json->string, obj->valuestring, d, &jc); \
			if (r != JC_OK) \
				break; \
		} \
	} \
}while(0)

#endif
