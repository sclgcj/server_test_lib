#ifndef TC_CLIENT_OPER_H
#define TC_CLIENT_OPER_H 1

struct test_client_operation{
	int (*tc_cmd)();
	int (*tc_recv)(char *data_buf, int data_len, unsigned long user_data);
	int (*tc_send)(char *data_buf, int data_len, unsigned long user_data);
	//创建连接
	int (*tc_result)(unsigned long user_data);
	int (*tc_recv_check)(user_data);
	int (*tc_hub)(unsigned long user_data);
	int (*tc_data_init)(	
			unsigned int client_ip, 
			unsigned short client_port, 
			unsigned long user_data 
			);
};



#endif
