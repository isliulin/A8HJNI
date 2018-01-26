#include <jni.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "binder/binder.h"
#include "binderClient.h"

#define SERVER_NAME "welbell_nativeserver"
#define WELBELL_SVR_CMD_RUNSCRIPT  0X00
#define WELBELL_SVR_CMD_HEARTBEAT  0X01
#define WELBELL_SVR_CMD_GETSOCKETFD  0X02
typedef struct BinderServerPack {
	BinderClientOps ops;
	uint32_t handle;
	struct binder_state *bs;
} BinderServerPack, *pBinderServerPack;
static int getSocketWriteFd(struct BinderClientOps * ops);
static int runScript(pBinderClientOps ops, const char *scriptName);
static int sendHeartbeat(struct BinderClientOps * ops, const char * str);
static BinderClientOps ops = {
		.getSocketWriteFd = getSocketWriteFd,
		.runScript = runScript,
		.sendHeartbeat = sendHeartbeat,
};

static int getSocketWriteFd(struct BinderClientOps * ops) {
	pBinderServerPack pack = (pBinderServerPack) ops;
	if (pack == NULL)
		return -1;
	unsigned iodata[512 / 4];
	struct binder_io msg, reply;
	int ret;
	bio_init(&msg, iodata, sizeof(iodata), 4);

	if (binder_call(pack->bs, &msg, &reply, pack->handle,
			WELBELL_SVR_CMD_GETSOCKETFD))
		return -1;
	ret = bio_get_uint32(&reply);
	binder_done(pack->bs, &msg, &reply);
	LOGE("getSocketWriteFd = %d\n", ret);
	return ret;
	fail0: return -1;
}
static int sendHeartbeat(struct BinderClientOps * ops, const char * str) {
	pBinderServerPack pack = (pBinderServerPack) ops;
	if (pack == NULL)
		return -1;
	unsigned iodata[512 / 4];
	struct binder_io msg, reply;
	int ret;
	bio_init(&msg, iodata, sizeof(iodata), 4);
	bio_put_uint32(&msg, 0); // strict mode header
	bio_put_string16_x(&msg, str);
	if (binder_call(pack->bs, &msg, &reply, pack->handle,
			WELBELL_SVR_CMD_HEARTBEAT))
		return -1;
	ret = bio_get_uint32(&reply);
	binder_done(pack->bs, &msg, &reply);
	LOGE("ret = %d\n", ret);
	return ret;
}
static int runScript(pBinderClientOps ops, const char *scriptName) {
	pBinderServerPack pack = (pBinderServerPack) ops;
	if (pack == NULL)
		return -1;
	unsigned iodata[512 / 4];
	struct binder_io msg, reply;
	int ret;
	bio_init(&msg, iodata, sizeof(iodata), 4);
	bio_put_uint32(&msg, 0); // strict mode header
	bio_put_string16_x(&msg, scriptName);
	LOGE("send:%s",scriptName);
	if (binder_call(pack->bs, &msg, &reply, pack->handle,
			WELBELL_SVR_CMD_RUNSCRIPT)){

		LOGE("fail to binder_call msg:%s",scriptName);
		return -1;
	}
	ret = bio_get_uint32(&reply);
	binder_done(pack->bs, &msg, &reply);
	LOGE("end send ret = %d\n", ret);
	return ret;
}
static uint32_t svcmgr_lookup(struct binder_state *bs, uint32_t target,
		const char *name) {
	uint32_t handle;
	unsigned iodata[512 / 4];
	struct binder_io msg, reply;
	bio_init(&msg, iodata, sizeof(iodata), 4);
	bio_put_uint32(&msg, 0); // strict mode header
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
static pBinderServerPack binderClient = NULL;

pBinderClientOps binder_getServer(void) {
	int fd, reply;

	binderClient = (pBinderServerPack) malloc(sizeof(BinderServerPack));
	if (binderClient == NULL) {
		LOGE( "failed to malloc pServerPack\n");
		return NULL;
	}
	uint32_t svcmgr = BINDER_SERVICE_MANAGER;
	binderClient->bs = binder_open(128 * 1024);
	if (!binderClient->bs) {
		LOGE( "failed to open binder driver\n");
		goto fail1;
	}


	binderClient->handle = svcmgr_lookup(binderClient->bs, svcmgr, SERVER_NAME);
	if (!binderClient->handle) {
		LOGE( "failed to get%s\n", SERVER_NAME);
		goto fail0;
	}
	binderClient->ops = ops;

	return binderClient;
	fail0: free(binderClient->bs);
	fail1: free(binderClient); binderClient = NULL;
	return NULL;
}
void binder_releaseServer(pBinderClientOps *base) {

		pBinderServerPack pack = (pBinderServerPack) (*base);
		if (pack == NULL)
			return;
		binder_release(pack->bs, pack->handle);
		free(pack);
		*base = NULL;


		return;
}

