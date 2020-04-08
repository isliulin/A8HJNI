#ifndef NATIVEMETHODS_H_
#define NATIVEMETHODS_H_
/*注意不要进行代码格式化，会搞乱下面的methods格式*/
#include <jni.h>


JNIEXPORT jint JNICALL jni_a8HardwareControlInit(JNIEnv * env, jobject obj);
JNIEXPORT jint JNICALL jni_a8HardwareControlExit(JNIEnv * env, jobject obj);
JNIEXPORT jbyteArray JNICALL jni_a8GetKeyValue(JNIEnv * env, jobject obj,
		jint key,jbyteArray ValueBuf, jint ValueLen);
JNIEXPORT jint JNICALL jni_a8SetKeyValue(JNIEnv *env, jobject obj, jint key,
	    jbyteArray ValueBuf, jint ValueLen);






static JNINativeMethod methods[] =
		{
				{"a8HardwareControlInit", "()I", (void *) jni_a8HardwareControlInit},
				{"a8SetKeyValue", "(I[BI)I", (void*) jni_a8SetKeyValue},
				{"a8GetKeyValue", "(I[BI)[B", (void*) jni_a8GetKeyValue },
				{"a8HardwareControlExit", "()I", (void *) jni_a8HardwareControlExit}
		};

#endif /* NATIVEMETHODS_H_ */
