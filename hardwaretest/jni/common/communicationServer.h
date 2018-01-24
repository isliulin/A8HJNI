#ifndef  __COMMUNICATION_SERVER_H_
#define  __COMMUNICATION_SERVER_H_


typedef int (*CommunicationCallBackFunc)(void*,int);

typedef struct CommunicationOps{
	int (*setRecvCallbackFunc)(struct CommunicationOps *,CommunicationCallBackFunc);
	int (*send)(struct CommunicationOps *,void *,unsigned int);
	int (*recv)(struct CommunicationOps *,void *,unsigned int);
}CommunicationOps,*pCommunicationOps;

pCommunicationOps createCommunicationServer(int writeFd,int readFd);
void destroyCommunicationServer(pCommunicationOps *server);



#endif



