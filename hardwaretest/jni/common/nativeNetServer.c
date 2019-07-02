#include <string.h>
#include <strings.h>
#include "common/nativeNetServer.h"
#include "common/netUdpServer.h"
#include "common/debugLog.h"
#include "common/Utils.h"


#define SERVER_PORT 	10800
#define CLIENT_PORT 	10900
#define SERVER_IP_ADDR   (inet_addr("127.0.0.1"))

//应答类命令字
#define NOT_ACK				0XAA
#define ACK_OK				0X61
#define ACK_ERR				0X62
//控制命令字
#define  CMD_CONNECT_SCRIPT 0X00
#define  CMD_RUN_SCRIPT		0X10
#define  CMD_HEARTBEAT      0X11
typedef struct {
#define UDP_BUF_MAXSIZE 1024
	unsigned char buf[UDP_BUF_MAXSIZE];
	int len;
} MsgBody, *pMsgBody;

typedef struct NativeNetServer {
	NativeNetServerOps ops;
	pUdpOps udpServer;
} NativeNetServer, *pNativeNetServer;
typedef struct T_Data_Head {
	unsigned int sourceID;
	unsigned int desID;
	unsigned int dtatLen; //高字节在前
	unsigned char dataStart;
} T_Data_Head, *pT_Data_Head;
typedef struct T_Comm_Head {
	unsigned char ackType;
	unsigned char cmd;
	unsigned char sequence;
	T_Data_Head dataPack;
} T_Comm_Head, *pT_Comm_Head;
static int sendHeartbeat(struct NativeNetServerOps *ops, const char *str);
static int runScript(struct NativeNetServerOps * ops, const char * str);
static NativeNetServerOps ops = {
		.runScript = runScript,
		.sendHeartbeat = sendHeartbeat,
};
static MsgBody UdpBuildMsg(unsigned char ackType, unsigned char cmd,
		const unsigned char *pData, int dataLen) {
	MsgBody tmpMsg;
	//包结构长度+数据长度+校验位
	int totalLen = sizeof(T_Comm_Head) + dataLen;
	T_Comm_Head * pHead = (T_Comm_Head *) tmpMsg.buf;
	static unsigned char seq = 0;
	pHead->ackType = ackType;
	pHead->cmd = cmd;
	pHead->sequence = seq++;
	if (pData != NULL && dataLen > 0) {
		memcpy(&(pHead->dataPack.dataStart), pData, dataLen);
		pHead->dataPack.dtatLen = htonl(dataLen);
	}
	//最后一位是校验位
	tmpMsg.buf[totalLen - 1] = getUtilsOps()->NByteCrc8(0,
			(unsigned char *) pHead, totalLen - 1);
	tmpMsg.len = totalLen;
	return tmpMsg;
}

static int runScript(struct NativeNetServerOps * ops, const char * str) {
	pNativeNetServer pthis = (pNativeNetServer) ops;

	LOGD(" runScript[%s]!",str);
	int ret  =-1;
	if (pthis == NULL || pthis->udpServer == NULL) {
		return -1;
	}
	char recvbuf[1024] = {0};
	MsgBody msg = UdpBuildMsg(NOT_ACK, CMD_RUN_SCRIPT, str, strlen(str) + 1);

	ret = pthis->udpServer->write(pthis->udpServer, msg.buf, msg.len,
			SERVER_IP_ADDR, SERVER_PORT);
	if(ret < 0){
		LOGE("[%s] fail to write!",str);
		goto fail0;
	}

	ret = pthis->udpServer->read(pthis->udpServer,recvbuf,sizeof(recvbuf),1000);
	if(ret < 0){
		LOGE("[%s] fail to read!",str);
		goto fail0;
	}
	pT_Comm_Head ackComm =  &recvbuf;
	LOGD("ackComm.dataPack.dataStart : %d\n",ackComm->dataPack.dataStart);
	return ackComm->dataPack.dataStart;
fail0:
	LOGE(" fail to runScript!");
	return -1;

}
static int sendHeartbeat(struct NativeNetServerOps *ops, const char *str) {
	pNativeNetServer pthis = (pNativeNetServer) ops;
	if (pthis == NULL || pthis->udpServer == NULL) {
		return -1;
	}
	MsgBody msg = UdpBuildMsg(NOT_ACK, CMD_HEARTBEAT, str, strlen(str) + 1);
	return pthis->udpServer->write(pthis->udpServer, msg.buf, msg.len,
			SERVER_IP_ADDR, SERVER_PORT);
}

pNativeNetServerOps createNativeNetServer(void) {
	int time_out = 0,ret;
	char recvbuf[100] = {0};
	pNativeNetServer server = malloc(sizeof(NativeNetServer));
	if (server == NULL) {
		LOGE("fail to  malloc pNativeNetServer");
		goto fail0;
	}
	bzero(server,sizeof(NativeNetServer));
	server->udpServer = createUdpServer(0);
	if (server->udpServer == NULL) {
		LOGE("fail to createUdpServer");
		goto fail1;
	}
	server->ops = ops;
#if 1
	int tryCount = 8;
	do{
		MsgBody msg = UdpBuildMsg(NOT_ACK, CMD_CONNECT_SCRIPT, NULL, 0);
		server->udpServer->write(server->udpServer, msg.buf, msg.len,
							SERVER_IP_ADDR, SERVER_PORT);
		ret = server->udpServer->read(server->udpServer,recvbuf,sizeof(recvbuf),1000);
		if((ret <= 0)&& (tryCount <=0))
		{
			goto fail1;
		}else {
			break;
		}
		tryCount --;
	}while(1);
#endif

	return (pNativeNetServerOps) server;
	fail1: free(server);
	fail0: return NULL;
}
void destroyNativeNetServer(pNativeNetServerOps *ops) {
	if (ops == NULL)
		return;
	pNativeNetServer server = (pNativeNetServer) (*ops);
	if (server == NULL) {
		return;
	}
	if(	server->udpServer)
	{
		destroyUdpServer(&server->udpServer);
	}
	free(server);
	*ops = NULL;
}

















