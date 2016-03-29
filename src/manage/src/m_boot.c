#include "m_boot.h"
#include "cJSON.h"

/*
 * If the client connected to the server successfully, 
 * it should send a boot request to the server to report
 * its basic information.
 *
 * The information contains : 
 *	1、The client's project info
 *	2、The current running project
 *	3、The last ending project
 *	4、The start time of current running project
 *	5、The start time of last ending project
 *	6、The end time of last ending project
 *	7、The duration time of the current runngin project
 *
 *	注意：这里有一个点是：客户端必须持续连接服务器
 */

int 
m_boot(
	MLink *pStruML
)
{	
		
}





