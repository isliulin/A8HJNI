/*
 * CallbackJavaMethod.cpp
 *
 *  Created on: 2013-11-14
 *      Author: jerry
 */
#include <jni.h>
#include <stdio.h>
#include <jni.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <strings.h>
#include "common/CallbackJavaMethod.h"
#include "common/debugLog.h"

typedef struct CallbackJavaMethod {
	JavaMethodOps ops;
	JNIEnv* jniEnv;
	JavaVM* javaVM;
	pthread_t jvmThread;
	pthread_t upMsgThread;
	jobject callBackObject;
	jmethodID SystemCallBack_id;
	int epoolFd;
	int wakeFds[2]; //[0]:read [1]:write
} CallbackJavaMethod, *pCallbackJavaMethod;

typedef struct PipeMsg{
	unsigned int dataLen;
	char  startData;
}PipeMsg,*pPipeMsg;
static void * SystemCallBack(pJavaMethodOps pthis,void *data,int len);
static int pushMsgToThread(pJavaMethodOps ops,void *data ,int len);
static void * threadUpFunc(void *arg);

static JavaMethodOps ops = {
		.up = pushMsgToThread,
};
static pthread_mutex_t  javaMethodMutex = PTHREAD_MUTEX_INITIALIZER;

pJavaMethodOps CallbackJavaMethodInit( JNIEnv *env,
		jobject obj,const char *callbackName) {
	int result;
	pCallbackJavaMethod javaMethod =(pCallbackJavaMethod)malloc(sizeof(CallbackJavaMethod));
	if(javaMethod == NULL )
	{
		goto fail0;
	}
	//获取虚拟机
	(*env)->GetJavaVM(env, &javaMethod->javaVM);
	javaMethod->jniEnv = env;
	//获取当前线程
	javaMethod->jvmThread = pthread_self();
	//获取当前类
	jclass cls = (*env)->GetObjectClass(env, obj);
	if (cls != NULL) {
		javaMethod->callBackObject =
				(jobject) (*javaMethod->jniEnv)->NewGlobalRef(env, obj);
		//获取回调函数的ID
		javaMethod->SystemCallBack_id = (*env)->GetMethodID(env, cls,
				callbackName,"([B)V"	);
	} else {
		LOGE("Get class form jobject failed.");
	}

	//创建epool节点
	struct epoll_event ev, events[1];
	javaMethod->epoolFd = epoll_create(1);
	if(javaMethod->epoolFd <0 )
	{
		goto fail1;
	}
	//创建管道
	result = pipe(javaMethod->wakeFds);
	if(result != 0)
	{
		goto fail1;
	}
	//设置管道成非阻塞模式
	result = fcntl(javaMethod->wakeFds[0], F_SETFL, O_NONBLOCK);
	if(result != 0)
	{
		goto fail2;
	}
	result = fcntl(javaMethod->wakeFds[1], F_SETFL, O_NONBLOCK);
	if(result != 0)
	{
		goto fail3;
	}
	bzero(&ev,sizeof(ev));

	ev.data.fd = javaMethod->wakeFds[0];
	ev.events =  EPOLLIN;
	//将读管道加入到epool中
	epoll_ctl(javaMethod->epoolFd, EPOLL_CTL_ADD,javaMethod->wakeFds[0], &ev);

	//创建线程监听管道

	 if (pthread_create(&javaMethod->upMsgThread, 0, threadUpFunc,
				 (void*)javaMethod) != 0) {
		 		goto fail4;
	 }



	javaMethod->ops = ops;
	return (pJavaMethodOps)javaMethod;
fail4:
	close(javaMethod->wakeFds[1]);
fail3:
	close(javaMethod->wakeFds[0]);
fail2:
	close(javaMethod->epoolFd);
fail1:
	free(javaMethod);
fail0:
	return NULL;
}
void CallbackJavaMethodExit(pJavaMethodOps *ops) {
	JNIEnv* env = NULL;
	pCallbackJavaMethod JavaMethodServer = (pCallbackJavaMethod)*ops;

	LOGE("Try ~CallbackJavaMethod");
	JNIEnv* jniEnv = JavaMethodServer->jniEnv;

	pushMsgToThread(*ops,NULL,0);//结束线程
	pthread_join(JavaMethodServer->upMsgThread,NULL);

	close(JavaMethodServer->epoolFd);
	close(JavaMethodServer->wakeFds[0]);
	close(JavaMethodServer->wakeFds[1]);
	if (JavaMethodServer->callBackObject != NULL) {
		if (jniEnv != NULL) {
			(*jniEnv)->DeleteGlobalRef(jniEnv, JavaMethodServer->callBackObject);
		}
	}
	free(JavaMethodServer);
	*ops = NULL;
}
//创建线程接收队列消息,并上报消息
static void * threadUpFunc(void *arg) {

	pCallbackJavaMethod javaMethod = (pCallbackJavaMethod)arg;
	struct epoll_event events[1];
	int nRead;
	PipeMsg recvMsgHead;
	char msgBuf[1024*4] = {0};
	int msgHead;
	int pollResult ;
    if(javaMethod == NULL)
    	goto exit;
	for(;;)
	{
		pollResult = epoll_wait(javaMethod->epoolFd, events, 1, -1);
		if(pollResult < 0){
			continue;
		}
	againRead:
		nRead = read(javaMethod->wakeFds[0], &msgHead, sizeof(msgHead));
		if(nRead != sizeof(msgHead)){
			continue;
		}
		if(msgHead == 0)
			goto exit;
		bzero((void*)msgBuf,sizeof(msgBuf));
		nRead = read(javaMethod->wakeFds[0], msgBuf, msgHead);
		if(nRead != msgHead)
			continue;
		SystemCallBack((pJavaMethodOps)javaMethod,msgBuf,nRead);
		goto againRead;//防止管道里面还有数据
	}
exit:
	return NULL;
}
//编写方法发送消息
static int pushMsgToThread(pJavaMethodOps ops,void *data ,int len)
{
	char msgBuf[4*1024] = {0};
	int nWrite = 0;
	if(len+sizeof(len) >= sizeof(msgBuf)||ops == NULL)
		return -1;
	pCallbackJavaMethod javaMethod = (pCallbackJavaMethod)ops;
	memcpy(msgBuf,&len,sizeof(len));
	if(len != 0 && data != NULL)
		memcpy(msgBuf+sizeof(len),data,len);
    do {
        nWrite = write(javaMethod->wakeFds[1], msgBuf, len+sizeof(len));
    } while ((nWrite == -1 && errno == EINTR));
	return len;
}
static void * SystemCallBack(pJavaMethodOps pthis,void *data,int len) {
	int battach = 0;
	pCallbackJavaMethod JavaMethodServer = (pCallbackJavaMethod)pthis;
	if(pthis == NULL)
		goto exit;
	pthread_mutex_lock(&javaMethodMutex);
	const  jbyte * Jbyte= data;
	//判断是不是本线程，不能在本线程中调用回调
	if (JavaMethodServer->jvmThread != pthread_self()) {
		//重新添加本线程的环境变量到javaVM中
		if ((*(JavaMethodServer->javaVM))->AttachCurrentThread(
				JavaMethodServer->javaVM, &JavaMethodServer->jniEnv, NULL) < 0) {
			goto exit;
		}
		battach = 1;
	}
	if (JavaMethodServer->jniEnv != NULL) {
		//创建数组
		jbyteArray jarray = (*JavaMethodServer->jniEnv)->NewByteArray(
				JavaMethodServer->jniEnv,len);
		(*(JavaMethodServer->jniEnv))->SetByteArrayRegion(JavaMethodServer->jniEnv,
				jarray, 0, len, Jbyte);
		if (JavaMethodServer->SystemCallBack_id != NULL) {
			//执行回调
			(*(JavaMethodServer->jniEnv))->CallVoidMethod(JavaMethodServer->jniEnv,
					JavaMethodServer->callBackObject,
					JavaMethodServer->SystemCallBack_id,jarray);
		} else {
			LOGE("callBackObject is null or SystemCallBack_id is null");
		}
		(*(JavaMethodServer->jniEnv))->DeleteLocalRef(JavaMethodServer->jniEnv,
				jarray);
		if (battach == 1) {
			(*(JavaMethodServer->javaVM))->DetachCurrentThread(
					JavaMethodServer->javaVM);
		}
	}
	pthread_mutex_unlock(&javaMethodMutex);
exit:
	return NULL;
}

