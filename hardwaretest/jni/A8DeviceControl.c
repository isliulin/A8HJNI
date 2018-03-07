#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <net/route.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <linux/stddef.h>
#include <jni.h>
#include "hwInterface/gpioServer.h"
#include "hwInterface/hwInterfaceManage.h"
#include "A8DeviceControl.h"
#include "common/debugLog.h"
#include "common/Utils.h"
#include "common/CallbackJavaMethod.h"
#include "common/netUdpServer.h"
#include "WB_hardwareSupport.h"
#include "WB_keyboard.h"
#include "WB_virtualHardwareSupport.h"
#include "WB_guardThread.h"
static pWB_hardWareOps hardWareServer;
static pVirtualHWops virtualHardWareServer;
static pJavaMethodOps JavaMethodServer;
static int icCardRecvFunc(CARD_TYPE type, unsigned char * data,
		unsigned int len);
static int openDoorKeyUp(pGpioPinState state);
static void pirUp(PIR_STATE state);
static int udpRecvFunc(unsigned char*data, unsigned int len);
static void KeyEventUp(int code, int value);
static int getpackAgeNameAndclassName(char *packAgeName, char *className,
		const char *local_Value);
JNIEXPORT jint JNICALL jni_a8HardwareControlInit(JNIEnv * env, jobject obj) {

	if (hardWareServer != NULL || JavaMethodServer != NULL)
		goto fail0;

	hardWareServer = crateHardWareServer();
	if (hardWareServer == NULL)
		goto fail0;
	virtualHardWareServer = crateVirtualHWServer();
	if (virtualHardWareServer == NULL) {
		LOGE("fail to crateVirtualHWServer!\n");
	} else {
		virtualHardWareServer->setPirUpFunc(virtualHardWareServer, pirUp);
		virtualHardWareServer->setDoorCardRawUpFunc(virtualHardWareServer,
				icCardRecvFunc);
		virtualHardWareServer->setKeyBoardUpFunc(virtualHardWareServer,
				KeyEventUp);
	}
	JavaMethodServer = CallbackJavaMethodInit(env, obj, "systemCallBack");
	if (JavaMethodServer == NULL)
		goto fail1;

	hardWareServer->setDoorCardRawUpFunc(hardWareServer, icCardRecvFunc);
	hardWareServer->setOpenDoorKeyUpFunc(hardWareServer, openDoorKeyUp);
	hardWareServer->setPirUpFunc(hardWareServer, pirUp);
	hardWareServer->setKeyboardEventUpFunc(hardWareServer, KeyEventUp);

	LOGD("jni_a8HardwareControlInit init succeed!");
	return 0;
	fail2: free(JavaMethodServer);
	fail1: free(hardWareServer);
	fail0: return -1;
}
static int getMapKeyCode(int code) {
	const int mapKeyCode[12] = { 5, 30, 48, 11, 10, 7, 2, 9, 6, 3, 8, 4 };
	int retCode;
	for (retCode = 0; retCode < sizeof(mapKeyCode) / sizeof(mapKeyCode[0]);
			retCode++) {
		if (mapKeyCode[retCode] == code)
			return retCode;
	}
	return code;
}
static void KeyEventUp(int code, int value) {
	char upData[6] = { 0 };

	if (code > 255) {
		LOGE("code value is too big");
		return;
	}
	LOGD("code:%d value:%d", code, value);
	upData[0] = UI_KEYBOARD_EVENT;
	upData[1] = getMapKeyCode(code);
	upData[2] = value;
	JavaMethodServer->up(JavaMethodServer, upData, 3);
}
static int udpRecvFunc(unsigned char* data, unsigned int len) {
	LOGE("data = %s", data);
	return 0;
}
static void pirUp(PIR_STATE state) {
	LOGE("pirUp :%d", state);
	char upData[6] = { 0 };
	upData[0] = UI_INFRARED_DEVICE;
	upData[1] = 1 - state;
	JavaMethodServer->up(JavaMethodServer, upData, 2);
}

static int openDoorKeyUp(pGpioPinState pinState) {
	LOGE("openDoorKeyUp");
	return 0;
}
static int icCardRecvFunc(CARD_TYPE type, unsigned char * data,
		unsigned int len) {
	getUtilsOps()->printData(data, len);
	char valid[128] = { 0 };

	union {
		char buf[sizeof(uint32_t)];
		uint32_t id;
	} cardNum;
	bzero(&cardNum, sizeof(cardNum));
	LOGD("type = %d\n", type);
	valid[0] = UI_DOORCARD_DEVICE;
	valid[1] = type;
	memcpy(&valid[2], data, len);
	JavaMethodServer->up(JavaMethodServer, valid, len + 2);
	getUtilsOps()->printData(data, len);
	getUtilsOps()->printHex(data, len);
	bzero(valid, sizeof(valid));
	valid[0] = UI_DOORCARD_DEVICE_ALG;
	valid[1] = type;
	getUtilsOps()->GetWeiGendCardId(data, len, &cardNum.id);
	memcpy(&valid[2], cardNum.buf, sizeof(cardNum.buf));

	JavaMethodServer->up(JavaMethodServer, valid, sizeof(cardNum.buf) + 2);
	return len;
}
static int getpackAgeNameAndclassName(char *packAgeName, char *className,
		const char *local_Value) {
	char* spaceStart = NULL;
	char* spaceEnd = NULL;
	if (strlen(local_Value) >= 256) {
		LOGE("fail to getpackAgeNameAndclassName");
		return -1;
	}

	spaceStart = strchr(local_Value, ' ');
	if (spaceStart == NULL)
		return -1;
	memcpy(packAgeName, local_Value, spaceStart - local_Value);
	packAgeName[spaceStart - local_Value] = 0;

	spaceEnd = strrchr(local_Value, ' ');
	strcpy(className, spaceEnd + 1);
	return 0;
}
JNIEXPORT jint JNICALL jni_a8HardwareControlExit(JNIEnv * env, jobject obj) {

	destroyHardWareServer(&hardWareServer);
	return 0;
}
JNIEXPORT jbyteArray JNICALL jni_a8GetKeyValue(JNIEnv * env, jobject obj,
		jint key) {

	unsigned char recvbuf[128] = { 0 };
	if (key <= 0) {
		return NULL;
	}
	switch (key) {
	case E_GET_HARDWARE_VER: {

		int recvLen = getUtilsOps()->getHardWareVer(recvbuf, sizeof(recvbuf));
		jbyteArray jarray = (*env)->NewByteArray(env, recvLen);

		if (recvLen > 0) {
			(*env)->SetByteArrayRegion(env, jarray, 0, recvLen,
					(jbyte*) recvbuf);
			return jarray;
		} else {
			return NULL;
		}
	}
	case E_GET_OPTO_SENSOR_STATE:{
		char state[1] = {0};
		state[0] = hardWareServer->getOptoSensorState(hardWareServer);

		jbyteArray jarray = (*env)->NewByteArray(env, 1);
		if (state[0] != -1) {
			(*env)->SetByteArrayRegion(env, jarray, 0, 1,
					(jbyte*) state);
			return jarray;
		} else {
			return NULL;
		}
	}
		break;
	}
}
JNIEXPORT jint JNICALL jni_a8SetKeyValue(JNIEnv *env, jobject obj, jint key,
		jbyteArray ValueBuf, jint ValueLen) {
	int ret = 0;
	int gpioValue = 0;
	char *local_Value = NULL;
	char data[1024 * 1024] = { UI_INFRARED_DEVICE, 1 };
	if (ValueLen > 0)
		local_Value = (char *) (*env)->GetByteArrayElements(env, ValueBuf,
				NULL);
	LOGD("Control Interface:%d  ValueLen:%d \n", key, ValueLen);
	switch (key) {

	case E_DOOEBEL:
		break; //有线门铃(西安郑楠项目)
	case E_SMART_HOME:
		break; //智能家居(西安郑楠项目)
	case E_DOOR_LOCK:
		if (local_Value != NULL)
			ret = hardWareServer->controlDoor(hardWareServer, local_Value[0]);
		break; //锁
	case E_INFRARED:
		break; //红外
	case E_CAMERA_LIGHT:
		if (local_Value != NULL)
			ret = hardWareServer->controlCameraLight(hardWareServer,
					local_Value[0]);
		break; //摄像头灯
	case E_KEY_LIGHT:
		if (local_Value != NULL)
			ret = hardWareServer->controlKeyboardLight(hardWareServer,
					local_Value[0]);
		break; //键盘灯
	case E_LCD_BACKLIGHT:
		if (local_Value != NULL)
			ret = hardWareServer->controlLCDLight(hardWareServer,
					local_Value[0]);
		break; //屏幕背光
	case E_FINGERPRINT:
		break;
	case E_SET_IPADDR:
		break;
	case E_RESTART:
		LOGD("E_RESTART!!");
		ret = hardWareServer->reboot(hardWareServer);
		break; //重启机器
	case E_SEND_SHELL_CMD:
		LOGD("E_SEND_SHELL_CMD:%s", local_Value);
		ret = hardWareServer->sendShellCmd(hardWareServer, local_Value);
		break;
	case E_ADD_GUARD: {
		LOGD("E_ADD_GUARD:%s", local_Value);
		char packAgeName[128] = { 0 };
		char className[128] = { 0 };

		ret = getpackAgeNameAndclassName(packAgeName, className, local_Value);
		if (ret == 0) {
			LOGD("%s:%s", packAgeName, className);
			hardWareServer->setGuardPackagenameAndMainclassname(hardWareServer,
					packAgeName, className);
		}
		break;
	}
	case E_DEL_GUARD: {
		hardWareServer->delGuardServer(hardWareServer);
	}
		break;
	default:
		LOGW("cannot find Control Interface!");
		break;
	}
	if (local_Value != NULL)
		exit: (*env)->ReleaseByteArrayElements(env, ValueBuf,
				(jbyte*) local_Value, 0);
	return ret;
}
