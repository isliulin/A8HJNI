#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "WB_pirSupport.h"
#include "taskManage/timerTaskManage.h"
#include "hwInterface/gpioServer.h"
#include "common/debugLog.h"

static int gpioInterruptFunc (pGpioPinState arg);
static int timerCallback( void *arg);
typedef struct {
	WBPir_ops ops;
	pGpioOps gpioServer;
	PIR_STATE pirState;
	pthread_mutex_t  mutex;
	WBPirCallBackFunc upStateFunc;
	struct timespec  current_time;
	struct timespec	last_time;
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

	clock_gettime(CLOCK_MONOTONIC, &wbPirServer->last_time);
	pthread_mutex_lock(&wbPirServer->mutex);
	//每秒钟只需上传一次
	if((wbPirServer->last_time.tv_sec-wbPirServer->current_time.tv_sec>= 1)
			&&arg->state == 0)
	{
		wbPirServer->pirState = PIR_NEAR;
		clock_gettime(CLOCK_MONOTONIC, &wbPirServer->current_time);
		wbPirServer->upStateFunc(wbPirServer->pirState);
	}
	pthread_mutex_unlock(&wbPirServer->mutex);

	return 0;
}


void destroyWBPirServer(pWBPir_ops *base)
{
	pWBPirServer wbPirServer = *((pWBPirServer*) base);
	if(wbPirServer == NULL)
		return ;
	gpio_releaseServer(&wbPirServer->gpioServer);
	free(wbPirServer);
	*base = NULL;

}
