#ifndef  __WB_VIRTUAL_HARDWARESUPPORT_H
#define  __WB_VIRTUAL_HARDWARESUPPORT_H





typedef  enum {
	PIR_NEAR  = 0,
	PIR_LEAVE = 1,
}PIR_STATE;
typedef struct {
	unsigned int pin;
	unsigned int state;
	void *interruptArg;
}GpioPinState,*pGpioPinState;
typedef union {
 		char buf[sizeof(unsigned int)];
 		unsigned int id;
}intChar_union;

typedef int (*IcRecvFunc)(unsigned char *,int);
typedef void(*WBPirCallBackFunc)(PIR_STATE );
typedef int (*T_InterruptFunc)(pGpioPinState );

typedef struct VirtualHWops{

		//设置人体感应回调函数
		int (*setPirUpFunc)(struct  VirtualHWops *,WBPirCallBackFunc);
		//设置开门按键触发回调函数
		int (*setOpenDoorKeyUpFunc)(struct  VirtualHWops *,T_InterruptFunc);
		//设置光感回调
		int (*setOptoSensorUpFunc)(struct  VirtualHWops *,T_InterruptFunc);
		//设置IC卡数据回调函数
		int (*setIcCardRawUpFunc)(struct  VirtualHWops *,IcRecvFunc);
}VirtualHWops,*pVirtualHWops;


pVirtualHWops crateVirtualHWServer(void);
void destroyVirtualHWServer(pVirtualHWops *server);



#endif
