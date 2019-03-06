#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "common/debugLog.h"
#include "common/Utils.h"
#include "hwInterface/hwInterfaceConfig.h"

#define HW_CONFIG_PATH  "/etc/hwConfig.cfg"
//外接电磁锁的GPIO
#define OPENDOOR_PIN 			"opendoor_pin"
//接内部开门按键的GPIO
#define INTERIORKEY_PIN 		"interiorkey_pin"
//接门磁的GPIO
#define DOORMAGNETIC_PIN 		"doormagnetic_pin"
//接可见光摄像头补光灯的GPIO
#define VL_CAMERALED_PIN 		"vl_cameraled_pin"
//接红外摄像头补光灯的GPIO
#define IF_CAMERALED_PIN 		"if_cameraled_pin"
//接人体感应的GPIO
#define PIR_PIN  				"pir_pin"
//接光敏的GPIO
#define LIGHTSENSOR_PIN 		"lightsensor_pin"
//接防拆按钮的gpio
#define PREVENT_SEPARATE_PIN 	"prevent_separate_pin"
//接屏幕背光的GPIO
#define LCDBACKLIGHT_PIN 		"lcdbacklight_pin"
//接红绿蓝LED等GPIO
#define R_LED_PIN 				"r_led_pin"
#define G_LED_PIN 				"g_led_pin"
#define	B_LED_PIN				"b_len_pin"
//接键盘背光的GPIO
#define KEYBOARD_LIGHT_PIN      "keyboard_light_pin"

//接RS485控制脚的GPIO
#define RS485CONTROL_PIN		"rs485control_pin"
//接刷卡模块的串口
#define IC_CARD_UART 			"ic_card_uart"
//接身份证模块的串口
#define ID_CARD_UART 			"id_card_uart"
//接蓝牙模块的串口
#define BT_UART 				"bt_uart"

//接rs485_uart
#define RS485_UART 				"rs485_uart"


static int readKey(char *key, char *value);
static CPU_VER cpuVer;

#define RK_P0(num) (0+num)
#define RK_P1(num) (32+num)
#define RK_P2(num) (64+num)
static int PA(int num) {
	if (cpuVer == A20 || cpuVer == A64)
		return num;
	else if (cpuVer == RK3368 || cpuVer == RK3128)
		return num;
	return -1;
}
static int PB(int num) {
	if (cpuVer == A20)
		return num + 24;
	else if (cpuVer == A64)
		return num + 32;
	else if (cpuVer == RK3368 || cpuVer == RK3128)
		return num + 8;
	return -1;
}
static int PC(int num) {
	if (cpuVer == A20)
		return num + 54;
	else if (cpuVer == A64)
		return num + 64;
	else if (cpuVer == RK3368 || cpuVer == RK3128)
		return num + 16;
	return -1;
}
static int PD(int num) {
	if (cpuVer == A20)
		return num + 85;
	else if (cpuVer == A64)
		return num + 96;
	else if (cpuVer == RK3368 || cpuVer == RK3128)
		return num + 24;
	return -1;
}
static int PE(int num) {
	if (cpuVer == A20)
		return num + 119;
	else if (cpuVer == A64)
		return num + 128;
	return -1;
}
static int PF(int num) {
	if (cpuVer == A20)
		return num + 137;
	else if (cpuVer == A64)
		return num + 160;
	return -1;
}
static int PG(int num) {
	if (cpuVer == A20)
		return num + 149;
	else if (cpuVer == A64)
		return num + 192;
	return -1;
}

static int PH(int num) {
	if (cpuVer == A20)
		return num + 167;
	else if (cpuVer == A64)
		return num + 224;
	return -1;
}

static int PI(int num) {
	if (cpuVer == A20)
		return num + 201;
	else if (cpuVer == A64)
		return num + 256;
	return -1;
}

static int PJ(int num) {
	if (cpuVer == A20)
		return -1;
	else if (cpuVer == A64)
		return num + 288;
	return -1;
}
static int PK(int num) {
	if (cpuVer == A20)
		return -1;
	else if (cpuVer == A64)
		return num + 320;
	return -1;
}
static int PL(int num) {
	if (cpuVer == A20)
		return -1;
	else if (cpuVer == A64)
		return num + 352;
	return -1;
}
static int PM(int num) {
	if (cpuVer == A20)
		return -1;
	else if (cpuVer == A64)
		return (num + 384);
	return -1;
}
static int PN(int num) {
	if (cpuVer == A20)
		return -1;
	else if (cpuVer == A64)
		return (num + 416);
	return -1;

}
static int PO(int num) {
	if (cpuVer == A20)
		return -1;
	else if (cpuVer == A64)
		return (num + 448);
	return -1;
}
static int GPIO(int num) {
	return 32 * num;
}
static int isNum(char *str){
	if(str ==NULL ||*str == 0 )
		return 0;
	while(*str){
		if((*str <'0') || (*str >'9')){
			return 0;
		}
		str ++;
	}
	return 1;
}
static int resolverGpio(char *gpioStr) {
	typedef struct GpioResolver {
		char *gpioStr;
		int (*func)(int);
	} GpioResolver;
	GpioResolver resolverList[] = { { "PA", PA }, { "PB", PB }, { "PC", PC }, {
			"PD", PD }, { "PE", PE }, { "PF", PF }, { "PG", PG }, { "PH", PH },
			{ "PI", PI }, { "PJ", PJ }, { "PK", PK }, { "PL", PL },
			{ "PM", PM }, { "PN", PN }, { "PO", PO }, { "GPIO", GPIO }, };
	int i, lGpioInt = -1;
	char *lGpioStr = NULL;
	for (i = 0; i < sizeof(resolverList) / sizeof(resolverList[0]); i++) {
		if (!strncmp(resolverList[i].gpioStr, gpioStr,
				strlen(resolverList[i].gpioStr))) {

			if (strcmp(resolverList[i].gpioStr, "GPIO")) {
				//PB8
				lGpioStr = gpioStr + strlen(resolverList[i].gpioStr);
				if(!isNum(lGpioStr))
					return -1;
				lGpioInt = atoi(lGpioStr);
				return resolverList[i].func(lGpioInt);
			} else {
				//GPIO9_PB8
				int pg = -1, pi = -1;
				lGpioStr = gpioStr + strlen(resolverList[i].gpioStr);
				*(lGpioStr + 1) = 0;
				if(!isNum(lGpioStr))
					return -1;
				lGpioInt = atoi(lGpioStr);
				pg = resolverList[i].func(lGpioInt);
				pi = resolverGpio(lGpioStr + 2);
				if (pg < 0 || pi < 0) {
					return -1;
				}
				return pg + pi;
			}
		}
	}
	return -1;
}

static int getDoorLockPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(OPENDOOR_PIN, value);
	if (ret < 0) {
		goto fail0;
	}

	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", OPENDOOR_PIN, gpiopin);
	return gpiopin;
	fail0: return -1;
}
static int getOpenDoorKeyPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(INTERIORKEY_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", INTERIORKEY_PIN,gpiopin);

	return gpiopin;
	fail0: return -1;
}

static int getLightSensorPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(LIGHTSENSOR_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", LIGHTSENSOR_PIN, gpiopin);

	return gpiopin;
	fail0: return -1;
}
static int getCameraLightPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(VL_CAMERALED_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", VL_CAMERALED_PIN, gpiopin);

	return gpiopin;
	fail0: return -1;
}
static int getIFCameraLightPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(IF_CAMERALED_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", IF_CAMERALED_PIN, gpiopin);
	return gpiopin;
	fail0: return -1;

}
static int getKeyLightPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(KEYBOARD_LIGHT_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", KEYBOARD_LIGHT_PIN, gpiopin);

	return gpiopin;
	fail0: return -1;
}
static int getLcdSwichPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(LCDBACKLIGHT_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", LCDBACKLIGHT_PIN,gpiopin);

	return gpiopin;
	fail0: return -1;
}
static int getRestartPin(void) {
	return -1;
}
static int getPirPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(PIR_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", PIR_PIN, gpiopin);
	return gpiopin;
	fail0: return -1;
}
//门磁
static int getDoorMagneticPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin =  -1;
	ret = readKey(DOORMAGNETIC_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", DOORMAGNETIC_PIN,gpiopin);
	return gpiopin;
	fail0: return -1;
}
static int getSecurityPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin =  -1;
	ret = readKey(PREVENT_SEPARATE_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", PREVENT_SEPARATE_PIN,gpiopin);
	return gpiopin;
	fail0: return -1;
}
static char *getBluetoothUART(void) {
	int ret = -1;
	static char value[128] = { 0 };
	ret = readKey(BT_UART, value);
	if (ret < 0) {
		goto fail0;
	}
	LOGD("[%s]:%s\n", BT_UART, value);

	return value;
	fail0: return NULL;
}
static char *getDoorCardUART(void) {
	int ret = -1;
	static char value[128] = { 0 };
	ret = readKey(IC_CARD_UART, value);
	if (ret < 0) {
		goto fail0;
	}
	LOGD("[%s]:%s\n", IC_CARD_UART, value);
	return value;
	fail0: return NULL;

}
static char * getIdCardUART(void) {
	int ret = -1;
	static char value[128] = { 0 };
	ret = readKey(ID_CARD_UART, value);
	if (ret < 0) {
		goto fail0;
	}
	LOGD("[%s]:%s\n", ID_CARD_UART, value);
	return value;
	fail0: return NULL;
}
static DOOR_CARD_MODULE getDoorType(void) {

	return -1;
}
static char *getRs485UART(void) {
	int ret = -1;
		static char value[128] = { 0 };
		ret = readKey(ID_CARD_UART, value);
		if (ret < 0) {
			goto fail0;
		}
		LOGD("[%s]:%s\n", ID_CARD_UART, value);
		return value;
		fail0: return NULL;
}
static int getRs485controlPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(RS485CONTROL_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", RS485CONTROL_PIN, gpiopin);
	return gpiopin;
	fail0: return -1;
}
static int getRedLedPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(R_LED_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", PREVENT_SEPARATE_PIN, gpiopin);
	return gpiopin;
	fail0: return -1;
}
static int getGreenLedPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(G_LED_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", PREVENT_SEPARATE_PIN, gpiopin);
	return gpiopin;
	fail0: return -1;
}
static int getBlueLedPin(void) {
	int ret = -1;
	char value[128] = { 0 };
	int gpiopin = -1;
	ret = readKey(B_LED_PIN, value);
	if (ret < 0) {
		goto fail0;
	}
	gpiopin =  resolverGpio(value);
	LOGD("[%s]:%d\n", PREVENT_SEPARATE_PIN, gpiopin);
	return gpiopin;
	fail0: return -1;
}

void getValue(char *line, char *value) {
	char * ptemp = line;
	while (*ptemp) {
		if (*ptemp++ == '=') {
			break;
		}
	}
	char *pval = ptemp;
	char *seps = " \n\r\t";
	int offset = 0;
	pval = strtok(pval, seps);
	while (pval != NULL) {
		strncpy(value + offset, pval, strlen(pval));
		offset += strlen(pval);
		pval = strtok(NULL, seps);
	}
	*(value + offset) = 0;
}
static int readKey(char *key, char *value) {
#define KEY_LENGTH  2048
	char bRet = false;
	char bFlagBegin = false;
	char strId[2];
	char str[KEY_LENGTH] = { 0 };
	FILE * mhKeyFile = NULL;
	if (key == 0 || value == 0) {
		LOGE("error input para");
		return false;
	}
	mhKeyFile = fopen(HW_CONFIG_PATH, "rb");
	if (mhKeyFile <= 0) {
		LOGE("open file %s failed", HW_CONFIG_PATH);
		return false;
	} else {
		LOGD("open file %s OK", HW_CONFIG_PATH);
	}
	fseek(mhKeyFile, 0L, SEEK_SET);

	memset(str, 0, KEY_LENGTH);
	while (fgets(str, KEY_LENGTH, mhKeyFile)) {
		if (!strncmp(key, str, strlen(key))) {
			getValue(str, value);
			bRet = true;
			break;
		}
		memset(str, 0, KEY_LENGTH);
	}
	fclose(mhKeyFile);
	return bRet;
}

static HwInterfaceOps ops = {
		.getDoorLockPin = getDoorLockPin, //控制门锁
		.getRedLedPin = getRedLedPin, //获取红色LED PIN
		.getGreenLedPin = getGreenLedPin, //获取绿色LED PIN
		.getBlueLedPin = getBlueLedPin, //获取蓝色LED PIN
		.getOpenDoorKeyPin = getOpenDoorKeyPin, //控制内部开门按钮
		.getLightSensorPin = getLightSensorPin, //摄像头光敏反馈
		.getCameraLightPin = getCameraLightPin, //控制摄像头灯
		.getIFCameraLightPin = getIFCameraLightPin, //控制摄像头灯
		.getKeyLightPin = getKeyLightPin, //控制键盘灯
		.getLcdSwichPin = getLcdSwichPin, //控制屏幕背光
		.getRestartPin = getRestartPin, //重启机器
		.getPirPin = getPirPin, //人体红外按钮
		.getDoorCardUART = getDoorCardUART, //获取门卡
		.getDoorType = getDoorType, //获取门卡模块类型
		.getDoorMagneticPin = getDoorMagneticPin, //获取门磁反馈
		.getSecurityPin = getSecurityPin, //获取防拆按钮反馈
		.getIdCardUART = getIdCardUART, //获取身份证对应的串口号
		.getBluetoothUART = getBluetoothUART, .getRs485UART = getRs485UART,
		.getRs485controlPin = getRs485controlPin, };

pHwInterfaceOps getHwInterfaceConfigServer(void) {
	LOGD("getHwInterfaceConfigServer\n");
	cpuVer = getUtilsOps()->getCpuVer();
	 if((access(HW_CONFIG_PATH,F_OK))!=-1)
	 {
		 return &ops;
	 }
	 return NULL;
}
