#ifndef _INTERFACE_MANAGE_
#define _INTERFACE_MANAGE_

#ifndef DENF_CPU_VER
#define DENF_CPU_VER
typedef enum {
	A20 = 0,
	A64,
}CPU_VER;
#endif

typedef struct  {
	int (*getDoorLockPin)(void);
	int (*getOpenDoorKeyPin)(void);
	int (*getLightSensorPin)(void);
	int (*getCameraLightPin)(void);
	int (*getKeyLightPin)(void);
	int (*getLcdSwichPin)(void);
	int (*getRestartPin)(void);
	int (*getPirPin)(void);
	char *(*getIcCardUART)(void);
}HwInterfaceOps,*pHwInterfaceOps;


pHwInterfaceOps crateHwInterfaceServer(CPU_VER ver);












#endif
