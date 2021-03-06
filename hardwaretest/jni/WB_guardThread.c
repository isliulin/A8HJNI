#include <linux/stddef.h>


#include "WB_guardThread.h"
#include "taskManage/timerTaskManage.h"
#include "common/nativeNetServer.h"
#include "common/debugLog.h"



#define SERVER_PORT 	10800
#define CLIENT_HB_PORT 	10900
#define SERVER_IP_ADDR  (inet_addr("127.0.0.1"))


typedef struct TimerArg{
	char heartbeatString[128];

#if USER_BINDER == 1
	pBinderClientOps binderClient;
#else
	pNativeNetServerOps netClient;
#endif
}TimerArg,*pTimerArg;
typedef struct PackageInfo{
	char packageName[32];
	char mainClassName[32];
	int  heartbeatTime;
	pTimerOps timeTaskId;
}PackageInfo,*pPackageInfo;

typedef struct GuardThreadServer{
	GuardThreadOps ops;
	PackageInfo packAgeList[6];

#if USER_BINDER == 1
	pBinderClientOps	binderClient;
#else
	pNativeNetServerOps netClient;
#endif
}GuardThreadServer,*pGuardThreadServer;
static int setGuardPackagenameAndMainclassname(struct GuardThreadOps* ops,
		const char *packageName,const char * mainClassName,int heartbeatTime );
static int sendHearBeatToServer(struct GuardThreadOps* ops,
		const char *packageName,const char * mainClassName,int heartbeatTime );

static GuardThreadOps ops = {
		.setGuardPackagenameAndMainclassname = setGuardPackagenameAndMainclassname,
		.sendHearBeatToServer = sendHearBeatToServer,
};

static void  sendHeartbeatToserver(void *arg)
{
	pTimerArg timerArg  = (pTimerArg)arg;

#if USER_BINDER == 1
	if(timerArg == NULL||timerArg->binderClient == NULL)
		return ;
	timerArg->binderClient->sendHeartbeat(timerArg->binderClient,timerArg->heartbeatString);
#else
	if(timerArg == NULL||timerArg->netClient == NULL)
		return ;
	timerArg->netClient->sendHeartbeat(timerArg->netClient,timerArg->heartbeatString);

#endif
	return ;

}
static int findEmptyMember(pPackageInfo array,int arrayNum)
{
	int i;
	for(i = 0; i < arrayNum;i++)
	{
		if(array[i].timeTaskId == NULL&&array[i].heartbeatTime == 0)
		{
			return i;
		}
	}
	return -1;
}
static int stopHeartbeat(struct GuardThreadOps* ops,
		const char *packageName,const char * mainClassName)
{
	char heartbeatString[128] = {0};
	pGuardThreadServer pthis  = (pGuardThreadServer)ops;
	if(pthis == NULL)
	{
		goto fail0;
	}
	sprintf(heartbeatString,"state:1;pack:%s;class:%s;",packageName,mainClassName);

#if USER_BINDER == 1
	if(pthis->binderClient == NULL)
	{
		goto fail0;
	}
	return pthis->binderClient->sendHeartbeat(pthis->binderClient,heartbeatString);
#else
	if(pthis->netClient == NULL)
	{
		goto fail0;
	}
	return pthis->netClient->sendHeartbeat(pthis->netClient,heartbeatString);

#endif
fail0:
	return -1;
}


static int sendHearBeatToServer(struct GuardThreadOps* ops,
		const char *packageName,const char * mainClassName,int heartbeatTime ){
	char heartbeatString[256] = {0};
	pGuardThreadServer pthis  = (pGuardThreadServer)ops;
	if(pthis == NULL)
	{
		goto fail0;
	}
	sprintf(heartbeatString,"state:0;pack:%s;class:%s;time:%d;",packageName,mainClassName,heartbeatTime);
#if USER_BINDER == 1
	if(pthis->binderClient != NULL ){
		pthis->binderClient(pthis->binderClient,heartbeatString);
	}
	else{
		goto fail0;
	}
#else
	if(pthis->netClient != NULL){
		pthis->netClient->sendHeartbeat(pthis->netClient,heartbeatString);
	}else{
		goto fail0;
	}
#endif
	return 0;
	fail0:
		return -1;
}
static int setGuardPackagenameAndMainclassname(struct GuardThreadOps* ops,
		const char *packageName,const char * mainClassName,int heartbeatTime )
{
	int index;
	TimerArg timerArg;
	bzero(&timerArg,sizeof(TimerArg));
	char heartbeatString[256] = {0};
	pGuardThreadServer pthis  = (pGuardThreadServer)ops;
	if(pthis == NULL)
	{
		goto fail0;
	}
	index = findEmptyMember(pthis->packAgeList,
			sizeof(pthis->packAgeList)/sizeof(pthis->packAgeList[0]));
	if(index < 0)
	{
		LOGE("fail to findEmptyMember");
		goto fail0;
	}
	strcpy(pthis->packAgeList[index].packageName,
			packageName);
	strcpy(pthis->packAgeList[index].mainClassName,
			mainClassName);
	pthis->packAgeList[index].heartbeatTime = heartbeatTime*1000;

	sprintf(heartbeatString,"state:0;pack:%s;class:%s;time:%d;",packageName,mainClassName,heartbeatTime);
	LOGD("heartbeatString:%s",heartbeatString);
#if USER_BINDER == 1
	timerArg.binderClient = pthis->binderClient;
#else
	timerArg.netClient = pthis->netClient;
#endif

	strcpy(timerArg.heartbeatString,heartbeatString);
	pthis->packAgeList[index].timeTaskId = createTimerTaskServer(3, heartbeatTime*1000,-1,1,sendHeartbeatToserver,
						&timerArg,sizeof(TimerArg));
	if(pthis->packAgeList[index].timeTaskId == NULL)
	{
		LOGE("fail to createTimerTaskServer");
		goto fail0;
	}
	pthis->packAgeList[index].timeTaskId->start(pthis->packAgeList[index].timeTaskId);

fail0:
	return -1;
}

pGuardThreadOps  createGuardThreadServer(void)
{
	pGuardThreadServer server = malloc(sizeof(GuardThreadServer));
	if(server == NULL)
	{
		LOGE("fial to malloc pGuardThreadServer !");
		goto fail0;
	}
	bzero(server,sizeof(GuardThreadServer));

#if USER_BINDER == 1
	server->binderClient = binder_getServer();
	if(server->binderClient == NULL)
	{
		LOGE("fial to binder_getServer !!!!");
		goto fail1;
	}
#else
	server->netClient = createNativeNetServer();
	if(server->netClient == NULL)
	{
		LOGE("fial to createNativeNetServer !!!!");
		goto fail1;
	}
#endif
	server->ops = ops;
	return (pGuardThreadOps)server;

fail1:
	free(server);
fail0:
	return NULL;
}
void destroyGuardThreadServer(pGuardThreadOps * server)
{
	int i;
	pGuardThreadServer pthis  = (pGuardThreadServer)*server;
	if(pthis == NULL)
	{
		goto fail0;
	}
	for(i = 0;i<sizeof(pthis->packAgeList)/sizeof(pthis->packAgeList[0]);i++ ){
		if(pthis->packAgeList[i].timeTaskId != NULL)
		{
			stopHeartbeat(pthis,pthis->packAgeList[i].packageName,
							pthis->packAgeList[i].mainClassName);
			destroyTimerTaskServer(&pthis->packAgeList[i].timeTaskId);
			bzero(&pthis->packAgeList[i],sizeof(PackageInfo));
		}
	}
#if USER_BINDER == 1
	if(pthis->binderClient)
	{
		binder_releaseServer(&pthis->binderClient);
	}
#else
	if(pthis->netClient)
	{
		destroyNativeNetServer(&pthis->netClient);
	}
#endif
	*server = NULL;
	fail0:
		return;
}
