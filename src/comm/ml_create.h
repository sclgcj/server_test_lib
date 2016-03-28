#ifndef ML_CREATE_H
#define ML_CREATE_H

#include "ml_comm.h"
#include "list.h"

typedef void * MLCLHandle;
typedef int (*MLCreateLinkFunc)(int iLinkID, struct in_addr struAddr, unsigned short usPort, void *pUserData);

typedef struct _MLCLParam
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
}MLCLParam, *PMLCLParam;

int
ml_create_link_handle(
	void						  *pUserData,
	MLCLParam			 	  *pStruCLParam,
	MLCreateLinkFunc  pCLFunc,	
	MLThreadHandle	  struThreadHandle,
	MLCLHandle		 	  *pStruHandle
);

void
ml_destroy_link_handle(
	MLCLHandle struHandle
);

int
ml_start_create_link(
	MLCLHandle struHandle	
);

#endif

