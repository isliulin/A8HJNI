#ifndef __IC_CARD_H__
#define __IC_CARD_H__

#include "hwInterface/hwInterfaceManage.h"
typedef int(*RecvFunc)(CARD_TYPE,unsigned char*,unsigned int);


typedef struct{

}CpuCardOps,*pCpuCardOps;


// Export interface
pCpuCardOps createZLG600AServer(const char *devPath, RecvFunc IDHandler);
void destroyZLG600AServer(pCpuCardOps *pthis);

#endif
