#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "WB_pirSupport.h"
#include "taskManage/timerTaskManage.h"
#include "gpio/gpioServer.h"
#include "common/debugLog.h"

static int gpioInterruptFunc (pGpioPinState arg);
static int timerCallback( void *arg);
typedef struct {
	WBPir_ops ops;
	pGpioOps gpioServer;
	pTimerOps timerServer;
	PIR_STATE pirState;
	pthread_mutex_t  mutex;
	WBPirCallBackFunc upStateFunc;

}WBPirServer,*pWBPirServer;


pWBPir_ops crateWBPirServer(int gpioPin,WBPirCallBackFunc upFunc)
{
	pWBPirServer wbPirServer = NULL;
	if(upFunc == NULL || gpioPin<0)
		return NULL;
	wbPirServer = malloc(sizeof(WBPirServer));
	if(wbPirServer == NULL)
	{
		goto fail0;
	}
	bzero(wbPirServer,sizeof(WBPirServer));
	wbPirServer->gpioServer = gpio_getServer(gpioPin);
	if(wbPirServer->gpioServer == NULL )
	{
		goto fail1;
	}
	//wbPirServer->timerServer = createTimerTaskServer(2*1000,-1,1,timerCallback,(void *)&wbPirServer,sizeof(void*));
	//if(wbPirServer->timerServer == NULL )
	//	goto fail2;

	wbPirServer->gpioServer->setInterruptFunc(wbPirServer->gpioServer ,gpioInterruptFunc,wbPirServer,FALLING);
	wbPirServer->upStateFunc = upFunc;
	wbPirServer->pirState = 0;


	pthread_mutex_init(&wbPirServer->mutex,NULL);
	return (pWBPir_ops)wbPirServer;
	fail2:
		free(wbPirServer->gpioServer);
	fail1:
		free(wbPirServer);
	fail0:
		return NULL;
}
static int timerCallback( void *arg)
{

	pWBPirServer wbPirServer = *((pWBPirServer*) arg);
	if(wbPirServer == NULL)
		return -1;
	pthread_mutex_lock(&wbPirServer->mutex);
	wbPirServer->pirState = PIR_LEAVE;
	wbPirServer->upStateFunc(wbPirServer->pirState);
	pthread_mutex_unlock(&wbPirServer->mutex);
	return 0;
}
static int gpioInterruptFunc (pGpioPinState arg)
{
	pGpioPinState gpioState = arg;
	pWBPirServer wbPirServer  = gpioState->interruptArg;
	LOGE("gpioInterruptFunc");
	pthread_mutex_lock(&wbPirServer->mutex);
	//if(wbPirServer->pirState == PIR_LEAVE)
	{
		wbPirServer->pirState = PIR_NEAR;
		wbPirServer->upStateFunc(wbPirServer->pirState);
	}
	//重设定时器
	pthread_mutex_unlock(&wbPirServer->mutex);

//   wbPirServer->timerServer->reset(wbPirServer->timerServer);
	return 0;
}


void destroyWBPirServer(pWBPir_ops *base)
{
	pWBPirServer wbPirServer = *((pWBPirServer*) base);
	if(wbPirServer == NULL)
		return ;
	gpio_releaseServer(&wbPirServer->gpioServer);
	destroyTimerTaskServer(&wbPirServer->timerServer);
	free(wbPirServer);
	*base = NULL;

}
