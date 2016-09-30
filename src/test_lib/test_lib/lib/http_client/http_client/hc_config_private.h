#ifndef HC_CONFIG_PRIVATE_H
#define HC_CONFIG_PRIVATE_H

struct hc_config{
	int total_link;				//连接总数
	int recv_timeout;			//接收超时
	int conn_timeout;			//连接超时
	int rendevous_enable;			//开启集合点
	int circle_run;				//循环跑
	int cirle_time;				//循环次数
	unsigned short http_server_port;	//服务器端口
	unsigned short res;
	char http_type[8];			//http类型，http 或者 https
	char http_server_ip[128];		//服务器地址，可以是域名
};

#endif
