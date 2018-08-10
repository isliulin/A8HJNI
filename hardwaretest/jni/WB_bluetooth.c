#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include "WB_bluetooth.h"
#include "hwInterface/hwInterfaceManage.h"
#include "common/debugLog.h"
#include "serial/serialServer.h"
#include "common/Utils.h"



static int bluetoothWriteAndWaitAck(pSerialOps serialServer,char *write,char* ackStr);
static int bluetoothServerInit(pSerialOps serialServer);
static int bluetoothsendStr(pBluetoothOps ops,char *str);

static int setName(pBluetoothOps ops,char* name);
static int setRecvFunc(pBluetoothOps ops ,T_bluetoothRecvFunc recvFunc);
static int getState(pBluetoothOps ops);

typedef struct BluetoothServer {
	BluetoothOps ops;
	pSerialOps serialServer;
}BluetoothServer,*pBluetoothServer;
static BluetoothOps ops = {
		.setName = setName,
		.setRecvFunc = setRecvFunc,
		.getState = getState,
		.sendStr = bluetoothsendStr,
};
static int setName(pBluetoothOps ops,char* name)
{
	pBluetoothServer server = (pBluetoothServer)ops;
	char cmdBuf[128]={0};
	int ret;
	if(server == NULL)
		goto fail0;
	if(server->serialServer == NULL)
	{
		goto fail0;
	}
	sprintf(cmdBuf,"AT+NAME=%s",name);
	server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_SYNC);
	ret = bluetoothWriteAndWaitAck(server->serialServer,cmdBuf,"OK");
	server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_ASYNC);

	if(ret < 0)
	{
		goto fail0;
	}
	return 0;
	fail0:
		return -1;
}
static int setRecvFunc(pBluetoothOps ops ,T_bluetoothRecvFunc recvFunc)
{
	pBluetoothServer server = (pBluetoothServer)ops;
	int ret;
	if(server == NULL)
		goto fail0;

	return server->serialServer->setHandle(server->serialServer,recvFunc,NULL,NULL);
fail0:
	return -1;
}

static int getState(pBluetoothOps ops)
{
	pBluetoothServer server = (pBluetoothServer)ops;
	int ret;
	if(server == NULL)
			goto fail0;
	server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_SYNC);
	ret =  bluetoothWriteAndWaitAck(server->serialServer,"AT","OK");
	server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_ASYNC);

	return ret;
	fail0:
		return -1;
}

static int bluetoothWriteAndWaitAck(pSerialOps serialServer,char *writeStr,char* ackStr)
{
	int ret;
	int timeoutCnt = 3;
	char readBuf[128] = {0};
	while(timeoutCnt-- >0)
	{
		ret = serialServer->write(serialServer,writeStr,strlen(writeStr));
		if(ret < 0)
		{
			usleep(100*1000);
			continue;
		}
		bzero(readBuf,sizeof(readBuf));
		ret = serialServer->read(serialServer,readBuf,sizeof(readBuf),500);
		if(ret <= 0)
		{
			LOGE(" fail to read \n");
			usleep(100*1000);
			continue;
		}else {
			if(0 == strncmp(ackStr,readBuf,strlen(ackStr)))
			{
				return 0;
			}
		}
	}
	return -1;
}
static int bluetoothsendStr(pBluetoothOps ops,char *str)
{
		int ret;
		int timeoutCnt = 3;
		char readBuf[128] = {0};
		LOGD("str:%s",str);
		pBluetoothServer server = (pBluetoothServer)ops;
		if(server == NULL)
			return -1;
		ret = server->serialServer->write(server->serialServer,str,strlen(str));
		return ret;
}
static int bluetoothServerInit(pSerialOps serialServer)
{
	int ret;
	ret  = bluetoothWriteAndWaitAck(serialServer,"AT","OK");
	if(ret < 0)
	{
		LOGE("bluetooth:fail to send AT!\n");
		goto fail0;
	}
	ret  = bluetoothWriteAndWaitAck(serialServer,"AT+ROLE=S","OK");
	if(ret < 0)
	{
		LOGE("bluetooth:fail to send AT+ROLE=S!\n");
		goto fail0;
	}
	ret  = bluetoothWriteAndWaitAck(serialServer,"AT+CONT=0","OK");
	if(ret < 0)
	{
		LOGE("bluetooth:fail to send AT+CONT=0!\n");
		goto fail0;
	}
	return 0;
fail0:
	return -1;
}
pBluetoothOps createBluetoothServer(void)
{
	int ret;
	pBluetoothServer server = (pBluetoothServer)malloc(sizeof(BluetoothServer));
	if(server == NULL)
	{
		goto fail0;
	}
	server->serialServer = createSerialServer( crateHwInterfaceServer()->getBluetoothUART(), 9600, 8, 1, 'n');
	if(server->serialServer == NULL)
	{
		goto fail1;
	}
	ret =  bluetoothServerInit(server->serialServer);
	if(ret < 0 )
	{
		goto fail1;
	}
	server->ops = ops;
	return (pBluetoothOps)server;
fail1:
	free(server);
	server = NULL;
fail0:
	return NULL;
}
void destroyBluetoothServer(pBluetoothOps *server)
{
	if(server == NULL || *server ==NULL)
	{
		return ;
	}
	pBluetoothServer pthis = (pBluetoothServer)*server;
	if(pthis->serialServer == NULL)
	{
		return;
	}
	destroySerialServer(&pthis->serialServer);
	*server = NULL;
}
