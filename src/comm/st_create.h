#ifndef ST_CREATE_H
#define ST_CREATE_H

#include "st_comm.h"
#include "list.h"

typedef void * STCLHandle;
typedef int (*STCreateLinkFunc)(int iLinkID, struct in_addr struAddr, unsigned short usPort, void *pUserData);

typedef struct _STCLParam
{
	int iThreadNum;				//The num of creating thread
	int iStackSize;    		//The size of every stack
	int iIpCount;			 		//The total number of thread
	int iTotalLink;				//The total num of links that need to create
	int iCreateLink;   
	int iCreateLinkTime;	//The iCreateLinkTime and iCreateLink mean that creating iCreateLink tcp-link
												//Every iCreatLinkTime seconds
	int iPortMapCnt;			//This element means how many ports between the last two links
	unsigned short usStartPort; //The starting port
	unsigned short usEndPort;		//The End port
	struct in_addr struStartAddr; //The starting addr
}STCLParam, *PSTCLParam;

int
st_create_link_handle(
	void						  *pUserData,
	STCLParam			 	  *pStruCLParam,
	STCreateLinkFunc  pCLFunc,	
	STThreadHandle	  struThreadHandle,
	STCLHandle		 	  *pStruHandle
);

void
st_destroy_link_handle(
	STCLHandle struHandle
);

int
st_start_create_link(
	STCLHandle struHandle	
);

#endif

