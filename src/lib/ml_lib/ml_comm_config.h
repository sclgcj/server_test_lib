#ifndef ML_COMM_CONFIG_H
#define ML_COMM_CONFIG_H 1

struct _MLOptConfig;
typedef struct _MLOptConfig MLCommConfig,*PMLCommConfig;

struct _MLOptConfig
{
	char *sName;
	char *sVal;
	MLCommConfig *pStruNext;
};

enum
{
	ML_CT_OPT,
	ML_CT_CONFIG,
	ML_CT_MAX
};

#define ML_ALLOC_CONFIG(cur) do{ \
	MLCommConfig *pStruCC = NULL; \
	ML_CALLOC(pStruCC, MLCommConfig, 1); \
	cur->pStruNext = pStruCC; \
}while(0)

#define ML_SET_CONFIG(name, val, cur) do{ \
	int iLen = strlen(name); \
	ML_CALLOC(cur->sName, char, (iLen + 1)); \
	memcpy(cur->sName, name, iLen); \
	if( val ) \
	{\
		iLen = strlen(val); \
		ML_CALLOC(cur->sVal, char, (iLen + 1)); \
		memcpy(cur->sVal, val, iLen); \
	}\
}while(0)

#define ML_SET_CONFIG_WITH_NEXT(name, val, cur) do{ \
	ML_SET_CONFIG(name, val, cur); \
	ML_ALLOC_CONFIG(cur); \
	cur = cur->pStruNext; \
}while(0)

#define ML_ADD_CONFIG(name, val, head) do{\
	MLCommConfig *pStruCC = NULL; \
	ML_CALLOC(pStruCC, MLCommConfig, 1); \
	ML_SET_CONFIG(name, val, pStruCC); \
	pStruCC->pStruNext = head->pStruNext; \
	head->pStruNext = pStruCC; \
}while(0)

#define ML_DEMLROY_COMM_CONFIG(head) do{ \
	MLCommConfig *pStruCur = NULL, *pStruNext = head; \
	while(pStruNext) \
	{ \
		ML_FREE(pStruNext->sName); \
		ML_FREE(pStruNext->sVal); \
		pStruCur = pStruNext; \
		pStruNext = pStruCur->pStruNext; \
		ML_FREE(pStruCur); \
	} \
}while(0)

#define ML_GET_VAL(name, val, head, ret) do{\
	int ilen = 0; \
	MLCommConfig *pStruCur = head; \
	while(pStruCur && pStruCur->sName) \
	{ \
		ML_CMP_DATA(name, pStruCur->sName, ret); \
		if(ret == ML_OK) \
		{ \
			(*val) = pStruCur->sVal; \
			break; \
		}\
		pStruCur = pStruCur->pStruNext; \
	} \
}while(0)


#endif
