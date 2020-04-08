#ifndef  __WB_HARDWARESUPPORT_H
#define  __WB_HARDWARESUPPORT_H

#include "WB_pirSupport.h"
#include "WB_doorCard.h"
#include "WB_keyboard.h"
#include "hwInterface/gpioServer.h"
#include "hwInterface/hwInterfaceManage.h"
#include "WB_bluetooth.h"
#include "WB_temperatureDetection.h"

typedef enum{
	CLOSE = 0,
	OPEN = 1,
}ControlCmd;

typedef enum{
	RED_LED = 0,
	GREEN_LED = 1,
	BLUE_LED = 2,

}LED_TYPE;

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
	//设置门磁回调函数
	int (*setMagneticUpFunc)(struct  WB_hardWareOps *,T_InterruptFunc);
	//设置防拆回调函数
	int (*setPreventSeparateServerUpFunc)(struct  WB_hardWareOps *,T_InterruptFunc);
	//获取光感状态
	int (*getOptoSensorState)(struct  WB_hardWareOps *ops );
	//设置IC卡数据回调函数
	int (*setDoorCardRawUpFunc)(struct  WB_hardWareOps *,DoorCardRecvFunc);
	//获取IC卡状态
	int (*getIcCardState)(struct  WB_hardWareOps *);
	//设置键盘事件上报函数
	int (*setKeyboardEventUpFunc)(struct  WB_hardWareOps *,KeyEventUpFunc);
	//设置需要守护app的包名和主类名
	int (*setGuardPackagenameAndMainclassname)(struct  WB_hardWareOps *,const char *,const char * );

	//发送心跳包给守护进程
	int (*sendHearBeatToServer)(struct  WB_hardWareOps *,const char *,const char * );

	//删除守护服务
	int (*delGuardServer)(struct  WB_hardWareOps *);

	//蓝牙相关
	//设置蓝牙回调接收函数
	int (*setBluetoothRecvFunc)(struct  WB_hardWareOps *,T_bluetoothRecvFunc);
	//获取蓝牙状态
	int (*getBluetoothState)(struct  WB_hardWareOps *);
	//设置蓝牙名
	int (*setBluetoothName)(struct  WB_hardWareOps *,char *);
	//发送蓝牙数据
	int (*sendBluetoothStr)(struct  WB_hardWareOps *,char *);
	//蓝牙重启
	int (*setbluetoothReboot)(struct  WB_hardWareOps *);

	//初始化RS485
	int (*rs485Init)(struct  WB_hardWareOps *,int,int ,int,int);
	//发送RS485消息
	int (*rs485SendMsg)(struct  WB_hardWareOps *,char *,int );
	//接收RS485消息
	int (*rs485RecvMsg)(struct  WB_hardWareOps *,int ,char *,int );
	//控制RGB灯
	int (*controlRGB)(struct  WB_hardWareOps *ops,LED_TYPE led,int state);

	/*功能:设置温度补偿系数
		 *  参数:
		 * 		ops:操作对象
		 * 		parameter:补偿系数 范围(0.93-1.0)
		 *
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
	int (*setTemperatureCompensation)(struct  WB_hardWareOps *ops,float parameter);

		/*功能:获取所有像素点温度
		 *  参数:
		 * 		ops:操作对象
		 * 		data:数据指针
		 * 		len:数据长度 <=1024
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
		int (*getGlobalTemperature)(struct  WB_hardWareOps *ops, float *data,int len);
		/*功能:获取特殊温度值
		 *  参数:
		 * 		centre:坐标中心温度
		 * 		max:最大值
		 * 		mini:最小值
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
		int (*getSpecialTemperature)(struct  WB_hardWareOps *ops,float  *centre,float *max,float *mini);



}WB_hardWareOps,*pWB_hardWareOps;
//创建硬件服务对象
pWB_hardWareOps crateHardWareServer(void);
//销毁硬件服务对象
void destroyHardWareServer(pWB_hardWareOps *ops);

#endif





















