#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "WB_hardwareSupport.h"
#include "WB_doorCard.h"
#include "WB_pirSupport.h"
#include "common/nativeNetServer.h"
#include "common/Utils.h"
#include "hwInterface/gpioServer.h"
#include "hwInterface/hwInterfaceManage.h"
#include "common/debugLog.h"
#include "binder/binderClient.h"
#include "WB_guardThread.h"
#include "WB_rs485.h"

typedef struct WB_hardWareServer {
	WB_hardWareOps ops;
	pGpioOps doorServer;
	pGpioOps redLedServer;
	pGpioOps greenLedServer;
	pGpioOps blueLedServer;
	pGpioOps openDoorKeyServer;
	pGpioOps doorMagneticServer; //门磁
	pGpioOps preventSeparateServer; //防拆
	pGpioOps optoSensorServer;
	pGpioOps IFCamerLightServer;
	pGpioOps LCDLightServer;
	pGpioOps camerLightServer;
	pGpioOps keyboardLightServer;
#if USER_BINDER == 1
	pBinderClientOps binderClient;
#else
	pNativeNetServerOps netClient;
#endif
	pDoorCardops doorCardServer;
	pWBPir_ops pirServer;
	pHwInterfaceOps interfaceOps;
	pWB_KeyBoardOps keyBoardServer;
	pGuardThreadOps guardThreadServer;
	pBluetoothOps bluetoothServer;
	pRs485Ops rs485Server;

	pTemperatureDetectionOps temperatureDetectionServer;

} WB_hardWareServer, *pWB_hardWareServer;

static int controlDoor(struct WB_hardWareOps *, ControlCmd);
static int controlLCDLight(struct WB_hardWareOps *, ControlCmd);
static int controlIFCameraLight(struct WB_hardWareOps *, ControlCmd);
static int controlCameraLight(struct WB_hardWareOps *, ControlCmd);
static int controlKeyboardLight(struct WB_hardWareOps *, ControlCmd);
static int sendShellCmd(struct WB_hardWareOps *, const char *);
static int reboot(struct WB_hardWareOps *);
static int setPirUpFunc(struct WB_hardWareOps * ops,
		WBPirCallBackFunc pirUpFunc);
static int setOpenDoorKeyUpFunc(struct WB_hardWareOps *, T_InterruptFunc);
static int setMagneticUpFunc(struct WB_hardWareOps *, T_InterruptFunc);
static int setPreventSeparateServerUpFunc(struct WB_hardWareOps *,
		T_InterruptFunc);
static int setOptoSensorUpFunc(struct WB_hardWareOps *, T_InterruptFunc);
static int setDoorCardRawUpFunc(struct WB_hardWareOps * ops,
		DoorCardRecvFunc rawUpFunc);
static int getIcCardState(struct WB_hardWareOps * ops);
static int getOptoSensorState(struct WB_hardWareOps *ops);
static int setKeyboardEventUpFunc(struct WB_hardWareOps * ops,
		KeyEventUpFunc func);
static int setGuardPackagenameAndMainclassname(struct WB_hardWareOps * ops,
		const char *packName, const char * className);
static int sendHearBeatToServer(struct WB_hardWareOps * ops,
		const char *packName, const char * className);
static int delGuardServer(struct WB_hardWareOps *ops);
static int setBluetoothRecvFunc(struct WB_hardWareOps *ops,
		T_bluetoothRecvFunc callback);
static int getBluetoothState(struct WB_hardWareOps *ops);
static int setBluetoothName(struct WB_hardWareOps *ops, char * name);
static int sendBluetoothStr(struct WB_hardWareOps *ops, char * str);
static int setbluetoothReboot(struct WB_hardWareOps *ops);
static int controlRGB(struct WB_hardWareOps *ops, LED_TYPE led, int state);
static int rs485Init(struct WB_hardWareOps *ops, int nBaudRate, int nDataBits,
		int nStopBits, int nParity);
static int rs485SendMsg(struct WB_hardWareOps *ops, char *data, int len);
static int rs485RecvMsg(struct WB_hardWareOps *ops, int timeout, char *data,
		int len);

/*功能:设置温度补偿系数
		 *  参数:
		 * 		ops:操作对象
		 * 		parameter:补偿系数 范围(0.93-1.0)
		 *
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
static int setTemperatureCompensation(struct  WB_hardWareOps *ops,float parameter);

		/*功能:获取所有像素点温度
		 *  参数:
		 * 		ops:操作对象
		 * 		data:数据指针
		 * 		len:数据长度 <=1024
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
static int getGlobalTemperature(struct  WB_hardWareOps *ops, float *data,int len);
		/*功能:获取特殊温度值
		 *  参数:
		 * 		centre:坐标中心温度
		 * 		max:最大值
		 * 		mini:最小值
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
static int getSpecialTemperature(struct  WB_hardWareOps *ops,float  *centre,float *max,float *mini);








static WB_hardWareOps ops = { .controlDoor = controlDoor,
		.controlIFCameraLight = controlIFCameraLight, .controlCameraLight =
				controlCameraLight,
		.controlKeyboardLight = controlKeyboardLight, .controlLCDLight =
				controlLCDLight, .sendShellCmd = sendShellCmd, .reboot = reboot,
		.setPirUpFunc = setPirUpFunc, .setOpenDoorKeyUpFunc =
				setOpenDoorKeyUpFunc, .getOptoSensorState = getOptoSensorState,
		.setDoorCardRawUpFunc = setDoorCardRawUpFunc, .getIcCardState =
				getIcCardState,
		.setKeyboardEventUpFunc = setKeyboardEventUpFunc, .setMagneticUpFunc =
				setMagneticUpFunc, .setPreventSeparateServerUpFunc =
				setPreventSeparateServerUpFunc,
		.setGuardPackagenameAndMainclassname =
				setGuardPackagenameAndMainclassname,
		.sendHearBeatToServer = sendHearBeatToServer,
		.delGuardServer =
				delGuardServer, .setBluetoothRecvFunc = setBluetoothRecvFunc,
		.getBluetoothState = getBluetoothState, .setBluetoothName =
				setBluetoothName, .sendBluetoothStr = sendBluetoothStr,
		.setbluetoothReboot = setbluetoothReboot, .rs485Init = rs485Init,
		.rs485SendMsg = rs485SendMsg, .rs485RecvMsg = rs485RecvMsg,
		.controlRGB = controlRGB,
		.setTemperatureCompensation = setTemperatureCompensation,
		.getGlobalTemperature = getGlobalTemperature,
		.getSpecialTemperature = getSpecialTemperature,
		};
static int rs485Init(struct WB_hardWareOps *ops, int nBaudRate, int nDataBits,
		int nStopBits, int nParity) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL) {
		goto fail0;
	}
	hardWareServer->rs485Server = createRs485Server(nBaudRate, nDataBits,
			nStopBits, nParity);
	if (hardWareServer->rs485Server == NULL) {
		goto fail0;
	}
	return 0;
	fail0: return -1;
}
static int controlRGB(struct WB_hardWareOps *ops, LED_TYPE led, int state) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL) {
		goto fail0;
	}

	switch (led) {
	case RED_LED:
		if (hardWareServer->redLedServer == NULL)
			goto fail0;
		LOGD("RED_LED : %d", state);
		return hardWareServer->redLedServer->setOutputValue(
				hardWareServer->redLedServer, state);
		break;
	case GREEN_LED:
		if (hardWareServer->greenLedServer == NULL)
			goto fail0;
		LOGD("GREEN_LED : %d", state);
		return hardWareServer->greenLedServer->setOutputValue(
				hardWareServer->greenLedServer, state);
		break;
	case BLUE_LED:
		if (hardWareServer->blueLedServer == NULL)
			goto fail0;
		LOGD("BLUE_LED : %d", state);
		return hardWareServer->blueLedServer->setOutputValue(
				hardWareServer->blueLedServer, state);
		break;
	default:
		break;
	}
	fail0: return -1;
}
static int rs485SendMsg(struct WB_hardWareOps *ops, char *data, int len) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	int sendRet;
	if (hardWareServer == NULL || hardWareServer->guardThreadServer == NULL) {
		goto fail0;
	}
	sendRet = hardWareServer->rs485Server->sendMsg(hardWareServer->rs485Server,
			data, len);
	return sendRet;
	fail0: return -1;
}
static int rs485RecvMsg(struct WB_hardWareOps *ops, int timeout, char *data,
		int len) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	int recvRet;
	if (hardWareServer == NULL || hardWareServer->guardThreadServer == NULL) {
		goto fail0;
	}
	recvRet = hardWareServer->rs485Server->recvMsg(hardWareServer->rs485Server,
			timeout, data, len);
	return recvRet;
	fail0: return -1;
}
/*功能:设置温度补偿系数
		 *  参数:
		 * 		ops:操作对象
		 * 		parameter:补偿系数 范围(0.93-1.0)
		 *
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
static int setTemperatureCompensation(struct  WB_hardWareOps *ops,float parameter){
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL || hardWareServer->temperatureDetectionServer  == NULL) {
		goto fail0;
	}


	return hardWareServer->temperatureDetectionServer->setTemperatureCompensation(hardWareServer->temperatureDetectionServer,parameter);


	fail0:
		return -1;
}

		/*功能:获取所有像素点温度
		 *  参数:
		 * 		ops:操作对象
		 * 		data:数据指针
		 * 		len:数据长度 <=1024
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
static int getGlobalTemperature(struct  WB_hardWareOps *ops, float *data,int len){
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL || hardWareServer->temperatureDetectionServer  == NULL) {
		goto fail0;
	}


	return hardWareServer->temperatureDetectionServer->getGlobalTemperature(hardWareServer->temperatureDetectionServer,data,len);
	fail0:
		return -1;
}
		/*功能:获取特殊温度值
		 *  参数:
		 * 		centre:坐标中心温度
		 * 		max:最大值
		 * 		mini:最小值
		 * 	返回值:
		 * 		0:成功 -1:失败
		 * */
static int getSpecialTemperature(struct  WB_hardWareOps *ops,float  *centre,float *max,float *mini){

	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL || hardWareServer->temperatureDetectionServer  == NULL) {
		goto fail0;
	}
	LOGD("getSpecialTemperature!\n");
	return hardWareServer->temperatureDetectionServer->getSpecialTemperature(hardWareServer->temperatureDetectionServer,centre,max,mini);
	fail0:
		return -1;

}


static int sendHearBeatToServer(struct WB_hardWareOps * ops,
		const char *packName, const char * className) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL || hardWareServer->guardThreadServer == NULL) {
		goto fail0;
	}
	return hardWareServer->guardThreadServer->sendHearBeatToServer(
			hardWareServer->guardThreadServer, packName, className, 6);
	fail0: return -1;
}
static int setGuardPackagenameAndMainclassname(struct WB_hardWareOps * ops,
		const char *packName, const char * className) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL || hardWareServer->guardThreadServer == NULL) {
		goto fail0;
	}
	return hardWareServer->guardThreadServer->setGuardPackagenameAndMainclassname(
			hardWareServer->guardThreadServer, packName, className, 6);
	fail0: return -1;
}
static int delGuardServer(struct WB_hardWareOps *ops) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL || hardWareServer->guardThreadServer == NULL) {
		goto fail0;
	}
	destroyGuardThreadServer(&hardWareServer->guardThreadServer);
	return 0;
	fail0: return -1;
}
static int setKeyboardEventUpFunc(struct WB_hardWareOps * ops,
		KeyEventUpFunc func) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL || hardWareServer->keyBoardServer == NULL)
		goto fail0;

	return hardWareServer->keyBoardServer->setKeyEventUpFunc(
			hardWareServer->keyBoardServer, func);
	fail0: return -1;
}
static int setDoorCardRawUpFunc(struct WB_hardWareOps * ops,
		DoorCardRecvFunc rawUpFunc) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	hardWareServer->doorCardServer = createDoorCardServer(rawUpFunc);
	if (hardWareServer->doorCardServer == NULL)
		return -1;
	return 0;
}

static int controlDoor(struct WB_hardWareOps * ops, ControlCmd cmd) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->doorServer != NULL) {
		return hardWareServer->doorServer->setOutputValue(
				hardWareServer->doorServer, cmd);
	}
	return -1;
}
static int controlLCDLight(struct WB_hardWareOps *ops, ControlCmd cmd) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->LCDLightServer != NULL) {
		return hardWareServer->LCDLightServer->setOutputValue(
				hardWareServer->LCDLightServer, cmd);
	}
	return -1;
}
static int controlIFCameraLight(struct WB_hardWareOps * ops, ControlCmd cmd) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->IFCamerLightServer != NULL) {
		return hardWareServer->IFCamerLightServer->setOutputValue(
				hardWareServer->IFCamerLightServer, cmd);
	}
	return -1;
}
static int controlCameraLight(struct WB_hardWareOps *ops, ControlCmd cmd) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->camerLightServer != NULL) {
		return hardWareServer->camerLightServer->setOutputValue(
				hardWareServer->camerLightServer, cmd);
	}
	return -1;

}
static int controlKeyboardLight(struct WB_hardWareOps *ops, ControlCmd cmd) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->keyboardLightServer != NULL
			&& hardWareServer->keyboardLightServer->setOutputValue != NULL) {
		return hardWareServer->keyboardLightServer->setOutputValue(
				hardWareServer->keyboardLightServer, cmd);
	}
	return -1;
}
static int sendShellCmd(struct WB_hardWareOps *ops, const char * cmd) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
#if USER_BINDER == 1
	if(hardWareServer->binderClient) {
		return hardWareServer->binderClient->runScript(
				hardWareServer->binderClient,cmd);
	} else
	{
		char cmdStr[128] = {0};
		sprintf(cmdStr,"su -c %s",cmd);
		LOGD("cmd:%s",cmdStr);
		return system(cmdStr);
	}
#else
	if (hardWareServer->netClient) {
		return hardWareServer->netClient->runScript(hardWareServer->netClient,
				cmd);
	} else {
		char cmdStr[128] = { 0 };
		sprintf(cmdStr, "su -c %s", cmd);
		LOGD("cmd:%s", cmdStr);
		return system(cmdStr);
	}
#endif

}
static int getIcCardState(struct WB_hardWareOps * ops) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->doorCardServer == NULL)
		return -1;
	return hardWareServer->doorCardServer->getState(
			hardWareServer->doorCardServer);
}
static int setBluetoothRecvFunc(struct WB_hardWareOps *ops,
		T_bluetoothRecvFunc callback) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->bluetoothServer == NULL)
		return -1;
	return hardWareServer->bluetoothServer->setRecvFunc(
			hardWareServer->bluetoothServer, callback);
}
static int getBluetoothState(struct WB_hardWareOps *ops) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->bluetoothServer == NULL)
		return -1;
	return hardWareServer->bluetoothServer->getState(
			hardWareServer->bluetoothServer);

}
static int setbluetoothReboot(struct WB_hardWareOps *ops) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->bluetoothServer == NULL)
		return -1;
	return hardWareServer->bluetoothServer->reboot(
			hardWareServer->bluetoothServer);
}
static int sendBluetoothStr(struct WB_hardWareOps *ops, char * str) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL || hardWareServer->bluetoothServer == NULL)
		return -1;
	return hardWareServer->bluetoothServer->sendStr(
			hardWareServer->bluetoothServer, str);

}
static int setBluetoothName(struct WB_hardWareOps *ops, char * name) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	if (hardWareServer->bluetoothServer == NULL)
		return -1;
	return hardWareServer->bluetoothServer->setName(
			hardWareServer->bluetoothServer, name);
}

static int reboot(struct WB_hardWareOps * ops) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
#if USER_BINDER == 1
	if(hardWareServer->binderClient) {
		return hardWareServer->binderClient->runScript(
				hardWareServer->binderClient,"reboot");
	} else {
		return system("su -c reboot");
	}
#else
	if (hardWareServer->netClient) {
		return hardWareServer->netClient->runScript(hardWareServer->netClient,
				"reboot");
	} else {
		return system("su -c reboot");
	}
#endif
}
static int setPirUpFunc(struct WB_hardWareOps * ops,
		WBPirCallBackFunc pirUpFunc) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		return -1;
	hardWareServer->pirServer = crateWBPirServer(
			hardWareServer->interfaceOps->getPirPin(), pirUpFunc);
	if (hardWareServer->pirServer == NULL)
		return -1;
	return 0;
}
static int getOptoSensorState(struct WB_hardWareOps *ops) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer == NULL)
		goto fail0;
	if (hardWareServer->optoSensorServer == NULL)
		goto fail0;
	return hardWareServer->optoSensorServer->getInputValue(
			hardWareServer->optoSensorServer);
	fail0: return -1;
}

static int setMagneticUpFunc(struct WB_hardWareOps *ops,
		T_InterruptFunc magneticUpFunc) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	hardWareServer->doorMagneticServer = gpio_getServer(
			hardWareServer->interfaceOps->getDoorMagneticPin());
	if (hardWareServer->doorMagneticServer == NULL) {
		LOGE("fail to setMagneticUpFunc");
		return -1;
	}
	hardWareServer->doorMagneticServer->setInterruptFunc(
			hardWareServer->doorMagneticServer, magneticUpFunc, NULL, BOTH);
	return 0;
}
static int setPreventSeparateServerUpFunc(struct WB_hardWareOps *ops,
		T_InterruptFunc preventSeparateUpFunc) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;

	hardWareServer->preventSeparateServer = gpio_getServer(
			hardWareServer->interfaceOps->getSecurityPin());
	if (hardWareServer->preventSeparateServer == NULL) {
		LOGE("fail to setPreventSeparateServerUpFunc");
		return -1;
	}

	hardWareServer->preventSeparateServer->setInterruptFunc(
			hardWareServer->preventSeparateServer, preventSeparateUpFunc, NULL,
			BOTH);
	return 0;
}

static int setOpenDoorKeyUpFunc(struct WB_hardWareOps * ops,
		T_InterruptFunc OpenDoorKeyUpFunc) {

	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;
	if (hardWareServer->openDoorKeyServer == NULL) {
		hardWareServer->openDoorKeyServer = gpio_getServer(
				hardWareServer->interfaceOps->getOpenDoorKeyPin());
		if (hardWareServer->openDoorKeyServer == NULL)
			return -1;
	}
	hardWareServer->openDoorKeyServer->setInterruptFunc(
			hardWareServer->openDoorKeyServer, OpenDoorKeyUpFunc, NULL, BOTH);
	return 0;
}
static int setOptoSensorUpFunc(struct WB_hardWareOps *ops,
		T_InterruptFunc optoSensorUpFunc) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) ops;

	if (hardWareServer->optoSensorServer == NULL)
		return -1;
	hardWareServer->optoSensorServer->setInterruptFunc(
			hardWareServer->optoSensorServer, optoSensorUpFunc, NULL, BOTH);

	return 0;
}
pWB_hardWareOps crateHardWareServer(void) {
	pWB_hardWareServer hardWareServer = malloc(sizeof(WB_hardWareServer));
	if (hardWareServer == NULL) {
		LOGE("fail to malloc hardWareServer!");
		goto fail0;
	}
	bzero(hardWareServer, sizeof(WB_hardWareServer));

	hardWareServer->interfaceOps = crateHwInterfaceServer();
#if 1
	hardWareServer->doorServer = gpio_getServer(
			hardWareServer->interfaceOps->getDoorLockPin());
	if (hardWareServer->doorServer == NULL) {
		LOGE("fail to malloc doorLockServer!");
		goto fail0;
	}
	hardWareServer->openDoorKeyServer = gpio_getServer(
			hardWareServer->interfaceOps->getOpenDoorKeyPin());
	if (hardWareServer->openDoorKeyServer == NULL) {
		LOGE("fail to malloc openDoorKeyServer!");
		if (getUtilsOps()->getCpuVer() != RK3368)
			goto fail0;
	}
	hardWareServer->IFCamerLightServer = gpio_getServer(
			hardWareServer->interfaceOps->getIFCameraLightPin());
	/*
	if (hardWareServer->IFCamerLightServer == NULL) {
		LOGE("fail to malloc IFCamerLightServer!");
		if (getUtilsOps()->getCpuVer() != RK3368)
			goto fail3;
	}
	*/
	hardWareServer->keyboardLightServer = gpio_getServer(
			hardWareServer->interfaceOps->getKeyLightPin());
	if (hardWareServer->keyboardLightServer == NULL) {
		LOGE("fail to malloc IFCamerLightServer!");
		if (getUtilsOps()->getCpuVer() != RK3368)
			goto fail0;
	}
	hardWareServer->camerLightServer = gpio_getServer(
			hardWareServer->interfaceOps->getCameraLightPin());
	if (hardWareServer->camerLightServer == NULL) {
		LOGE("fail to malloc camerLightServer!");
		if (getUtilsOps()->getCpuVer() != RK3368)
			goto fail0;
	}
	hardWareServer->LCDLightServer = gpio_getServer(
			hardWareServer->interfaceOps->getLcdSwichPin());
	if (hardWareServer->LCDLightServer == NULL) {
		LOGE("fail to malloc LCDLightServer! cpu:[%d]",getUtilsOps()->getCpuVer());
		if (getUtilsOps()->getCpuVer() != RK3368 && getUtilsOps()->getCpuVer() != RK3288 &&
				getUtilsOps()->getCpuVer() != RK3128)
			goto fail0;
	}
	hardWareServer->optoSensorServer = gpio_getServer(
			hardWareServer->interfaceOps->getLightSensorPin());
	if (hardWareServer->optoSensorServer == NULL) {
		LOGE("fail to malloc getLightSensorPin!");
		if (getUtilsOps()->getCpuVer() != RK3368)
			goto fail0;
	}
	hardWareServer->keyBoardServer = createKeyBoardServer("dev/input/event");
	if (hardWareServer->keyBoardServer == NULL) {
		goto fail0;
	}




#if	USER_BLUETOOTH
	hardWareServer->bluetoothServer = createBluetoothServer();
	if(hardWareServer->bluetoothServer == NULL)
	{
		LOGW("fail to createBluetoothServer!");
	}
#endif

#endif
#if USER_BINDER == 1
	hardWareServer->binderClient = binder_getServer();
	if(hardWareServer->binderClient == NULL) {
		LOGE("fail to malloc binderClient!");
	} else {
		hardWareServer->guardThreadServer = createGuardThreadServer();
		if(hardWareServer->guardThreadServer == NULL)
		{
			LOGE("fail to guardThreadServer!");
		}
	}
#else
	hardWareServer->netClient = createNativeNetServer();
	if (hardWareServer->netClient == NULL) {
		LOGE("fail to malloc netClient!");
	} else {
		hardWareServer->guardThreadServer = createGuardThreadServer();
		if (hardWareServer->guardThreadServer == NULL) {
			LOGE("fail to guardThreadServer!");
			goto fail0;
		}
	}
#endif
	hardWareServer->redLedServer = gpio_getServer(
			hardWareServer->interfaceOps->getRedLedPin());
	hardWareServer->greenLedServer = gpio_getServer(
			hardWareServer->interfaceOps->getGreenLedPin());
	hardWareServer->blueLedServer = gpio_getServer(
			hardWareServer->interfaceOps->getBlueLedPin());


	hardWareServer->temperatureDetectionServer = createTemperatureDetectionServer(crateHwInterfaceServer()->getTemperatureDetectionUART());
	hardWareServer->ops = ops;
	return (pWB_hardWareOps) hardWareServer;
	fail0:
		destroyHardWareServer(&hardWareServer);
	return NULL;
}

void destroyHardWareServer(pWB_hardWareOps *ops) {
	pWB_hardWareServer hardWareServer = (pWB_hardWareServer) *ops;
	if (hardWareServer == NULL)
		return;
	if (hardWareServer->keyboardLightServer) {
		LOGE("destroyHardWareServer keyboardLightServer");
		gpio_releaseServer(&hardWareServer->keyboardLightServer);
	}
	if (hardWareServer->camerLightServer) {
		LOGE("destroyHardWareServer camerLightServer");
		gpio_releaseServer(&hardWareServer->camerLightServer);
	}
#if 1
	if (hardWareServer->IFCamerLightServer) {
		LOGE("destroyHardWareServer IFCamerLightServer");
		gpio_releaseServer(&hardWareServer->IFCamerLightServer);
	}
#endif
	if (hardWareServer->openDoorKeyServer) {
		LOGE("destroyHardWareServer openDoorKeyServer");
		gpio_releaseServer(&hardWareServer->openDoorKeyServer);
	}
	if (hardWareServer->doorServer) {
		LOGE("destroyHardWareServer doorServer");
		gpio_releaseServer(&hardWareServer->doorServer);
	}
	if (hardWareServer->pirServer) {
		LOGE("destroyHardWareServer pirServer");
		destroyWBPirServer(&hardWareServer->pirServer);
	}
	if (hardWareServer->doorCardServer) {
		LOGE("destroyHardWareServer doorCardServer");
		destroyDoorCardServer(&hardWareServer->doorCardServer);
	}
	if (hardWareServer->keyBoardServer) {
		LOGE("destroyHardWareServer keyBoardServer");
		destroyKeyBoardServer(&hardWareServer->keyBoardServer);
	}
#if USER_BINDER == 1
	if(hardWareServer->binderClient)
	binder_releaseServer(&hardWareServer->binderClient);
#else
	if (hardWareServer->netClient)
		destroyNativeNetServer(&hardWareServer->netClient);
#endif
	if (hardWareServer->guardThreadServer)
		destroyGuardThreadServer(&hardWareServer->guardThreadServer);
	if(hardWareServer->temperatureDetectionServer){

		destroyTemperatureDetectionServer(&hardWareServer->temperatureDetectionServer);
	}
	free(hardWareServer);
	*ops = NULL;
}

