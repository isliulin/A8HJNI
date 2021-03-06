#ifndef  __WB_VIRTUAL_HARDWARESUPPORT_H
#define  __WB_VIRTUAL_HARDWARESUPPORT_H
#include "WB_pirSupport.h"
#include "WB_doorCard.h"
#include "WB_keyboard.h"
#include "hwInterface/gpioServer.h"
#include "hwInterface/hwInterfaceManage.h"



typedef struct VirtualHWops{
		//设置人体感应回调函数
		int (*setPirUpFunc)(struct  VirtualHWops *,WBPirCallBackFunc);
		//设置开门按键触发回调函数
		int (*setOpenDoorKeyUpFunc)(struct  VirtualHWops *,T_InterruptFunc);
		//设置光感回调
		int (*setOptoSensorUpFunc)(struct  VirtualHWops *,T_InterruptFunc);
		//设置IC卡数据回调函数
		int (*setDoorCardRawUpFunc)(struct  VirtualHWops *,DoorCardRecvFunc);
		int (*setKeyBoardUpFunc)(struct  VirtualHWops *,KeyEventUpFunc);
}VirtualHWops,*pVirtualHWops;

pVirtualHWops crateVirtualHWServer(void);
void destroyVirtualHWServer(pVirtualHWops *server);



#endif
