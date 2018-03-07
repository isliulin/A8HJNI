package com.welbell.hardwaretest;


import com.welbell.hardware.HardWareUpEvent;
import com.welbell.hardware.HardwareSupport;
import com.welbell.hardwaretest.R;

import android.os.Build;
import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.View;

public class MainActivity extends Activity implements HardWareUpEvent{

	static final String TAG = "HardwareDemo";
//	RecvHardwareEvent upEvent = new RecvHardwareEvent();
	HardwareSupport hardwareResource = new HardwareSupport();
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//获取版本号

		setContentView(R.layout.activity_main);
		String version;
		version = hardwareResource.getHardWareVersion();
		//添加接口
		hardwareResource.addEventCallBack(this);
		//以root权限执行shell脚本:设置IP地址
		try {
			Thread.sleep(1);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		
		hardwareResource.executeRootShell("ls");
		//把本进程添加到守护进程中 
		/*
		 * 参数1：app 包名
		 * 参数2：主Activity全类名
		 * */
		
		hardwareResource.addAPPtoDaemon("com.welbell.hardwaretest", "com.welbell.hardwaretest.MainActivity");
	
		hardwareResource.executeRootShell("ifconfig eth0 192.168.1.88");

		
		//开启摄像头灯
		hardwareResource.cameraLightControl(true);
		//开门
		hardwareResource.doorLockControl(true);
		//开启键盘灯
		hardwareResource.keyboardLightControl(true);
		//开启红外摄像头灯
		hardwareResource.ifcameraLightControl(true);
		//关闭屏幕
		hardwareResource.screenBlacklightControl(true);
		//重启机器
	//	hardwareResource.reboot();
		//获取光感状态
		//返回值  1：表示亮    0：表示暗   -1：表示获取失败
		//hardwareResource.getOptoSensorState();
		
		//删除守护服务
		//hardwareResource.delDaemonServer();
		//删除回调接口
		//hardwareResource.removeEventCallBack(this);
		

	}
	


	@Override
	public void someoneCloseEvent() {
		// TODO Auto-generated method stub
		Log.e("someoneCloseEvent","有人靠近 光敏状态是:"+hardwareResource.getOptoSensorState());
		
	}
	@Override
	public void doorLockKeyEvent(byte keyState) {
		// TODO Auto-generated method stub
		
	}
	@Override
	public void doorCardBandAlgEvent( byte type,String icCardID) {
		// TODO Auto-generated method stub
		String cardType[] = {"IC卡","CPU卡","身份证"};
		Log.e("","有人刷"+cardType[type]+"CardID:"+icCardID);
		hardwareResource.delDaemonServer();
		hardwareResource.removeEventCallBack(this);
	}
	@Override
	public void doorCardBandRawEvent(byte type,byte[] icCardID) {
		// TODO Auto-generated method stub
		String cardType[] = {"IC卡","CPU卡","身份证"};
		Log.e("","有人刷"+cardType[type]);
		for(int i = 0;i < icCardID.length;i++)
		{
			Log.e("",""+icCardID[i]);
		}
		
	}
	@Override
	public void keyBoardEvent(int code, int value) {
		// TODO Auto-generated method stub
		
			//this.systemNavBar(false);
		Log.e("keyBoardEvent"," code:"+code+
			" value:"+(value==1?"down":"up"));
		
	}
	


	
	

}

 