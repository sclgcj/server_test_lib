#ifndef M_PROJ_H
#define M_PROJ_H

#include "m_read_config.h"
#include "list.h"

#define M_DEFAULT_PROJECT_NUM    1000
#define M_DEFAULT_RESULT_PATH    "/tmp/result"
#define M_DEFAULT_PROJECT_PATH   "/tmp/project.txt"


struct _MProj;
typedef struct _MProj MProj, *PMProj;
struct _MProj
{
	int  iInit;
	int  iRunCnt;					//The count of project run
	int  iProjStatus;			//Project status(run, idle, delete)
	char sName[128];			//Project name
	time_t tCreateTime;		//Project created time
	time_t tLastRunEndTime;    //Project last run end time
	time_t tLastRunStartTime;  //Project last run start Time
	time_t tCurRunStartTime;   //Project current run start time
	time_t tCurRunDurationTime; //Project current run duration time;
	MProj *pStruNext;
	struct list_head struNode;
};

typedef struct _MProjArray
{
	int iProjID;
	int iProjNum;
	int iProjCnt;
	int iRunningProjCnt;
	char sResultPath[128];//The path of result file
	MLHandle	 struHandle;
}MProjArray, *PMProjArray;

typedef struct _MProjsInfo
{
	int  iProjNum;
	char sResultPath[128];
	MProj struProjHead;
}MProjInfo, *PMProjInfo;

enum{
	M_PROJECT_STATUS_IDLE,
	M_PROJECT_STATUS_RUNNING,
	M_PROJECT_STATUS_DELETE,
	M_PROJECT_STATUS_MAX
};

int
m_create_proj_file(
	int				 iProjNum,
	int				 iClearFile,
	char			 *sResultPath,
	char			 *sProjFilePath,
	MLHandle   struHandle,
	MProjArray *pStruPA
);

void
m_destroy_proj_file(
	MProjArray *pStruPA
);

int
m_get_projct_by_name(
	char			 *sName,
	MProjArray *pStruPA,
	MProj      *pStruProj
);

int
m_set_proj_by_name(
	char  *sName,
	MProj *pStruProj,
	MProjArray *pStruPA
);

int
m_add_new_proj(
	char *sName,
	MProjArray *pStruPA
);

int
m_start_proj(
	char			 *sName,
	MProjArray *pStruPA	
);

int
m_stop_proj(
	char *sName,
	MProjArray *pStruPA
);

int
m_get_project_by_id(
	int				 iID,
	MProjArray *pStruPA,
	MProj		   *pStruProj
);

int
m_set_project_by_id(
	int				 iID,
	MProjArray *pStruPA,
	MProj			 *pStruProj
);

void
m_free_proj_node(
	struct list_head *pStruNode
);

#endif
