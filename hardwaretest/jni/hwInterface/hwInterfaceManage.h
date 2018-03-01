#ifndef _INTERFACE_MANAGE_
#define _INTERFACE_MANAGE_

#ifndef DENF_CPU_VER
#define DENF_CPU_VER
typedef enum {
	A20 = 0,
	A64,
}CPU_VER;
#endif
typedef enum{
	FM1702NL = 0,
	ZLG600A ,
}DOOR_CARD_MODULE;
typedef enum{
	IC_CARD=0,
	CPU_CARD,
	ID_CARD,
}CARD_TYPE;

typedef int (*DoorCardRecvFunc)(CARD_TYPE,unsigned char*,unsigned int);

typedef struct  {
	int (*getDoorLockPin)(void);
	int (*getOpenDoorKeyPin)(void);
	int (*getLightSensorPin)(void);
	int (*getCameraLightPin)(void);
	int (*getKeyLightPin)(void);
	int (*getLcdSwichPin)(void);
	int (*getRestartPin)(void);
	int (*getPirPin)(void);
	char *(*getDoorCardUART)(void);
	DOOR_CARD_MODULE (*getDoorType)(void);

}HwInterfaceOps,*pHwInterfaceOps;


pHwInterfaceOps crateHwInterfaceServer(void);












#endif
