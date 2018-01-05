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
#include "gpio/gpioServer.h"
#include "A8DeviceControl.h"
#include "common/debugLog.h"
#include "common/Utils.h"
#include "common/CallbackJavaMethod.h"
#include "common/netUdpServer.h"
#include "WB_hardwareSupport.h"
#include "WB_keyboard.h"


static pWB_hardWareOps hardWareServer;
static pJavaMethodOps  JavaMethodServer;
static  int icCardRecvFunc(unsigned char * data,int len);
static  int openDoorKeyUp(pGpioPinState state);
static  int pirUp(PIR_STATE state);
static  int udpRecvFunc(unsigned char*data ,unsigned int len);
static  void KeyEventUp(int code ,int value);
JNIEXPORT jint JNICALL jni_a8HardwareControlInit(JNIEnv * env, jobject obj) {


	if(hardWareServer!= NULL||JavaMethodServer!= NULL )
		goto fail0;
	CPU_VER cpu_ver = getUtilsOps()->getCpuVer();

	hardWareServer =  crateHardWareServer(cpu_ver);
	if( hardWareServer == NULL)
		goto fail0;
	JavaMethodServer =  CallbackJavaMethodInit(env,obj,"systemCallBack");
	if(JavaMethodServer ==NULL )
		goto fail1;

	hardWareServer->setIcCardRawUpFunc(hardWareServer,icCardRecvFunc);
	hardWareServer->setOpenDoorKeyUpFunc(hardWareServer,openDoorKeyUp);
	hardWareServer->setPirUpFunc(hardWareServer,pirUp);
	hardWareServer->setKeyboardEventUpFunc(hardWareServer,KeyEventUp);
	LOGD("jni_a8HardwareControlInit init succeed!");
	return 0;
fail2:
	free(JavaMethodServer);
fail1:
	free(hardWareServer);
fail0:
	return -1;
}
static void KeyEventUp(int code ,int value)
{
	if(code > 255){
		LOGE("code value is too big");
		return ;
	}
	char upData[6] = {0};
	upData[0] = UI_KEYBOARD_EVENT;
	upData[1] = code;
	upData[2] = value;
	JavaMethodServer->up(JavaMethodServer,upData,3);
}
static int udpRecvFunc(unsigned char* data,unsigned int len)
{
	LOGE("data = %s",data);
}
static int pirUp(PIR_STATE state)
{
	char upData[6] = {0};
	upData[0] = UI_INFRARED_DEVICE;
	upData[1] = 1-state;
	JavaMethodServer->up(JavaMethodServer,upData,2);
	return 0;
}

static int openDoorKeyUp(pGpioPinState state)
{
	LOGE("OptoSensorUp:%s",state->state==0?"抬起":"按下" );
	return 0;
}
static  int icCardRecvFunc(unsigned char * data,int len)
{
	char valid[128] = {0};
	union {
 		char buf[sizeof(uint32_t)];
 		uint32_t id;
 	}cardNum;
 	bzero(&cardNum,sizeof(cardNum));

	valid[0] = UI_DOORCARD_DEVICE;
	memcpy(&valid[1],data,len);
	JavaMethodServer->up(JavaMethodServer,valid,len+1);

	bzero(valid,sizeof(valid));
	valid[0] = UI_DOORCARD_DEVICE_ALG;
	getUtilsOps()->GetWeiGendCardId(data,len,&cardNum.id);
	memcpy(&valid[1],cardNum.buf,sizeof(cardNum.buf));
	JavaMethodServer->up(JavaMethodServer,valid,sizeof(cardNum.buf)+1 );
	return len;
}
JNIEXPORT jint JNICALL jni_a8HardwareControlExit(JNIEnv * env, jobject obj) {

	destroyHardWareServer(&hardWareServer);
	return 0;
}
JNIEXPORT jbyteArray JNICALL jni_a8GetKeyValue(JNIEnv * env, jobject obj,jint key) {

	unsigned char recvbuf[128] = {0};


	if(key <=0)
	{
		return NULL;
	}
	switch(key)
	{
	case E_GET_HARDWARE_VER:{
		int  recvLen = getUtilsOps()->getHardWareVer(recvbuf,sizeof(recvbuf));
		jbyteArray jarray = (*env)->NewByteArray(env, recvLen);
		if(recvLen > 0 )
		{
			(*env)->SetByteArrayRegion(env, jarray, 0, recvLen,(jbyte*) recvbuf);
			 return jarray;
		}else{
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
	char *local_Value  = NULL;
	char data[1024*1024] = {UI_INFRARED_DEVICE,1};
	if(ValueLen > 0 )
		local_Value = (char *) (*env)->GetByteArrayElements(env, ValueBuf,NULL);
	LOGD("Control Interface:%d  ValueLen:%d \n", key, ValueLen);
	switch (key) {

		case E_DOOEBEL:
			break; //有线门铃(西安郑楠项目)
		case E_SMART_HOME:
			break; //智能家居(西安郑楠项目)
		case E_DOOR_LOCK:
			if(local_Value != NULL )
				hardWareServer->controlDoor(hardWareServer,local_Value[0]);
			break; //锁
		case E_INFRARED:
			break; //红外
		case E_CAMERA_LIGHT:
			if(local_Value != NULL )
				hardWareServer->controlCameraLight(hardWareServer,local_Value[0]);
			break; //摄像头灯
		case E_KEY_LIGHT:
			if(local_Value != NULL )
				hardWareServer->controlKeyboardLight(hardWareServer,local_Value[0]);
			break; //键盘灯
		case E_LCD_BACKLIGHT:
			if(local_Value != NULL )
				hardWareServer->controlLCDLight(hardWareServer,local_Value[0]);
			break; //屏幕背光
		case E_FINGERPRINT:
			break;
		case E_SET_IPADDR:
			break;
		case E_RESTART:
			LOGD("E_RESTART!!");
			hardWareServer->reboot(hardWareServer);
			break; //重启机器
		case E_SEND_SHELL_CMD:
			LOGD("E_SEND_SHELL_CMD:%s",local_Value);
			hardWareServer->sendShellCmd(hardWareServer,local_Value);
			break;
		default:
			LOGW("cannot find Control Interface!");
			break;
	}
	if(local_Value != NULL )
		exit: (*env)->ReleaseByteArrayElements(env, ValueBuf, (jbyte*) local_Value,0);
	return ret;
}
