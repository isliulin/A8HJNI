#include <linux/stddef.h>
#include "WB_guardThread.h"
#include "taskManage/timerTaskManage.h"
#include "common/nativeNetServer.h"
#include "common/communicationServer.h"
#include "common/netUdpServer.h"
#include "common/debugLog.h"
#include "binder/binderClient.h"

#define SERVER_PORT 	10800
#define CLIENT_HB_PORT 	10900
#define SERVER_IP_ADDR   (inet_addr("127.0.0.1"))


typedef struct TimerArg{
	char heartbeatString[128];
	//pNativeNetServerOps netServer;
	pBinderClientOps binderClient;
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
	//pNativeNetServerOps netServer;
	pBinderClientOps	binderClient;
}GuardThreadServer,*pGuardThreadServer;
static int setGuardPackagenameAndMainclassname(struct GuardThreadOps* ops,
		const char *packageName,const char * mainClassName,int heartbeatTime );

static GuardThreadOps ops = {
		.setGuardPackagenameAndMainclassname = setGuardPackagenameAndMainclassname,


};

static void  sendHeartbeatToserver(void *arg)
{
	pTimerArg timerArg  = (pTimerArg)arg;
	if(timerArg == NULL||timerArg->binderClient == NULL)
		return ;


	timerArg->binderClient->sendHeartbeat(timerArg->binderClient,timerArg->heartbeatString);

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
	if(pthis->binderClient == NULL)
	{
		goto fail0;
	}
	return pthis->binderClient->sendHeartbeat(pthis->binderClient,heartbeatString);

fail0:
	return -1;
}
static int setGuardPackagenameAndMainclassname(struct GuardThreadOps* ops,
		const char *packageName,const char * mainClassName,int heartbeatTime )
{
	int index;
	TimerArg timerArg;
	bzero(&timerArg,sizeof(TimerArg));
	char heartbeatString[128] = {0};
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

	timerArg.binderClient = pthis->binderClient;
	strcpy(timerArg.heartbeatString,heartbeatString);
	pthis->packAgeList[index].timeTaskId = createTimerTaskServer(5, heartbeatTime*1000,-1,1,sendHeartbeatToserver,
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


	server->binderClient = binder_getServer();
	if(server->binderClient == NULL)
	{
		LOGE("fial to binder_getServer !!!!");
		goto fail1;
	}
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
	if(pthis->binderClient)
	{
		binder_releaseServer(&pthis->binderClient);
	}

	*server = NULL;
	fail0:
		return;
}
