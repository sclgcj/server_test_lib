#ifndef TC_CREATE_COMM_H
#define TC_CREATE_COMM_H 

int
tc_event_set(int event, unsigned long user_data);

int
tc_peer_info_get(unsigned long user_data, struct in_addr *peer_addr, unsigned short *peer_port);

int
tc_local_info_get(unsigned long user_data, 
		  struct in_addr *local_addr, 
		  unsigned short *local_port);

#endif