#ifndef A8_DEVICE_CONTROL_H
#define A8_DEVICE_CONTROL_H

typedef enum Interface{
	E_DOOEBEL  = 0X10,			//16 有线门铃
	E_SMART_HOME,				//17 智能家居(西安郑楠项目)
	E_DOOR_LOCK,				//18 锁
	E_INFRARED,					//19 红外
	E_CAMERA_LIGHT,				//20 摄像头灯
	E_KEY_LIGHT,				//21 键盘灯
	E_LCD_BACKLIGHT,			//22 屏幕背光
	E_FINGERPRINT,				//23 指纹
	E_SET_IPADDR,				//24 设置IP地址
	E_RESTART,				    //25 重启机器
	E_GET_HARDWARE_VER,			//26 获取硬件版本
	E_SEND_SHELL_CMD,           //27 发送SHELL指令
	E_ADD_GUARD,				//28 把APP加入到守护进程中
	E_DEL_GUARD,				//29 删除守护服务
	E_GET_OPTO_SENSOR_STATE,    //30 获取光感状态
	E_GET_IDCARD_UARTDEV,		//31 获取身份证对应的串口号
	E_MAX
}E_INTER_TYPE;


#define UI_BE_CALLED_RING		0x01 //有数据，数据结构为T_Room,表示来电者的信息
#define UI_TO_CALL_RING			0x02
#define UI_PEER_ANSWERED 		0x03
#define UI_PEER_HUNG_UP			0x04
#define UI_PEER_OFF_LINE		0x05
#define UI_PEER_NO_ANSWER		0x06
#define UI_PEER_LINE_BUSY		0x07
#define UI_TALK_TIME_OUT		0x08

#define	UI_WATCH_SUCCESS		0x09
#define UI_WATCH_TIME_OUT		0x0A

#define UI_OTHER_DEV_PROC 		0x0B
#define UI_HUNG_UP_SUCCESS		0x0C
#define UI_ANSWER_SUCESS		0x0D
#define UI_OPEN_LOCK_SUCESS 	0x0E
#define UI_UPDATE_TIME_WEATHRE	0x0F  //有数据，数据结构为　time_weather_t
#define UI_UPDATE_HOUSE_INFO	0x10 //有数据，数据结构为　T_Room,表示当前设备的房号信息
#define UI_ELEVATOR_CALL_SUCCESS   0x11
#define UI_ELEVATOR_CALL_FAIL   0x12
#define UI_DOORBEL_TRY_SUCCESS   0x13
#define UI_NETWORK_NOT_ONLINE    0x14 //通知UI 服务端 或本机已经掉线
#define UI_CONNECT_SERVER        0x15
#define UI_INFRARED_DEVICE       	             0X30   //红外
#define UI_DOORCARD_DEVICE      	             0X31   //门卡不带算法
#define UI_FACE_RECOGNITION 	 	 		     0X32   //人脸识别
#define UI_FINGERPRINT_RECOGNITION 	             0X33   //指纹识别
#define UI_LIGHT_SENSOR 	 			 		 0X34   //光敏传感器
#define UI_DOORCARD_DEVICE_ALG      	         0X35   //门卡带算法
#define UI_DOORCARD_TYPE_IC                      0
#define UI_DOORCARD_TYPE_CPU                     1
#define UI_DOORCARD_TYPE_ID                      2

#define UI_OPENDOOR_KEY_DOWN					 0X36
#define UI_HARDWARE_INFO	  	                 0X40   //获取FIRMWARE_VERSION

#define UI_KEYBOARD_EVENT					 0X41   //上报键盘事件
#define UI_MAGNETIC_EVENT                    0X42
#define UI_PREVENTSEPARATE_EVENT  			 0X43
#endif
















