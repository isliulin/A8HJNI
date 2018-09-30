#include <stddef.h>
#include <malloc.h>
#include "WB_doorCard.h"
#include "hwInterface/hwInterfaceManage.h"
#include "cpuCard/cpu_card.h"
#include "icCard/fm1702nl.h"
#include "common/debugLog.h"

typedef struct {
	DoorCardops ops;
	void *cardClass;
	DOOR_CARD_MODULE type;
}DoorCardServer,*pDoorCardServer;
pDoorCardops createDoorCardServer(DoorCardRecvFunc callBackFunc) {
	pDoorCardServer server = (pDoorCardServer)calloc(1,sizeof(DoorCardServer));
	if(server == NULL)
		return NULL;
	DOOR_CARD_MODULE type = ZLG600A;
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
			break;
		}

	default:
		server->type = -1;
		LOGD("none card Server!\n");
		return NULL;
		break;
	}
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

