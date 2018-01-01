#ifndef  __WB_ICDOORCARD_H
#define  __WB_ICDOORCARD_H


typedef union {
 		char buf[sizeof(uint32_t)];
 		uint32_t id;
}intChar_union;
typedef int (*IcRecvFunc)(unsigned char *,int);

typedef struct{


}IcDoorCardOps,*pIcDoorCardOps;

pIcDoorCardOps crateIcDoorCardOpsServer(const unsigned char *devPath,IcRecvFunc recvFunc);
void  destroyIcDoorCardOpsServer(pIcDoorCardOps* server);





#endif
