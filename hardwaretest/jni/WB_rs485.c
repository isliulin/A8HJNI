#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include "hwInterface/hwInterfaceManage.h"
#include "common/debugLog.h"
#include "serial/serialServer.h"
#include "common/Utils.h"
#include "hwInterface/gpioServer.h"
#include "WB_rs485.h"

typedef struct Rs485Server{
	Rs485Ops ops;
	pSerialOps serialServer;
	pGpioOps rs485controlServer;

}Rs485Server,*pRs485Server;
static int sendMsg(struct Rs485Ops * ops,char *data,int len);
static int recvMsg(struct Rs485Ops * ops,int timeOut,char * data,int len);

static const  Rs485Ops ops = {
		.sendMsg = sendMsg,
		.recvMsg = recvMsg,

};
static int sendMsg(struct Rs485Ops * ops,char *data,int len){
	pRs485Server pthis = (pRs485Server)ops;
	if(pthis == NULL)
		return -1;
	int sendRet;
	pthis->rs485controlServer->setOutputValue(pthis->rs485controlServer,1);
	sendRet = pthis->serialServer->write(pthis->serialServer,data,len);
	usleep(1000*len+200*1000);

	pthis->rs485controlServer->setOutputValue(pthis->rs485controlServer,0);
	return sendRet;
}
static int recvMsg(struct Rs485Ops * ops,int timeOut,char * data,int len){
	pRs485Server pthis = (pRs485Server)ops;
		if(pthis == NULL)
			return -1;
	int recvRet;
	pthis->rs485controlServer->setOutputValue(pthis->rs485controlServer,0);
	recvRet = pthis->serialServer->read(pthis->serialServer,data,len,timeOut);
	return recvRet;
}
pRs485Ops createRs485Server(int nBaudRate, int nDataBits, int nStopBits, int nParity){
	char *uartpath = NULL;
	int   controlPin;

	pRs485Server server = (pRs485Server)malloc(sizeof(Rs485Server));
	if(server == NULL){
		goto fail0;
	}
	uartpath = crateHwInterfaceServer()->getRs485UART();
	controlPin = crateHwInterfaceServer()->getRs485controlPin();
	if(uartpath == NULL || controlPin  < 0){
		goto fail1;
	}

	server->serialServer =  createSerialServer( uartpath,nBaudRate,nDataBits,nStopBits,nParity);
	if(server->serialServer == NULL){
		goto fail1;
	}
	server->rs485controlServer  = gpio_getServer(controlPin);
	if(server->rs485controlServer == NULL ){
		goto fail2;
	}
	server->ops = ops;
	return server;
	fail2:
		destroySerialServer(&server->serialServer);
	fail1:
		free(server);
	fail0:
		return NULL;
}
void destroyRs485Server(pRs485Ops *server){
	if(server == NULL || *server){
		return;
	}
	pRs485Server pthis = (pRs485Server)*server;
	if(pthis->serialServer){

		destroySerialServer(&pthis->serialServer);
	}
	free(pthis);
	*server = NULL;
}
