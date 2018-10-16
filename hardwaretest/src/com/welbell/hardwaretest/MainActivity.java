package com.welbell.hardwaretest;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.LineNumberReader;

import com.welbell.hardware.HardWareUpEvent;
import com.welbell.hardware.HardwareSupport;
import com.welbell.hardwaretest.R;

import android.os.Build;
import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.View;

public class MainActivity extends Activity implements HardWareUpEvent {

	static final String TAG = "HardwareDemo";
	HardwareSupport hardwareResource = new HardwareSupport();
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		int ret;
		// 获取版本号
		
		setContentView(R.layout.activity_main);
	
		ret = hardwareResource.init();
		if(ret < 0)
		{
			Log.e("", " fail to hardwareResource ");
			return ;
			
		}
		//执行shell脚本: 例 执行重启命令 
		//hardwareResource.executeRootShell("reboot");
		// 获取身份证模块所接的串口号
		String IdCardUartDEV = hardwareResource.getIdCardUartDev();

		Log.e("", "IdCardUartDEV:" + IdCardUartDEV);
		// 获取系统版本号
	//	String version = hardwareResource.getHardWareVersion();
	//	Log.e("", "************version:" + version);
		// 获取CPU型号
		String cpumodel = hardwareResource.getCpuModel();


		// 添加接口
		hardwareResource.addEventCallBack(this);
		/*
		 * 参数1：app 包名 参数2：主Activity全类名
		 */
		hardwareResource.addAPPtoDaemon("com.welbell.hardwaretest",
				"com.welbell.hardwaretest.MainActivity");
		hardwareResource.getCpuModel();
		
		// 开启摄像头灯
		hardwareResource.cameraLightControl(false);
		// 开门
		
		hardwareResource.doorLockControl(true);
		// 关闭键盘灯
		hardwareResource.keyboardLightControl(true);
		// 关闭红外摄像头灯
		hardwareResource.ifcameraLightControl(false);
		// 关闭屏幕
		hardwareResource.screenBlacklightControl(true);
		
		if( true == hardwareResource.getBuletoothState())
		{
			hardwareResource.setBluetoothName("快来链接我");
		}
		
		//hardwareResource.rebootBluetooth();
		// 重启机器
		// hardwareResource.reboot();
		// 获取光感状态
		// 返回值 1：表示亮 0：表示暗 -1：表示获取失败
		// hardwareResource.getOptoSensorState();

		// 删除守护服务
		// hardwareResource.delDaemonServer();
		// 删除回调接口
		// hardwareResource.removeEventCallBack(this);
		//用root权限执行命令
		//hardwareResource.executeRootShell("reboot");
	}

	@Override
	public void someoneCloseEvent() {
		// TODO Auto-generated method stub
		Log.e("someoneCloseEvent",
				"有人靠近 光敏状态是:" + hardwareResource.getOptoSensorState());
		
		if( true == hardwareResource.getBuletoothState())
		{
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
	public void  buletoothEvent(String data){
		Log.e("buletoothEvent", " data:" +data);
		
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
