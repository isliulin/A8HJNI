#include <stdio.h>
#include "common/debugLog.h"
#include "NativeMethods.h"


static const char*  classPathName = "com/welbell/hardware/HardwareSupport";
static const char * libVersion = "Ver1.0.3.7 For A8HNativeControl";
/*
 * Register several native methods for one class.
 */
static int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* gMethods, int numMethods) {
	jclass clazz;
	LOGI( "RegisterNatives start for '%s'", className);
	//获取类
	clazz = (*env)->FindClass( env,className);
	if (clazz == NULL) {
		LOGE( "Native registration unable to find class '%s'", className);
		return JNI_FALSE;
	}
	//将本地方法注册到类中
	if ((*env)->RegisterNatives(env,clazz, gMethods, numMethods) < 0) {
		LOGE("RegisterNatives failed for '%s'", className);
		return JNI_FALSE;
	}
	return JNI_TRUE;
}
/*
 * Register native methods for all classes we know about.
 *
 * returns JNI_TRUE on success.
 */
static int registerNatives(JNIEnv* env) {
	//注册本地方法
	if (!registerNativeMethods(env,classPathName , methods, sizeof(methods) / sizeof(methods[0]))) {
		return JNI_FALSE;
	}
	return JNI_TRUE;
}
/*
 * This is called by the VM when the shared library is first loaded.
 */
typedef union {
	JNIEnv* env;
	void* venv;
} UnionJNIEnvToVoid;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	UnionJNIEnvToVoid uenv;
	uenv.venv = NULL;
	jint result = -1;
	JNIEnv* env = NULL;
	LOGI("------Load A8System  Lib , %s------", libVersion);
	LOGI("------Build at %s %s -------", __DATE__, __TIME__);
	//获取环境变量内容
	if ((*vm)->GetEnv(vm,&uenv.venv, JNI_VERSION_1_4) != JNI_OK) {
		LOGE("GetEnv failed");
		goto bail;
	}
	env = uenv.env;
	//注册
	if (registerNatives(env) != JNI_TRUE) {
		LOGE("registerNatives failed");
		goto bail;
	}

	result = JNI_VERSION_1_4;

	bail: return result;
}

