#ifndef NATIVE_NET_SERVER__H__
#define NATIVE_NET_SERVER__H__



typedef struct NativeNetServerOps{

	int (*runScript)(struct NativeNetServerOps *,const char *);
	int (*sendHeartbeat)(struct NativeNetServerOps *,const char *);



}NativeNetServerOps,*pNativeNetServerOps;

pNativeNetServerOps createNativeNetServer(void);
void destroyNativeNetServer(pNativeNetServerOps *ops);



#endif
