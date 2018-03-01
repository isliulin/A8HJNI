#include <linux/stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "WB_virtualHardwareSupport.h"
#include "common/netUdpServer.h"
#include "common/Utils.h"
//#include "common/nativeNetServer.h"
#include "common/debugLog.h"
#include "binder/binderClient.h"

typedef struct ThreadArg{
	void *data;
	int len;
}ThreadArg,*pThreadArg;
typedef void *(*ThreadFunc)(pThreadArg);

//应答类命令字
#define NOT_ACK				0XAA
#define ACK_OK				0X61
#define ACK_ERR				0X62
//控制命令字
#define  SIM_CALL_OUT		0X10
#define  SIM_SWIPNG_CARD    0X11
#define  SIM_PEOPHE_CLOSE   0X12



typedef struct T_Data_Head{
	unsigned int  sourceID;
	unsigned int  desID;
	unsigned int  dtatLen; //高字节在前
	unsigned char dataStart;
}T_Data_Head,*pT_Data_Head;


typedef struct T_Comm_Head {
	unsigned char ackType;
	unsigned char cmd;
	unsigned char sequence;
	T_Data_Head   dataStart;
} T_Comm_Head, *pT_Comm_Head;

typedef struct {
	#define UDP_BUF_MAXSIZE  (1024*4)
	unsigned char buf[UDP_BUF_MAXSIZE];
	int len;
	int offset;
} MsgBody, *pMsgBody;


typedef struct {
	VirtualHWops ops;
	pUdpOps udpServer;

	//pNativeNetServerOps netServer;
	pBinderClientOps binderClient;
    WBPirCallBackFunc pirCallBackFunc;
    T_InterruptFunc	 openDoorKeyUpFunc;
    T_InterruptFunc  optoSensorUpFunc;
    DoorCardRecvFunc doorCardRawUpFunc;
    KeyEventUpFunc   keyboardFunc;

}VirtualHWServer, *pVirtualHWServer;

static int setPirUpFunc
			   (struct  VirtualHWops * ops,WBPirCallBackFunc pirCallBack);
static int setOpenDoorKeyUpFunc
				(struct  VirtualHWops * ops ,T_InterruptFunc openKeyCallBack);
static int setOptoSensorUpFunc
				(struct  VirtualHWops * ops ,T_InterruptFunc optoSensorCallBack );
static int setDoorCardRawUpFunc
			(struct  VirtualHWops *ops ,DoorCardRecvFunc ICCardALGCallBack);
static int setKeyBoardUpFunc(struct  VirtualHWops *ops,KeyEventUpFunc keyboardFunc);

static  int udpRecvFunc(unsigned char* data ,unsigned int size);
static int _dialing(char num);
static int _getKeyNum(char ch);
static startThreadwork(ThreadFunc func,pThreadArg arg );
static void * do_callout(pThreadArg arg);
static int callout( int  callTimeS);
static int swipngCard(char *data,int len );

static void * do_peopheClose(pThreadArg arg);

static VirtualHWops ops = {
		.setDoorCardRawUpFunc = setDoorCardRawUpFunc,
		.setPirUpFunc = setPirUpFunc,
		.setOpenDoorKeyUpFunc = setOpenDoorKeyUpFunc,
		.setOptoSensorUpFunc = setOptoSensorUpFunc,
		.setKeyBoardUpFunc = setKeyBoardUpFunc,
};
static pVirtualHWServer vHWServer;

static int _getKeyNum(char ch)
{
	struct {
		char keych;
		int keynum;
	}KeyList[12] = {
			{'0',11},{'1',29},{'2',30},{'3',7},{'4',16},{'5',13},
			{'6',8},{'7',15},{'8',12},{'9',9},{'#',10},{'*',14}
	};
	int i;
	for(i = 0 ; i< sizeof(KeyList)/sizeof(KeyList[0]);i++)
	{
		if(KeyList[i].keych == ch  )
			return KeyList[i].keynum;
	}
	return -1;
}

static int _dialing(char num)
{
	char cmdStr[128] = {0};
	char cmdStrSu[128] = {0};
	int keyNum = _getKeyNum(num);
	if(keyNum < 0)
		return -1;
	bzero(cmdStr,sizeof(cmdStr));
	sprintf(cmdStr, "sendevent /dev/input/event3 1 %u 1",keyNum );
	if(vHWServer->binderClient)
	{
		vHWServer->binderClient->runScript(vHWServer->binderClient,cmdStr);
	}else{
		bzero(cmdStrSu,sizeof(cmdStrSu));
		sprintf(cmdStrSu,"su -c '%s'", cmdStr);
		system(cmdStrSu);

	}
	bzero(cmdStr,sizeof(cmdStr));
	sprintf(cmdStr, "sendevent /dev/input/event3 0 0 0" );
	if(vHWServer->binderClient)
	{
		vHWServer->binderClient->runScript(vHWServer->binderClient,cmdStr);
	}else{
		bzero(cmdStrSu,sizeof(cmdStrSu));
		sprintf(cmdStrSu,"su -c '%s'", cmdStr);
		system(cmdStrSu);

	}
	bzero(cmdStr,sizeof(cmdStr));
	sprintf(cmdStr, "sendevent /dev/input/event3 1 %u 0",keyNum );
	if(vHWServer->binderClient)
	{
		vHWServer->binderClient->runScript(vHWServer->binderClient,cmdStr);
	}else{
		bzero(cmdStrSu,sizeof(cmdStrSu));
		sprintf(cmdStrSu,"su -c '%s'", cmdStr);
		system(cmdStrSu);
	}
	bzero(cmdStr,sizeof(cmdStr));
	sprintf(cmdStr, "sendevent /dev/input/event3 0 0 0" );
	if(vHWServer->binderClient)
	{
		vHWServer->binderClient->runScript(vHWServer->binderClient,cmdStr);
	}else{
		bzero(cmdStrSu,sizeof(cmdStrSu));
		sprintf(cmdStrSu,"su -c '%s'", cmdStr);
		system(cmdStrSu);
	}
	return 0;
}

static startThreadwork(ThreadFunc func,pThreadArg arg )
{
	pthread_t  thread;
	if (pthread_create(&thread,NULL,func,
			(void *)arg) != 0) {
		return -1;
	}
	pthread_detach(thread);
	return 0;
}
static void * do_callout(pThreadArg arg)
{
		if(arg == NULL)
			return NULL;

		if(vHWServer != NULL)
		{
			vHWServer->keyboardFunc(5,1);
			vHWServer->keyboardFunc(5,0);
			sleep(1);
			vHWServer->keyboardFunc(30,1);
			vHWServer->keyboardFunc(30,0);
			sleep(1);
			vHWServer->keyboardFunc(5,1);
			vHWServer->keyboardFunc(5,0);
			sleep(1);
			vHWServer->keyboardFunc(30,1);
			vHWServer->keyboardFunc(30,0);
			sleep(1);
		}
		sleep(*(int*)(arg->data));
		vHWServer->keyboardFunc(4,1);
		vHWServer->keyboardFunc(4,0);
		if(arg->data != NULL)
			free(arg->data);
		free(arg);
		return NULL;
}
static int callout( int  callTimeS)
{
	pThreadArg arg = malloc(sizeof(ThreadArg));
	arg->data = malloc(sizeof(int));
	memcpy(arg->data,&callTimeS,sizeof(int));
	arg->len = sizeof(int);
	startThreadwork(do_callout,arg);
	return 0;
}

static int swipngCard(char *data,int len )
{

	if(vHWServer&&data!=NULL&&len>0)
		return vHWServer->doorCardRawUpFunc(IC_CARD,data,len);
	return -1;
}
static void * do_peopheClose(pThreadArg arg)
{

	 vHWServer->pirCallBackFunc(PIR_NEAR);
	 if(arg->data != NULL)
		 free(arg->data);
	 free(arg);
	 return NULL;

}
static int peopheClose(int timeS )
{
	pThreadArg arg = malloc(sizeof(ThreadArg));
	arg->data = malloc(sizeof(int));
	memcpy(arg->data,&timeS,sizeof(int));
	arg->len = sizeof(int);

	if( vHWServer&&vHWServer->pirCallBackFunc)
	{
		return startThreadwork(do_peopheClose,arg);
	}
	return -1;

}
static int setPirUpFunc
			   (struct  VirtualHWops * ops,WBPirCallBackFunc pirCallBack)
{
	pVirtualHWServer pthis = (pVirtualHWServer)ops;
	if(pthis == NULL )
		return -1;
	pthis->pirCallBackFunc = pirCallBack;
	return 0;
}
static int setOpenDoorKeyUpFunc
				(struct  VirtualHWops * ops ,T_InterruptFunc openKeyCallBack)
{
	pVirtualHWServer pthis = (pVirtualHWServer)ops;
	if(pthis == NULL )
		return -1;

	pthis->openDoorKeyUpFunc = openKeyCallBack;
	return 0;

}
static int setOptoSensorUpFunc
				(struct  VirtualHWops * ops ,T_InterruptFunc optoSensorCallBack )
{
	pVirtualHWServer pthis = (pVirtualHWServer)ops;
	if(pthis == NULL )
		return -1;
	pthis->optoSensorUpFunc = optoSensorCallBack;
	return 0;
}
static int setKeyBoardUpFunc(struct  VirtualHWops *ops,KeyEventUpFunc keyboardFunc)
{
	pVirtualHWServer pthis = (pVirtualHWServer)ops;
		if(pthis == NULL )
			return -1;
	pthis->keyboardFunc = keyboardFunc;
	return 0;
}
static int setDoorCardRawUpFunc
			(struct  VirtualHWops *ops ,DoorCardRecvFunc ICCardALGCallBack)
{
	pVirtualHWServer pthis = (pVirtualHWServer)ops;
	if(pthis == NULL )
		return -1;
	pthis->doorCardRawUpFunc = ICCardALGCallBack;
	return 0;
}

static  int udpRecvFunc(unsigned char* data ,unsigned int size)
{
	if(data == NULL || size< sizeof(T_Comm_Head))
		return size;
	T_Comm_Head * pHead = (T_Comm_Head *)data;
	getUtilsOps()->printData(data,size);
	pHead->dataStart.dtatLen = ntohl(pHead->dataStart.dtatLen);
	if(pHead->ackType == NOT_ACK)
	{
		switch(pHead->cmd)
		{
			case SIM_CALL_OUT:
				 LOGD("SIM_CALL_OUT time:%d",pHead->dataStart.dataStart);
				     callout(pHead->dataStart.dataStart);
				break;
			case SIM_SWIPNG_CARD:
				 LOGD("SIM_SWIPNG_CARD len:%d",pHead->dataStart.dtatLen);
					 swipngCard(&pHead->dataStart.dataStart,pHead->dataStart.dtatLen);
				break;
			case SIM_PEOPHE_CLOSE:
					 LOGD("SIM_PEOPHE_CLOSE time:%d",pHead->dataStart.dataStart);
				     peopheClose(pHead->dataStart.dataStart);
				break;
			default:
				break;
		}
	}
	return size;
}


static MsgBody UdpBuildMsg(unsigned char ackType, unsigned char cmd, const unsigned char *pData, int dataLen) {
	MsgBody tmpMsg;
	//包结构长度+数据长度+校验位
	int totalLen = sizeof(T_Comm_Head) + dataLen;
	T_Comm_Head * pHead = (T_Comm_Head *) tmpMsg.buf;
	static unsigned char seq = 0;
	pHead->ackType = ackType;
	pHead->cmd = cmd;
	pHead->sequence = seq++;
	if (pData != NULL && dataLen > 0) {
		memcpy(&(pHead->dataStart.dataStart), pData, dataLen);
		pHead->dataStart.dtatLen = htonl(dataLen);
	}
	//最后一位是校验位
	tmpMsg.buf[totalLen - 1] = getUtilsOps()->NByteCrc8(0, (unsigned char *) pHead, totalLen - 1);
	tmpMsg.len = totalLen;
	return tmpMsg;
}

pVirtualHWops crateVirtualHWServer(void)
{
	if(vHWServer)
		return (pVirtualHWops)vHWServer;
	vHWServer = malloc(sizeof(VirtualHWServer));
	if(vHWServer == NULL )
		goto fail0;
	vHWServer->udpServer =  createUdpServer(19999);
	if(vHWServer->udpServer == NULL )
		goto fail1;
	vHWServer->udpServer->setHandle(vHWServer->udpServer,udpRecvFunc,NULL,NULL);



	vHWServer->binderClient = binder_getServer();
	vHWServer->ops = ops;
	return (pVirtualHWops)vHWServer;
fail1:
	free(vHWServer);
fail0:
	return NULL;

}
void destroyVirtualHWServer(pVirtualHWops *server)
{

	if(vHWServer->binderClient)
		binder_releaseServer(&vHWServer->binderClient);
	if(vHWServer->udpServer)
		destroyUdpServer(&vHWServer->udpServer);
	free(vHWServer);
	*server = NULL;
}
