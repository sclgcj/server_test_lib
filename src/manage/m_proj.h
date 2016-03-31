#ifndef M_PROJ_H
#define M_PROJ_H

#include "manage.h"

typedef struct _MProj
{
	int  iRunCnt;					//The count of project run
	int  iPorjStatus;			//Project status(run or idle)
	char sName[128];			//Project name
	char sResultPath[128];//The path of result file
	time_t tCreateTime;		//Project created time
	time_t tLastRunEndTime;    //Project last run end time
	time_t tLastRunStartTime;  //Project last run start Time
	time_t tCurRunStartTime;   //Project current run start time
	time_t tCurRunDurationTime; //Project current run duration time;
}MProj, *PMProj;

typedef struct _MProjArray
{
	int iProjNum;
	MProj *pStruProj;
}MProjArray, *PMProjArray;

enum{
	M_PROJECT_STATUS_IDLE,
	M_PROJECT_STATUS_RUNNING,
	M_PROJECT_STATUS_MAX
};

#endif
