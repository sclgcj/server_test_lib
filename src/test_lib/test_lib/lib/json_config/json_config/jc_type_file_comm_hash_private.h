#ifndef JC_TYPE_FILE_COMM_HASH_PRIVATE_H
#define JC_TYPE_FILE_COMM_HASH_PRIVATE_H

#include "jc_letter_hash_private.h"
#include "jc_number_hash_private.h"
#include "jc_type_file_private.h"
#include "jc_var_module_hash_private.h"
#include "tc_hash.h"
#include <pthread.h>

/*
 * 这个模块有点问题...　使用起来有点问题，感觉略微的麻烦，
 * 暂时还没想到如何修改，暂时先用上
 */

/*
 * 这个单个文件模板
 */
struct jc_type_file_comm_node {
	char *var_name;			
	int  col_num;			//指明数据在第几 
	int  file_size;                  //指明文件大小
	char *lib;                      //指明文件路径
	void *comm_hash;		//指向所属的hash结构
	char data[0];
};

/*
 * 这个是每个文件模板具体的值
 */
struct jc_type_file_comm_var_node {
	int  line_num;
	void *map_ptr;
	void *cur_ptr;
	char *last_val;
	char *var_name;			//为了节省空间，直接引用的struct jc_type_file_comm_node中的var_name
	void *var_hash;
	pthread_mutex_t mutex;
	char data[0];
};

struct jc_type_file_comm_vuser_node {
	void *comm_hash;
	tc_hash_handle_t var_spec_hash;
};

struct jc_type_file_comm_hash_oper {
	/*
	 * comm_hash_init: 初始化调用者的私有数据
	 * comm_hash_copy: 拷贝调用者的私有数据
	 * comm_hash_destroy: 销毁调用者的comm_node私有数据
	 * comm_var_node_destroy:　销毁单个用户的私有数据
	 * comm_hash_execute: 执行调用者私有数据,　目前主要用于判断是否需要
	 *		　　　获取下一个数据
	 */
	int (*comm_hash_init)(struct jc_type_file_comm_node *fsn);
	int (*comm_hash_copy)(struct jc_type_file_comm_node *fsn, 
			      struct jc_type_file_comm_var_node *svar);
	/*
	 * 执行值会存放在var_node的last_value 和 jcc的retval中
	 */
	int (*comm_node_destroy)(struct jc_type_file_comm_node *fsn);
	int (*comm_var_node_destroy)(struct jc_type_file_comm_var_node *svar);
	char* (*comm_hash_execute)(char separate, 
				   struct jc_type_file_comm_node *fsn,
				   struct jc_type_file_comm_var_node *svar);
};

struct jc_type_file_comm_hash {
	int  vuser_num;			 
	int  var_node_size;
	int  comm_node_size;
	jc_letter_t var_hash;
	jc_number_t vuser_hash;
	struct jc_type_file_comm_hash_oper oper;
	int (*init)(struct jc_type_file_comm_hash *cj, 
		    struct jc_comm *jcc);
	int (*copy)(struct jc_type_file_comm_hash *cj,
		    unsigned int data_num);
	int (*execute)(struct jc_type_file_comm_hash *cj, 
		       struct jc_comm *jcc);
};

struct jc_type_file_comm_hash*
jc_type_file_comm_create(
	int var_node_size,
	int comm_node_size,
	struct jc_type_file_comm_hash_oper *oper
);

int
jc_type_file_comm_destroy(
	struct jc_type_file_comm_hash *ch
);

#endif

