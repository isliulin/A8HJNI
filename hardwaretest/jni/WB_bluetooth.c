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





typedef enum BLUETOOTH_TYPE {
	BL_HC = 0,
	BL_XLW = 1,
}BLUETOOTH_TYPE;
typedef struct BluetoothServer {
	BluetoothOps ops;
	pSerialOps serialServer;
	BLUETOOTH_TYPE type;
}BluetoothServer,*pBluetoothServer;

static int setName(pBluetoothOps ops,char* name);
static int setRecvFunc(pBluetoothOps ops ,T_bluetoothRecvFunc recvFunc);
static int getState(pBluetoothOps ops);
static int reboot(struct BluetoothOps * ops);
static int bluetoothsendStr(pBluetoothOps ops,char *str);

static BluetoothOps ops = {
		.setName = setName,
		.setRecvFunc = setRecvFunc,
		.getState = getState,
		.sendStr = bluetoothsendStr,
		.reboot = reboot,
};
static int bluetoothWriteAndWaitAck(pSerialOps serialServer,char *write,char* ackStr);
static int bluetoothServerInit(pBluetoothServer server);
static int checkHCstate(pSerialOps serialServer);

static int checkXLWstate(pSerialOps serialServer);


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
	switch(server->type){
		case BL_HC:
				sprintf(cmdBuf,"AT+NAME=%s",name);
			break;
		case BL_XLW:
				sprintf(cmdBuf,"AT+NAME=%s\r\n",name);
			break;
	}
	server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_SYNC);
	ret = bluetoothWriteAndWaitAck(server->serialServer,cmdBuf,"OK");
	server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_ASYNC);
	if(ret < 0)
	{
		LOGE("fail to bluetooth set name!");
		goto fail0;
	}else {
		LOGI("set Bluetooth name succeed!");
		if(server->type == BL_XLW){
			reboot(ops);
		}
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

	switch(server->type){
		case BL_HC:{
			server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_SYNC);
			ret =  checkHCstate(server->serialServer);
			server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_ASYNC);
		}break;
		case BL_XLW:{
			server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_SYNC);
			ret  =  checkXLWstate(server->serialServer);
			server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_ASYNC);
		}break;
	}
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
static int reboot(struct BluetoothOps * ops)
{
		pBluetoothServer server = (pBluetoothServer)ops;
		char cmdBuf[128]  = { 0};
		int ret;

		if(server == NULL)
			goto fail0;
		switch(server->type){
			case BL_HC:{
				LOGI("restart BL_HC!\n");
				strcpy(cmdBuf,"AT+RESET");
				server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_SYNC);
				ret = bluetoothWriteAndWaitAck(server->serialServer,cmdBuf,"OK");
				server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_ASYNC);
				if(ret < 0)
					return -1;
				}
				break;
			case BL_XLW:
				LOGI("restart BL_XLW!\n");
				strcpy(cmdBuf,"AT+REBOOT\r\n");
				server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_SYNC);
				server->serialServer->write(server->serialServer,cmdBuf,strlen(cmdBuf));
				server->serialServer->changeReadMode(server->serialServer,SERIAL_READ_ASYNC);
				break;
			default:
				return -1;
		}
		return 0;

fail0:
	return -1;

}

static int checkHCstate(pSerialOps serialServer)
{
	int ret;
	ret  = bluetoothWriteAndWaitAck(serialServer,"AT","OK");
	if(ret < 0)
	{
			LOGE("bluetooth:fail to send AT!\n");
			return -1;
	}
	return 0;
}

static int checkXLWstate(pSerialOps serialServer)
{
	int ret;
	int timeoutCnt = 3;
	char readBuf[128] = {0};
	char *cmd = "AT+MAC\r\n";
	while(timeoutCnt-- >0)
	{
		ret = serialServer->write(serialServer,cmd,strlen(cmd));
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
			LOGD("len :%d blueTooth str = %s\n",ret,readBuf);
			return 0;
		}
	}
	fail0:
		return -1;
}
static int bluetoothServerInit(pBluetoothServer server)
{
	int ret;

	switch(server->type){
		case BL_HC:{
			ret  = bluetoothWriteAndWaitAck(server->serialServer,"AT+ROLE=S","OK");
			if(ret < 0)
			{
				LOGE("bluetooth:fail to send AT+ROLE=S!\n");
				goto fail0;
			}
			ret  = bluetoothWriteAndWaitAck(server->serialServer,"AT+CONT=0","OK");
			if(ret < 0)
			{
				LOGE("bluetooth:fail to send AT+CONT=0!\n");
				goto fail0;
			}
		}break;
		case BL_XLW:{

		}break;

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
	server->serialServer = createSerialServer(crateHwInterfaceServer()->getBluetoothUART(), 115200, 8, 1, 'n');
	if(server->serialServer == NULL)
	{
		goto fail1;
	}
	ret = checkXLWstate(server->serialServer);
	if(ret  == 0)
	{
		server->type = BL_XLW;
		LOGD("this is BL_XLW! ");
		goto succeed;
	}else {
		LOGE("fail to BL_XLW!");
		destroySerialServer(&server->serialServer);
	}
	server->serialServer = createSerialServer(crateHwInterfaceServer()->getBluetoothUART(), 9600, 8, 1, 'n');
	if(server->serialServer == NULL)
	{
		goto fail1;
	}
	ret = checkHCstate(server->serialServer);
	if(ret  == 0)
	{
		server->type = BL_HC;
		LOGD("this is BL_HC! ");

	}else {
		LOGE("fail to BL_HC!");
		destroySerialServer(&server->serialServer);
		goto fail2;
	}

succeed:
	ret =  bluetoothServerInit(server);
	if(ret < 0 )
	{
		goto fail2;
	}
	server->ops = ops;
	return (pBluetoothOps)server;
fail2:
	destroySerialServer(&server->serialServer);
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
