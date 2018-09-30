#ifndef WB_DOORCARD_H__
#define WB_DOORCARD_H__
#include "hwInterface/hwInterfaceManage.h"
typedef struct {


}DoorCardops,*pDoorCardops;

pDoorCardops createDoorCardServer(DoorCardRecvFunc callBackFunc);
void destroyDoorCardServer(pDoorCardops *server);






#endif
