#ifndef  __WB_HARDWARESUPPORT_H
#define  __WB_HARDWARESUPPORT_H

#include "WB_pirSupport.h"
#include "WB_doorCard.h"
#include "WB_keyboard.h"
#include "hwInterface/gpioServer.h"
#include "hwInterface/hwInterfaceManage.h"

typedef enum{
	CLOSE = 0,
	OPEN = 1,
}ControlCmd;

typedef enum{
	YES = 0,
	NO
}StateCmd;
typedef void(*UP_FUNC)(void *);
typedef struct  WB_hardWareOps{
	//控制开门关
	int (*controlDoor)(struct  WB_hardWareOps *,ControlCmd);
	//控制红外摄像头灯开关
	int (*controlIFCameraLight)(struct  WB_hardWareOps *,ControlCmd);
	//控制普通摄像头开关
	int (*controlCameraLight)(struct  WB_hardWareOps *,ControlCmd);
	//控制键盘灯开关
	int (*controlKeyboardLight)(struct  WB_hardWareOps *,ControlCmd);
	//控制LCD背光灯开关
	int (*controlLCDLight)(struct  WB_hardWareOps *,ControlCmd);
	//上层直接发送shell脚本指令
	int (*sendShellCmd)(struct  WB_hardWareOps *,const char *);
	//设备重启
	int (*reboot )(struct  WB_hardWareOps *);
	//设置人体感应回调函数
	int (*setPirUpFunc)(struct  WB_hardWareOps *,WBPirCallBackFunc);
	//设置开门按键触发回调函数
	int (*setOpenDoorKeyUpFunc)(struct  WB_hardWareOps *,T_InterruptFunc);
	//获取光感状态
	int (*getOptoSensorState)(struct  WB_hardWareOps *ops );
	//设置IC卡数据回调函数
	int (*setDoorCardRawUpFunc)(struct  WB_hardWareOps *,DoorCardRecvFunc);
	//设置键盘事件上报函数
	int (*setKeyboardEventUpFunc)(struct  WB_hardWareOps *,KeyEventUpFunc);
	//设置需要守护app的包名和主类名
	int (*setGuardPackagenameAndMainclassname)(struct  WB_hardWareOps *,const char *,const char * );
	//删除守护服务
	int (*delGuardServer)(struct  WB_hardWareOps *);

}WB_hardWareOps,*pWB_hardWareOps;
//创建硬件服务对象
pWB_hardWareOps crateHardWareServer(void);
//销毁硬件服务对象
void destroyHardWareServer(pWB_hardWareOps *ops);

#endif





















