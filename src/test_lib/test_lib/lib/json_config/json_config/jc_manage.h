#ifndef JC_MANAGE_H
#define JC_MANSGE_H

#define JC_INIT(func) \
	int __attribute__((constructor)) func()

/*
 * jc_manage_create() - 创建json配置的管理结构
 * @name :	该json配置的名字
 * @file_path:  该json配置的路径(完整路径)
 *
 * Return: 成功返回0， 失败返回-1
 */
int
jc_manage_create(
	char *name,
	char *file_path
);

/*
 * jc_manage_destroy() -- 销毁json配置管理的所有配置
 *
 * Return: 成功返回0， 失败返回-1
 */
int
jc_manage_destroy();

/*
 * jc_manage_special_destroy() -- 销毁指定的json配置
 * @name: 指定的json配置名
 *
 * Return: 成功返回0， 失败返回-1
 */
int
jc_manage_special_destroy(char *name);

/*
 * jc_manage_param_get() -- 获取指定json配置的参数
 * @id: 用户id。由于该配置是可用于多个虚拟用户的，因此，每个虚拟用户
 *	使用自己id进行获取即可。id从0开始，直到指定的最大虚拟用户数
 * @user_data: 获取时使用的用户数据。主要用于一些注册的用户回调函数（目前没用）
 * @name: json配置的名字
 *
 * Return: 成功时返回生成的参数首地址，失败则返回0
 *
 * 该函数会自动生成json文件配置的参数结构(json或者get，以后会更多), 返回值
 * 为unsigned long是因为无法直接给出准确的数据格式，但是这个可以由调用者自
 * 行决定
 *
 */
unsigned long
jc_manage_param_get(
	int  id,
	unsigned long user_data,
	char *name
);

void
jc_manage_iteration_increase();

int
jc_manage_iteration_get();

#endif
