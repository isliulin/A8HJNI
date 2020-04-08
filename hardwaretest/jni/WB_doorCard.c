#include <stddef.h>
#include <malloc.h>
#include "WB_doorCard.h"
#include "hwInterface/hwInterfaceManage.h"
#include "cpuCard/cpu_card.h"
#include "icCard/fm1702nl.h"
#include "common/debugLog.h"
#include "common/Utils.h"

typedef struct {
	DoorCardops ops;
	void *cardClass;
	DOOR_CARD_MODULE type;
	int state;
}DoorCardServer,*pDoorCardServer;
static int getState(pDoorCardops ops);

static const DoorCardops ops = {
		.getState = getState,
};
static int getState(pDoorCardops ops)
{

	pDoorCardServer server = ops;
	return server->state;
}
pDoorCardops createDoorCardServer(DoorCardRecvFunc callBackFunc) {
	pDoorCardServer server = (pDoorCardServer)calloc(1,sizeof(DoorCardServer));
	if(server == NULL)
		return NULL;

	bzero(server,sizeof(DoorCardServer));
	DOOR_CARD_MODULE type ;

	if(getUtilsOps()->getCpuVer() == A20){
		type = FM1702NL;
	}else {
		type = ZLG600A;
	}
	switch (type) {

	case ZLG600A:
		LOGD("start ZLG600A!\n");
		server->cardClass = createZLG600AServer(
				crateHwInterfaceServer()->getDoorCardUART(),
				(RecvFunc) callBackFunc);
		if(server->cardClass == NULL)
		{
			LOGE("fail to server->cardClass!");
			return NULL;
		}else
		{
			LOGI("ZLG600A start succeed!");
			server->type = ZLG600A;
			server->state = 1;
		}
		break;
	case FM1702NL:
		LOGD("start FM1702NL!\n");
		server->cardClass = crateFM1702NLOpsServer(
				crateHwInterfaceServer()->getDoorCardUART(),
				callBackFunc);
		if(server->cardClass == NULL)
		{
			LOGE("fail to FM1702NL!");
		}else{
			LOGI("FM1702NL start succeed!");
			server->type = FM1702NL;
			server->state = 1;
			break;
		}
		break;
	default:
		server->type = -1;
		LOGD("none card Server!\n");
		return NULL;
		break;
	}
	server->ops =ops;
	return (pDoorCardops)server;
}
void destroyDoorCardServer(pDoorCardops *server) {
	if(server !=NULL && *server!=NULL)
		return ;

	pDoorCardServer pthis = (pDoorCardServer)*server;

	switch (pthis->type) {
	case FM1702NL:
		destroyFM1702NLOpsServer((pIcDoorCardOps*)&pthis->cardClass);
		break;
	case ZLG600A:
		destroyZLG600AServer((pIcDoorCardOps*)&pthis->cardClass);
		break;
	default:
		break;
	}
	free(pthis);
	*server = NULL;

}

