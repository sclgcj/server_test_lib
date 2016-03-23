#include "list.h"
#include "st_comm.h"
#include "st_timer.h"
#include "st_recv_check.h"

typedef struct _STRecvCheckParam
{
	int iTotalLink;
	int iRecvTimeout
}STRecvCheckParam, *PSTRecvCheck;

typedef struct _STRecvCheck
{
	int iNodeCnt;
	int iTotalCnt;
	int iSaveTime;
	int iTotalLink;
	int iRecvTimeout;
	int	iCheckListNum;
	struct list_head *pStruCheckList;
	pthread_mutex_t  *pStruCheckListMutex;
	pthread_mutex_t  struSaveMutex;
	STTimerHandle    struTimerHandle;
}STRecvCheck, *PSTRecvCheck;

int
st_create_recv_check(
	int iTotalLink,	
	int iRecvTimeout,
	int iCheckListNum,
	STTimerHandle struHandle,
	STRecvCheckHandle *pStruHandle
)
{
	int i = 0;
	STRecvCheck *pStruRC = NULL;	

	ST_CALLOC(pStruRC, SRRecvCheck, 1);
	pStruRC->iTotalLink    = iTotalLink;
	pStruRC->iRecvTimeout  = iRecvTimeout;
	pStruRC->iCheckListNum = pStruRC->iCheckListNum;
	pthread_mutex_init(&pStruRC->struSaveMutex, NULL);
	ST_CALLOC(pStruRC->pStruCheckList, struct list_head, iCheckListNum);
	ST_CALLOC(pStruRC->pStruCheckListMutex, pthread_mutex_t, iCheckListNum);
	for( ; i < iCheckListNum; i++ )
	{
		INIT_LIST_HEAD(&pStruRC->pStruCheckList[i]);
		pthread_mutex_init(&pStruRC->pStruCheckListMutex[i], NULL);
	}

	return ST_OK;
}

static void
st_free_recv_check_list(
	struct list_head *pStruHead
)
{
	struct list_head *pStruSL = pStruHead->next;
	PCCheckNode *pStruNode = NULL;

	while(pStruSL != pStruHead)
	{
		pStruNode = list_entry(pStruSL, PCCheckNode, struNode);
		list_del_init(pStruSL);
		free(pStruNode);
		pStruSL = pStruHead->next;
	}
}

void
st_destroy_recv_check(
	STRecvCheckHandle struHandle
)
{
	int i = 0;
	STRecvCheck *pStruRC = (STRecvCheck*)struHandle;

	if( !struHandle )
	{
		return;
	}

	for( ; i < pStruRC->iCheckListNum; i++ )	
	{	
		st_free_recv_check_list(&pStruRC->pStruCheckList[i]);
		pthread_mutex_destroy(&pStruRC->pStruCheckListMutex[i]);
	}
	ST_FREE();
}

//static int giTimerCnt = 0;
static int giTotalCnt = 0;
static int giTotalLink = 0;
static int giSaveTime = -1;
static int giNodeCnt = 0;
static int giRecvTimeout = 0;
//static char gcCheckTick[ST_CHECK_SIZE];
static time_t gtStartTime = 0;
static struct list_head gStruCheckList[ST_CHECK_SIZE];
static pthread_mutex_t  gStruCheckListMutex[ST_CHECK_SIZE];
static pthread_mutex_t  gStruSaveMutex = PTHREAD_MUTEX_INITIALIZER;


int
st_init_recv_check()
{
	int i = 0;
	PCConfig struConf;

	memset(&struConf, 0, sizeof(struConf));
	st_get_config(&struConf);
	giRecvTimeout = struConf.iRecvTimeout;
	giTotalLink = struConf.iTotalLink;

	for( i = 0; i < ST_CHECK_SIZE; i++ )
	{
		INIT_LIST_HEAD(&gStruCheckList[i]);
		pthread_mutex_init(&gStruCheckListMutex[i], NULL);
	}

	//st_create_recv_check();

	return ST_OK;
}

int
st_uninit_recv_check()
{
	int i = 0;

	for( i = 0; i < ST_CHECK_SIZE; i++ )
	{
		pthread_mutex_lock(&gStruCheckListMutex[i]);
		st_free_recv_check_list(&gStruCheckList[i]);
		pthread_mutex_unlock(&gStruCheckListMutex[i]);
		pthread_mutex_destroy(&gStruCheckListMutex[i]);
	}
}


static int
st_check_assistant_udp_timeout(
	struct timespec *pStruTS,
	PCConfig   *pStruConf,
	PushClient *pStruPC 
)
{
#if  0
	int i = 0;
	int iRet = 0;
	int iStatus = 0;

	for(i = 0; i < pStruConf->iMapCnt; i++ )
	{
		pthread_mutex_lock(pStruPC->pStruAssistantMutex);
		iStatus = pStruPC->pStruAssistantInfo[i].iStatus;
		pthread_mutex_unlock(pStruPC->pStruAssistantMutex);
		if( pStruPC->pStruAssistantInfo[i].iTick == ST_CHECK_TICK_STOP )
		{
			continue;
		}

		if( pStruPC->pStruAssistantInfo[i].iAssistantSockfd )
		{
			iRet = ST_ERR;
		}

		if(pStruPC->pStruAssistantInfo[i].iAssistantSockfd && 
			 iStatus == ST_STATUS_ASSISTANT_BOOT)
		{
			{
				//PC_ERROR("iTick = %d, %d\n", pStruPC->pStruAssistantInfo[i].iTick, pStruConf->usUdpRetransferTime);
				if( pStruPC->pStruAssistantInfo[i].iTick >= pStruConf->usUdpRetransferTime )
				{
					if( pStruPC->iRetransferCount >= pStruConf->usUdpRetransferCnt )
					{
						st_set_fail(st_get_step_by_status(pStruPC->pStruAssistantInfo[i].iStatus), ST_FAIL_RECV_TIMEOUT, pStruPC);
						pStruPC->pStruAssistantInfo[i].iConnectResult = ST_ERR;
						iRet = ST_ERR;
						continue;
					}
					else
					{
						pStruPC->iRetransferCount++;
						pStruPC->pStruAssistantInfo[i].iTick = 0;
						st_write_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, pStruPC);
						PC_ERROR("sConnToken = %s\n", pStruPC->pStruAssistantInfo[i].sConnToken);
					//	st_assistant_boot(&pStruPC->pStruAssistantInfo[i], pStruPC->pStruAssistantInfo[i].sConnToken, pStruPC);
					}
				}
				else
				{
					pStruPC->pStruAssistantInfo[i].iTick++;
				}
			}
		}
		else if(pStruPC->pStruAssistantInfo[i].iStatus == ST_STATUS_SEND_TEST_DATA)
		{
			if(pStruPC->pStruAssistantInfo[i].iTick >= pStruConf->iRecvTimeout)
			{
				st_set_fail(st_get_step_by_status(pStruPC->pStruAssistantInfo[i].iStatus), ST_FAIL_RECV_TIMEOUT, pStruPC);
				return ST_ERR;
			}
			else
			{
				pStruPC->pStruAssistantInfo[i].iTick++;
			}
		}
	}
	return iRet;
#endif
	return ST_OK;
}


static int
st_check_assistant_timeout(
	struct timespec *pStruTS, 
	PCConfig			  *pStruConf,
	PushClient      *pStruPC
)
{
	int i = 0;	
	int iNum = 0;
	int iRet = 0;
	PCNatLink *pStruNL = NULL;

	ST_CALLOC(pStruNL, PCNatLink, (pStruConf->iMapCnt + 2) );

	pthread_mutex_lock(pStruPC->pStruAssistantMutex);		
	iNum = *(pStruPC->pusAssistantCnt);
	if( iNum == 0 )
	{
		i = -1;
	}
	memcpy(pStruNL, pStruPC->pStruAssistantInfo, sizeof(PCNatLink) * iNum);
	pthread_mutex_unlock(pStruPC->pStruAssistantMutex);
	if( i == -1 )
	{
		free(pStruNL);
		return ST_ERR;
	}

	for( i = 0; i < iNum; i++ )
	{
		if( pStruNL[i].iTick == ST_CHECK_TICK_STOP )
		{
			continue;
		}
		if(pStruTS->tv_sec - pStruNL[i].struSendData.tv_sec > giRecvTimeout ||
			  (pStruTS->tv_sec - pStruNL[i].struSendData.tv_sec == giRecvTimeout && 
				 pStruTS->tv_nsec - pStruNL[i].struSendData.tv_nsec > 0))
		{
			PC_ERROR("status = %d\n", pStruNL[i].iStatus);
			PC_ERROR("starttime = %ld.%ld, endtime = %ld.%ld\n", pStruNL[i].struSendData.tv_sec, pStruNL[i].struSendData.tv_nsec, pStruTS->tv_sec, pStruTS->tv_nsec);
			PC_ERROR("dfsdfsdfsdfs, iSockfd = %d, port = %d, ip = %s, sec_offset = %ld, nsec_offset = %ld\n", pStruNL[i].iAssistantSockfd, pStruPC->usClientPort, inet_ntoa(pStruPC->struClientIP), pStruTS->tv_sec - pStruNL[i].struSendData.tv_sec, pStruTS->tv_nsec - pStruNL[i].struSendData.tv_nsec);
			//PC_DEBUG("fsdfasdfasdfasfasf, isockfd = %d, tick =%d, giRecvTimeout = %d, uistatus = %d\n", struPC.iSockfd, struPC.uiTick, giRecvTimeout, struPC.uiStatus);
			pthread_mutex_lock(pStruPC->pStruAssistantMutex);
			pStruPC->pStruAssistantInfo[i].iTick = ST_CHECK_TICK_STOP;
			pthread_mutex_unlock(pStruPC->pStruAssistantMutex);
			st_set_fail(st_get_step_by_status(pStruNL[i].iStatus), ST_FAIL_RECV_TIMEOUT, pStruPC);
			PC_DEBUG("\n");
			iRet = ST_FAIL_RECV_TIMEOUT;
			continue;
		}
		else
		{
			pthread_mutex_lock(pStruPC->pStruAssistantMutex);
			pStruPC->pStruAssistantInfo[i].iTick++;
			pthread_mutex_unlock(pStruPC->pStruAssistantMutex);
		}
	}

	free(pStruNL);
	pStruNL = NULL;

	return iRet;
}

static void
st_check_nat_link(
	PushClient *pStruPC 
)
{
	int iRet = 0;
	int iStatus = 0;
	PCConfig struConf;

	memset(&struConf, 0, sizeof(struConf));
	st_get_config(&struConf);

//	PC_ERROR("iRet = %d -- %d\n",pStruPC->iCurSockfd, iRet);
	pthread_mutex_lock(pStruPC->pStruResultMutex);
	iStatus = *pStruPC->puiStatus;
	pthread_mutex_unlock(pStruPC->pStruResultMutex);

	if( iStatus != ST_STATUS_CONNECT_TO_NAT_SERVER )
	{
		return;
	}

	//检查代理服务是否已经打洞成功
	//如果没有发送连接, 则开始发送连接
	if( pStruPC->uiStatus == ST_ERR )
	{
		pStruPC->uiStatus = ST_OK;
		clock_gettime(CLOCK_MONOTONIC, &pStruPC->struCreateTime);
		st_write_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, pStruPC);
		iRet = st_loop_connect(
												pStruPC->iNatSockfd, 
												struConf.iRecvTimeout,
												&pStruPC->struSendAddr,
												sizeof(pStruPC->struSendAddr)
											);
		if( iRet != ST_OK )
		{
			if( iRet != ST_FAIL_WOULDBLOCK )
			{
				st_set_fail_value(ST_CONNECT_TO_SERVER, iRet, pStruPC);	
			}
			else
			{
				st_mod_write_listen_sockfd(&pStruPC->pStruED[0]);
			}
		}
		else
		{
			st_mod_write_listen_sockfd(&pStruPC->pStruED[0]);
		}
	}
}

static int
st_recv_check(
	unsigned long ulData
)
{
	int iRet = 0;
	int iCnt = ulData;
	struct list_head *pStruHead = &gStruCheckList[iCnt];
	struct timespec struTS;
	struct timespec struT1, struT2;
	struct timespec struSendData;
	PCCheckNode *pStruHN = NULL;
	PCConfig struConf;
	PushClient struPC;
	PCNatLink *pStruNL = NULL;
	
	memset(&struConf, 0, sizeof(struConf));
	st_get_config(&struConf);

#if 1
	//PC_DEBUG("++++++++++++++++++++++ulData = %ld\n", ulData);
	iRet = pthread_mutex_trylock(&gStruCheckListMutex[iCnt]);
	if( iRet != 0 )
	{
		if( iRet == EBUSY )
		{
			return ST_OK;
		}
		PC_ERROR("pthread_mutex_trylock: %s\n", strerror(errno));
		exit(0);
	}
	//clock_gettime(CLOCK_MONOTONIC, &struT1);
	//PC_DEBUG("giRecvTimeout = %d\n", giRecvTimeout);
	
	clock_gettime(CLOCK_MONOTONIC, &struTS);
	list_for_each_entry(pStruHN, pStruHead, struNode)
	{
		memset(&struPC, 0, sizeof(struPC));
		st_read_mmap_data_by_offset(pStruHN->iOffset, &struPC);
		/*if( struConf.iProtocol == ST_PROTO_UDP )
		{
			iRet = st_check_assistant_udp_timeout(&struTS, &struConf, &struPC);
			if( iRet != ST_OK )
			{
				continue;	
			}
		}

		iRet = st_check_assistant_timeout(&struTS, &struConf, &struPC);*/

		iRet = ST_ERR;
		if( iRet == ST_ERR )
		{
			if( (*struPC.puiTick) == ST_CHECK_TICK_STOP )
			{
				continue;
			}
			pthread_mutex_lock(struPC.pStruResultMutex);
			(*struPC.puiTick)++;
			pthread_mutex_unlock(struPC.pStruResultMutex);

			if(struTS.tv_sec - struPC.struSendData.tv_sec > giRecvTimeout ||
				  (struTS.tv_sec - struPC.struSendData.tv_sec == giRecvTimeout && struTS.tv_nsec - struPC.struSendData.tv_nsec > 0))
			//if( struPC.uiTick == giRecvTimeout )
			{
				PC_ERROR("starttime = %ld.%ld, endtime = %ld.%ld\n", struPC.struSendData.tv_sec, struPC.struSendData.tv_nsec, struTS.tv_sec, struTS.tv_nsec);
				PC_ERROR("dfsdfsdfsdfs, iSockfd = %d, port = %d, ip = %s, sec_offset = %ld, nsec_offset = %ld\n", struPC.iSockfd, struPC.usClientPort, inet_ntoa(struPC.struClientIP), struTS.tv_sec - struPC.struSendData.tv_sec, struTS.tv_nsec - struPC.struSendData.tv_nsec);
				//PC_DEBUG("fsdfasdfasdfasfasf, isockfd = %d, tick =%d, giRecvTimeout = %d, uistatus = %d\n", struPC.iSockfd, struPC.uiTick, giRecvTimeout, struPC.uiStatus);
				st_set_fail(st_get_step_by_status(*struPC.puiStatus), ST_FAIL_RECV_TIMEOUT, &struPC);
				PC_DEBUG("\n");
			}
		}
	}
	//clock_gettime(CLOCK_MONOTONIC, &struT2);
	//PC_DEBUG("check proces = %ld.%ld\n", struT2.tv_sec - struT1.tv_sec, struT2.tv_nsec, struT1.tv_nsec);
	pthread_mutex_unlock(&gStruCheckListMutex[ulData]);
#endif

	return ST_OK;
}

int
st_add_recv_check(
	PushClient *pStruPC
)
{
	int iRet = 0;
	int iOffset = 0;
	int iIntervalTime = 0;
	//time_t tCurTime = time(NULL);
	static struct timespec struT1;
	struct timespec struT2;
	PCCheckNode *pStruHN = NULL;
	
	iRet = st_get_offset(
						pStruPC->struClientIP.s_addr, 
						pStruPC->usClientPort, 
						&iOffset 
						);
	if( iRet != ST_OK || iOffset < 0 )
	{
		return iRet;
	}

	PC_DEBUG("\n");
	if( gtStartTime == 0 )
	{
		//gtStartTime = 1;
		clock_gettime(CLOCK_MONOTONIC, &struT1);
		gtStartTime = time(NULL);
	}

	PC_DEBUG("\n");
	ST_CALLOC(pStruHN, PCCheckNode, 1);
	/*pStruHN = (PCCheckNode*)calloc(1, sizeof(PCCheckNode));
	if( !pStruHN )
	{
		PC_ERROR("calloc: %s\n", strerror(errno));
		exit(0);
	}*/
	pStruHN->iOffset = iOffset;
	clock_gettime(CLOCK_MONOTONIC, &struT2);
	iIntervalTime = struT2.tv_sec - struT1.tv_sec;
	//struT2.tv_sec -= struT1.tv_sec;
	//struT2.tv_nsec -= struT1.tv_nsec;
	pthread_mutex_lock(&gStruCheckListMutex[iIntervalTime]);
	pthread_mutex_lock(&gStruSaveMutex);
	if( giSaveTime != iIntervalTime || giTotalCnt == giTotalLink - 1 )
	{
		if( giSaveTime != -1 || giTotalCnt == giTotalLink - 1)
		{
			if( giNodeCnt == 0 )
			{
				giNodeCnt = 1;
			}
			if( giSaveTime == -1 )
			{
				giSaveTime = 0;
			}
		//	PC_ERROR("+++++++++++++++++++++++++iIntervalTime = %d, giSabeTime = %d, giTotalCnt = %d, giTotalLink = %d\n", iIntervalTime, giSaveTime, giTotalCnt, giTotalLink);
			st_create_timer(giNodeCnt, 1, ST_TIMER_STATUS_CONSTANT, giSaveTime, st_recv_check, NULL, NULL);
		}
		giNodeCnt = 0;
		giSaveTime = iIntervalTime;
	}
	list_add_tail(&pStruHN->struNode, &gStruCheckList[iIntervalTime]);
	giNodeCnt++;
	giTotalCnt++;
	pthread_mutex_unlock(&gStruSaveMutex);
	pthread_mutex_unlock(&gStruCheckListMutex[iIntervalTime]);
	PC_DEBUG("\n");
	return ST_OK;
}

void
st_stop_recv_check(
	int iSockfd,
	PushClient *pStruPC
)
{
	PCNatLink *pStruNL = NULL;
	PushClient struPC;


	memset(&struPC, 0, sizeof(struPC));
	st_read_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, &struPC);
	struPC.iCurSockfd = iSockfd;
	/*st_get_assistan_link(&struPC, &pStruNL);
	if( pStruNL )
	{
		pStruNL->iTick = ST_CHECK_TICK_STOP;
		return;
	}*/
//	st_read_mmap_data_by_sock(iSockfd, &struPC);
	struPC.uiTick = ST_CHECK_TICK_STOP;
//	PC_ERROR("uiTick = %d\n", struPC.uiTick);
	st_write_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, &struPC);
}

void
st_start_recv_check(
	int iSockfd,
	PushClient *pStruPC
)
{
	PushClient struPC;

	memset(&struPC, 0, sizeof(struPC));
	st_read_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, &struPC);
	//st_read_mmap_data_by_sock(iSockfd, &struPC);
	struPC.uiTick = 0;
	st_write_mmap_data(pStruPC->struClientIP.s_addr, pStruPC->usClientPort, &struPC);
//	st_write_mmap_data_by_sock(iSockfd, &struPC);
}

