#include <jni.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "binderClient.h"


#define SERVER_NAME "welbell_nativeserver"
#define WELBELL_SVR_CMD_RUNSCRIPT  0X00
#define WELBELL_SVR_CMD_HEARTBEAT  0X01
typedef struct BinderServerPack{
	BinderClientOps ops;
	uint32_t handle;
	struct binder_state *bs;
}BinderServerPack,*pBinderServerPack;
static int sendHeartbeat(struct BinderClientOps * ops,const char * str)
{
	pBinderServerPack  pack = (pBinderServerPack)ops;
		if(pack == NULL)
			return -1;
		unsigned iodata[512/4];
		struct binder_io msg, reply;
		int ret;
		bio_init(&msg, iodata, sizeof(iodata), 4);
		bio_put_uint32(&msg, 0);  // strict mode header
	    bio_put_string16_x(&msg, str);
		if (binder_call(pack->bs, &msg, &reply, pack->handle, WELBELL_SVR_CMD_HEARTBEAT))
			return -1;
		ret = bio_get_uint32(&reply);
		binder_done(pack->bs, &msg, &reply);
		return ret;
}
static int runScript(pBinderClientOps ops,char *scriptName)
{
	pBinderServerPack  pack = (pBinderServerPack)ops;
	if(pack == NULL)
		return -1;
	unsigned iodata[512/4];
	struct binder_io msg, reply;
	int ret;
	bio_init(&msg, iodata, sizeof(iodata), 4);
	bio_put_uint32(&msg, 0);  // strict mode header
    bio_put_string16_x(&msg, scriptName);
	if (binder_call(pack->bs, &msg, &reply, pack->handle, WELBELL_SVR_CMD_RUNSCRIPT))
		return -1;
	ret = bio_get_uint32(&reply);
	binder_done(pack->bs, &msg, &reply);
	return ret;
}
static uint32_t svcmgr_lookup(struct binder_state *bs, uint32_t target, const char *name)
{
    uint32_t handle;
    unsigned iodata[512/4];
    struct binder_io msg, reply;
    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0);  // strict mode header
    bio_put_string16_x(&msg, SVC_MGR_NAME);
    bio_put_string16_x(&msg, name);

    if (binder_call(bs, &msg, &reply, target, SVC_MGR_CHECK_SERVICE))
        return 0;
    handle = bio_get_ref(&reply);
    if (handle)
        binder_acquire(bs, handle);
    binder_done(bs, &msg, &reply);
    return handle;
}
pBinderClientOps binder_getServer(void)
{
	int fd,reply;
	pBinderServerPack pack = NULL;
	pack =(pBinderServerPack)malloc(sizeof(BinderServerPack));
	if(pack == NULL)
	{
		LOGE( "failed to malloc pServerPack\n");
		return NULL;
	}
	uint32_t svcmgr = BINDER_SERVICE_MANAGER;
	pack->bs = binder_open(128*1024);
	if (!pack->bs) {
	    LOGE( "failed to open binder driver\n");
	    goto fail1;
	}
	pack->handle = svcmgr_lookup(pack->bs, svcmgr, SERVER_NAME);
	if (!pack->handle) {
	    LOGE( "failed to get%s\n",SERVER_NAME);
	    goto fail0;
	}
	pack->ops.runScript = runScript;
	pack->ops.sendHeartbeat = sendHeartbeat;
	return  &pack->ops;
	fail0:
		free(pack->bs);
	fail1:
		free(pack);
	return 	NULL;
}
void binder_releaseServer(pBinderClientOps *base)
{
	pBinderServerPack  pack = (pBinderServerPack)(*base);
	if(pack == NULL)
		return;
	binder_release(pack->bs, pack->handle);
	free(pack);
	*base = NULL;
	return ;
}




