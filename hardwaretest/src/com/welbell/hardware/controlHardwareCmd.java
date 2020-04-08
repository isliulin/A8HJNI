package com.welbell.hardware;


public class controlHardwareCmd {
	public class LED_TYPE{
		 public static final byte RED = 0;
		 public static final byte GREEN = 1;
		 public static final byte BLUE = 2;
	};
    public static final byte E_DOOR_LOCK = 0x12; // 锁
    /* 控制摄像头灯 例：开灯：buf[] = {0x14,0x01} */
    public static final byte E_CAMERA_LIGHT = 0x14; // 摄像头灯
    public static final byte E_IFCAMERA_LIGHT = 0x15;
    
    /* 控制键盘灯 例：开灯：buf[] = {0x15,0x01} */
    public static final byte E_KEY_LIGHT = 0x16; // 键盘灯
    /* 控制屏幕背光 例：灭屏：buf[] = {0x16,0x01} */
    public static final byte E_LCD_BACKLIGHT = 0x17; // 屏幕背光
    /* 指纹识别 ，方法见fingerprintCmd */
    public static final byte E_RESTART = 0x1A; // 重启机器
    public static final byte E_GET_HARDWARE_VER = 0X1B; //获取硬件版本号
    public static final byte E_EXECURT_SHELL = 0x1C; // 以root权限执行shell脚本
    public static final byte E_ADD_GUARD = 0x1D; //添加app到守护进程中
    public static final byte E_DEL_GUARD = 0x1E; //添加app到守护进程中
    public static final byte E_GET_OPTO_SENSOR_STATE = 0x1F; //获取光感状态
    public static final byte E_GET_IDCARD_UARTDEV = 0x20; //获取身份证对应的串口号
    public static final byte E_GET_CPUMODEL = 0x21;//获取CPU型号
    public static final byte E_GET_BLUETOOTH_STATE = 0x22; //获取蓝牙状态
    public static final byte E_SET_BLUENAME = 0x23; //设置蓝牙名
    public static final byte E_SEND_BLUESTR = 0x24;
    public static final byte E_SET_BLUETOOTH_REBOOT = 0x25;
    public static final byte E_GET_ICCARD_STATE = 0x26;
    public static final byte E_SET_RS485INIT = 0x27;	//RS485初始化
    public static final byte E_SET_RS485SEND  = 0x28;	//RS485发送函数
    public static final byte E_GET_RS485RECV = 0x29;    //RS485接收函数
    public static final byte E_SET_RGB_LED   = 0x2a;
    public static final byte E_SEND_HEARBEAT   = 0x2b;
    public static final byte E_GET_GLOBAL_TEMP = 0x2c; 			//获取全部温度数据
    public static final byte E_GET_SPECIAL_TEMP = 0x2d; 		//获取特殊温度数据
    public static final byte E_SET_COMPENSATION_TEMP = 0x2e;    //设置温度系数(0.93-1.0)
    

}
