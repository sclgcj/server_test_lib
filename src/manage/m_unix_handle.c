#include "manage.h"
#include "m_json_comm.h"
#include "m_unix_handle.h"

static void
m_unix_test_response(
	char *sTmp,
	MLink *pStruML
)
{
	int iLen = 0;
	char *sRes = "{\"Result\": 0, \"data\":\"%s\"}";
	char sSendData[512] = { 0 };

	sprintf(sSendData, sRes, sTmp);
	
	m_send_data(pStruML->iSockfd, sSendData);
	ML_ERROR("\n");
}

int
m_unix_test(
	cJSON *pStruRoot,
	void *pData
)
{
	char *sTmp = NULL;
	MLink *pStruML = (MLink*)pData;

	if( !pData )
	{
		return ML_PARAM_ERR;
	}

	m_json_get_object_str_malloc("data", pStruRoot, &sTmp);
	ML_ERROR("sTmp = %s\n", sTmp);

	m_unix_test_response(sTmp, pStruML);

	ML_FREE(sTmp);
	ML_ERROR("\n");
	return ML_OK;
}
