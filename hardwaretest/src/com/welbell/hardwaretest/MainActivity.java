package com.welbell.hardwaretest;

import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.LineNumberReader;

import com.welbell.hardware.HardWareUpEvent;
import com.welbell.hardware.HardwareSupport;
import com.welbell.hardwaretest.R;

import android.os.Build;
import android.os.Bundle;
import android.provider.Settings.System;
import android.app.Activity;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;

public class MainActivity<EthernetDevInfo> extends Activity implements HardWareUpEvent {

	static final String TAG = "HardwareDemo";
	HardwareSupport hardwareResource = new HardwareSupport();

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		int ret;
		// 获取版本号
//		TelephonyManager telephonyManager = (TelephonyManager) getSystemService(TELEPHONY_SERVICE);
//        String simSerialNumber = telephonyManager.getSimOperatorName();
        
		 File myFile = new File("/mnt/sdcard/test");
		   //判断文件是否存在，如果不存在则调用createNewFile()方法创建新目录，否则跳至异常处理代码
		   if(!myFile.exists())
			try {
				myFile.createNewFile();
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
  
		setContentView(R.layout.activity_main);
		Log.e(TAG, " init start!");
		ret = hardwareResource.init();
		if (ret < 0) {
			Log.e("", " fail to hardwareResource!   ret =  "+ret);
			return;
		}
		Log.e(TAG, " init end!");
		// 执行shell脚本: 例 执行重启命令
		// hardwareResource.executeRootShell("reboot");
		// 获取身份证模块所接的串口号
		String IdCardUartDEV = hardwareResource.getIdCardUartDev();

		Log.e("", "IdCardUartDEV:" + IdCardUartDEV);
		// 获取系统版本号
		// String version = hardwareResource.getHardWareVersion();
		// Log.e("", "************version:" + version);
		// 获取CPU型号
		String cpumodel = hardwareResource.getCpuModel();
		// 获取ZLG600A门卡状态 有装IC卡返回0 没装返回小于<=0.使用
		// 中控读卡模块前先获取此状态，如果返回<=0才能调用
		int state = hardwareResource.getIcCardState();
		Log.e("", "IdCardstate :" + state);

		// 添加接口
		hardwareResource.addEventCallBack(this);
		/*
		 * 参数1：app 包名 参数2：主Activity全类名
		 */
		/*
		hardwareResource.addAPPtoDaemon("com.welbell.hardwaretest",
				"com.welbell.hardwaretest.MainActivity");
		*/
		hardwareResource.getCpuModel();

		// 开启摄像头灯
		hardwareResource.cameraLightControl(true);
		// 开门 true:开 false:关

		hardwareResource.doorLockControl(true);
		// 关闭键盘灯
		hardwareResource.keyboardLightControl(true);
		// 关闭红外摄像头灯
		hardwareResource.ifcameraLightControl(true);
		// 关闭屏幕
		hardwareResource.screenBlacklightControl(true);
		hardwareResource.controlRedLed(true); //开灯
		hardwareResource.controlGreenLed(true);
		hardwareResource.controlBlueLed(true);
		hardwareResource.getCpuModel();
	
		if (true == hardwareResource.getBuletoothState()) {
			hardwareResource.setBluetoothName("HC01");
			hardwareResource.sendBluetoothStr("hello word!");
		}
		float[] temp = null;
		hardwareResource.setTempCompensation(0.99f);
		//temp = hardwareResource.getSpecialTemp();
		//Log.d(TAG,"====="+temp[0] +" "+ temp[1] +" "+temp[2] );
		try {
			Thread.sleep(1000);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		temp = hardwareResource.getGlobalTemp();
		if(temp !=null)
			Log.d(TAG,"====="+temp[0] +" "+ temp[1] +" "+temp[2] );
		/*
		//重启网络方案
		hardwareResource.executeRootShell("echo 0 > /sys/devices/misc_power_en.23/usbhub");
		try {
			Thread.sleep(500);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		hardwareResource.executeRootShell("echo 1 > /sys/devices/misc_power_en.23/usbhub");
		try {
			Thread.sleep(500);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		hardwareResource.executeRootShell("busybox ifconfig eth0 down && busybox ifconfig eth0 up");
		try {
			Thread.sleep(500);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		hardwareResource.executeRootShell("busybox udhcpc -i eth0");
		*/
		
		// hardwareResource.rebootBluetooth();
		// 重启机器
		// hardwareResource.reboot();
		// 获取光感状态
		// 返回值 1：表示亮 0：表示暗 -1：表示获取失败
		// hardwareResource.getOptoSensorState();

		// 删除守护服务
		// hardwareResource.delDaemonServer();
		// 删除回调接口
		// hardwareResource.removeEventCallBack(this);
		// 用root权限执行命令
		// hardwareResource.executeRootShell("mount -o remount /system && chmod 000 /system/priv-app/VpnDialogs/*");
		// rs485初始化,波特率9600，8个数据为
		//hardwareResource.rs485init( 9600, 8, 1, 'n');

		hardwareResource.executeRootShell("date -s  20181126.114951");
		hardwareResource.executeRootShell("date -s  20181126.114951");
	
		Log.d("hardware","iccard_type:******************"+ hardwareResource.getIcCardState());

		new Thread(new Runnable() {
			boolean falg = false;
			public void run() {
				boolean start = false;
				while (start) {
					falg = !falg;
					byte sendData[] = new byte[1240];
					if(falg == true){
						Log.d("rs485 ","send");
					//	hardwareResource.rs485send(sendData);
						hardwareResource.doorLockControl(falg);
						
						//每隔30分执行一次
						hardwareResource.executeRootShell("sysctl vm.drop_caches=3");
						hardwareResource.cameraLightControl(falg);
						hardwareResource.keyboardLightControl(falg);
						//hardwareResource.sendHearBeatToDaemon("com.welbell.hardwaretest",
						//		"com.welbell.hardwaretest.MainActivity");
						try {
							Thread.sleep(1*1000);
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}else {
						byte[] recvData;
						Log.d("rs485 ","recv");
//						recvData = hardwareResource.rs485recv(6000);
//						if(recvData == null){
//							continue;
//						}
//						for(int i = 0;i<recvData.length;i++ ){
//							
//							Log.d("rs485","[ " + recvData[i]+"]");
//						}
						hardwareResource.doorLockControl(falg);
						
						hardwareResource.keyboardLightControl(falg);
						try {
							Thread.sleep(1*1000);
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}			
				}
			}
		}).start();
	}

	@Override
	public void someoneCloseEvent() {
		// TODO Auto-generated method stub
		Log.e("someoneCloseEvent",
				"有人靠近 光敏状态是:" + hardwareResource.getOptoSensorState());

		if (true == hardwareResource.getBuletoothState()) {
			hardwareResource.sendBluetoothStr("someoneCloseEvent");
		}
	}

	@Override
	public void doorLockKeyEvent(byte keyState) {
		// TODO Auto-generated method stub
		Log.e("doorLockKeyEvent", ":" + keyState);
	}

	@Override
	public void doorCardBandAlgEvent(byte type, String icCardID) {
		// TODO Auto-generated method stub
		String cardType[] = { "IC卡", "CPU卡", "身份证" };
		Log.e("", "有人刷" + cardType[type] + "CardID:" + icCardID);
	}

	@Override
	public void doorCardBandRawEvent(byte type, byte[] icCardID) {
		// TODO Auto-generated method stub
		String cardType[] = { "IC卡", "CPU卡", "身份证" };
		Log.e("", "有人刷" + cardType[type]);
		for (int i = 0; i < icCardID.length; i++) {
			Log.e("", "" + icCardID[i]);
		}
	}

	@Override
	public void keyBoardEvent(int code, int value) {
		// TODO Auto-generated method stub
		// this.systemNavBar(false);
		Log.e("keyBoardEvent", " code:" + code + " value:"
				+ (value == 1 ? "down" : "up"));

	}
	
	
	
	private int getChCountFromStr(String str,char ch){
		int count = 0;
		for(int i =0 ;i < str.length();i++){
			if(str.charAt(i) == ch)
			{
				count ++;
			}		
		}

		return count ;			
	}

	public void buletoothEvent(String data) {
		Log.e("buletoothEvent", " data:" + data);
		
		


	}

	@Override
	public void doorMagneticEvent(byte keyState) {
		// TODO Auto-generated method stub
		Log.e("doorMagneticEvent", ":" + keyState);

	}

	@Override
	public void preventSeparateEvent(byte keyState) {
		// TODO Auto-generated method stub
		Log.e("preventSeparateEvent", ":" + keyState);
	}

}
