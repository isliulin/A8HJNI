#ifndef _NATIVE_SERVER_
#define _NATIVE_SERVER_
#include <jni.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "binder.h"
#include "common/debugLog.h"



typedef struct BinderClientOps{
	int (*runScript)(struct BinderClientOps *,const char *);
	int (*getSocketWriteFd)(struct BinderClientOps *);

}BinderClientOps,*pBinderClientOps;

pBinderClientOps
binder_getServer(void);
void
binder_releaseServer(pBinderClientOps *);
#endif
