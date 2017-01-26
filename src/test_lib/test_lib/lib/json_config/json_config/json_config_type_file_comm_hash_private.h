#ifndef JSON_CONFIG_TYPE_FILE_COMM_HASH_PRIVATE_H
#define JSON_CONFIG_TYPE_FILE_COMM_HASH_PRIVATE_H

#include "json_config_letter_hash_private.h"
#include "json_config_type_file_private.h"
#include "json_config_var_module_hash_private.h"
#include "tc_hash.h"

/*
 * 这个单个文件模板
 */
struct jc_type_file_comm_node {
	char *var_name;			
	int  col_num;			//指明数据在第几 
	int  file_size;                  //指明文件大小
	char *lib;                      //指明文件路径
	struct hlist_node node;
	char data[0];
};

/*
 * 这个是每个文件模板具体的值
 */
struct jc_type_file_comm_var_node {
	void *map_ptr;
	void *cur_ptr;
	char *last_val;
	char *var_name;			//为了节省空间，直接引用的struct jc_type_file_comm_node中的var_name
	void *var_hash;
	struct hlist_node node;
	char data[0];
};

struct jc_type_file_comm_vuser_node {
	int id;
	void *comm_hash;
	tc_hash_handle_t var_spec_hash;
	struct hlist_node node;
};

struct jc_type_file_comm_hash_oper {
	int (*comm_hash_init)(struct json_config_comm *jcc, 
			      struct jc_type_file_comm_node *fcn);
	int (*comm_hash_copy)(struct jc_type_file_comm_var_node *fcnv,
			      struct jc_type_file_comm_vuser_node *svn);
	/*
	 * 执行值会存放在var_node的last_value 和 jcc的retval中
	 */
	int (*comm_hash_execute)(struct json_config_comm *jcc,
			         struct jc_type_file_comm_node *fcn, 
				 struct jc_type_file_comm_var_node *fcnv, 
				 struct jc_type_file_comm_vuser_node *svn);
};

struct jc_type_file_comm_hash {
	int  vuser_num;			 
	int  var_node_size;
	int  comm_node_size;
	jc_letter_t var_hash;
//	tc_hash_handle_t var_hash;
	tc_hash_handle_t vuser_hash;	
	struct jc_type_file_comm_hash_oper oper;
	int (*init)(struct jc_type_file_comm_hash *cj, 
		    struct json_config_comm *jcc);
	int (*copy)(struct jc_type_file_comm_hash *cj,
		    unsigned int data_num);
	int (*execute)(struct jc_type_file_comm_hash *cj, 
		       struct json_config_comm *jcc);
};

int
json_config_type_file_comm_create(
	int var_node_size,
	int comm_node_size,
	struct jc_type_file_comm_hash_oper *oper
);

int
json_config_type_file_comm_destroy(
	struct jc_type_file_comm_hash *ch
);

#endif

