#ifndef WB_BLUETOOTH_H__
#define WB_BLUETOOTH_H__
#include "hwInterface/hwInterfaceManage.h"
typedef int (*T_bluetoothRecvFunc)(char *,unsigned int);
typedef struct BluetoothOps {
	int (*setName)(struct BluetoothOps *,char*);
	int (*setRecvFunc)(struct BluetoothOps *,T_bluetoothRecvFunc);
	int (*getState)(struct BluetoothOps *);
	int (*sendStr)(struct BluetoothOps *,char *);
	int (*reboot)(struct BluetoothOps *);
}BluetoothOps,*pBluetoothOps;

pBluetoothOps createBluetoothServer(void);
void destroyBluetoothServer(pBluetoothOps *server);



#endif
