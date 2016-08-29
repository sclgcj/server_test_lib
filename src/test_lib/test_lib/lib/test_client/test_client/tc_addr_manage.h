#ifndef TC_ADDR_MANAGE_H
#define TC_ADDR_MANAGE_H 1

enum {
	TC_ADDR_INET,
	TC_ADDR_UNIX,
	TC_ADDR_INET6,
	TC_ADDR_MAX
};

/*
 * 目前涉及到的地址，最多的还是unix套接字和ipv4，
 * 但是考虑到后期可能会扩展到ipv6上，因此这里使用
 * data[0] 来时实现变长数组，以满足不同地址结构。
 * 原意是想把所有结构地址都填进去，但是想想还是算
 * 了，感觉不怎么灵活，因为目前还不知道具体会使用
 * 多少地址结构，不想以后在增加的时候就修改已有的
 * 代码。data[0]所对应的结构根据不同类型不同：
 * TC_ADDR_INET -> struct tc_inet -> tc_addr_inet.h
 * TC_ADDR_UNIX -> struct tc_unix -> tc_addr_unix.h
 * ipv6还没有实现，以后若有新的地址结构添加进去，
 * 会在文档中说明。
 */


struct tc_address {
	int addr_type;
	char data[0];
};

#endif
