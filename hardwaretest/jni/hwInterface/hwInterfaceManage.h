#ifndef _INTERFACE_MANAGE_
#define _INTERFACE_MANAGE_

#ifndef DENF_CPU_VER
#define DENF_CPU_VER
typedef enum {
	A20 = 0,
	A64,
	RK3368,
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
	int (*getIFCameraLightPin)(void);
	int (*getKeyLightPin)(void);
	int (*getLcdSwichPin)(void);
	int (*getRestartPin)(void);
	int (*getPirPin)(void);
	int (*getDoorMagneticPin)(void);//门磁
	int (*getSecurityPin)(void);//防拆按钮
	char *(*getDoorCardUART)(void);
	char *(*getIdCardUART)(void);
	char *(*getBluetoothUART)(void); //获取蓝牙模块串口号
	DOOR_CARD_MODULE (*getDoorType)(void);

}HwInterfaceOps,*pHwInterfaceOps;


pHwInterfaceOps crateHwInterfaceServer(void);












#endif
