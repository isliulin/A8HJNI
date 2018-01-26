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
		
		Log.e("end","11");
		hardwareResource.executeRootShell("ls");
		Log.e("end","22");
		hardwareResource.addAPPtoDaemon("com.welbell.hardwaretest  ", "  com.welbell.hardwaretest.MainActivity");
		Log.e("end","33");
			hardwareResource.executeRootShell("ifconfig eth0 192.168.1.88");
		/*hardwareResource.executeRootShell("am force-stop com.wlbell.hardwaretest&&" +
				"pm install -r /mnt/sdcard/app.apk&&" +
				"am start -a  android.intent.action.MAIN -n com.welbell.hardwaretestdemo/.MainActivity");
		*/
		//hardwareResource.executeRootShell("pm install -r /mnt/sdcard/app.apk");
		//hardwareResource.executeRootShell("am start -a  android.intent.action.MAIN -n com.welbell.hardwaretestdemo/.MainActivity");
		/*com.welbell.hardwaretestdemo
		//&& pm install -r /mnt/sdcard/huarui/2016-08-08/huarui.apk 
		//&& am start -a android.intent.action.MAIN -n com.jack.huarui.capture/.mvp.ui.activity.MainAty
		
		*/
		
//		//开启摄像头灯
//		hardwareResource.cameraLightControl(true);
//		//开门
//		hardwareResource.doorLockControl(true);
//		//开启键盘灯
//		hardwareResource.keyboardLightControl(true);
//		//开启红外摄像头灯
//		hardwareResource.ifcameraLightControl(true);
		//关闭屏幕
		//hardwareResource.screenBlacklightControl(false);
		//重启机器
		//hardwareResource.reboot();

	}
	


	@Override
	public void someoneCloseEvent() {
		// TODO Auto-generated method stub
		Log.e("someoneCloseEvent","有人靠近");
	//	hardwareResource.executeRootShell("ifconfig eth0 192.168.1.128");
	}
	@Override
	public void doorLockKeyEvent(byte keyState) {
		// TODO Auto-generated method stub
		
	}
	@Override
	public void icCardBandAlgEvent(String icCardID) {
		// TODO Auto-generated method stub
		Log.e("有人刷卡了，卡号:",icCardID);
		hardwareResource.executeRootShell("ifconfig eth0 192.168.1.25");
	}
	@Override
	public void icCardBandRawEvent(byte[] icCardID) {
		// TODO Auto-generated method stub
		
	}
	@Override
	public void keyBoardEvent(int code, int value) {
		// TODO Auto-generated method stub
		
			//this.systemNavBar(false);
		Log.e("keyBoardEvent"," code:"+code+
			" value:"+(value==1?"down":"up"));
		
	}
	
//	private class  RecvHardwareEvent implements HardWareUpEvent{
//		@Override
//		//人体红外回调，表示有人靠近
//		public void someoneCloseEvent() {
//			// TODO Auto-generated method stub
//			Log.d("RecvHardwareEvent","有人靠近了!");
//			
//		}
//
//		@Override
//		//内部开门按键回调
//		public void doorLockKeyEvent(byte keyState) {
//			// TODO Auto-generated method stub
//			if(keyState == this.KEY_DOWN){
//				Log.d("RecvHardwareEvent","按键按下");
//			}else if(keyState == this.KEY_UP){
//					Log.d("RecvHardwareEvent","按键抬起");	
//			}	
//		}
//		@Override
//		/*键盘事件回调函数
//		 * code:键值
//		 * value：按键状态
//		 * 
//		 * */
//		public void keyBoardEvent(int code, int value) {
//			// TODO Auto-generated method stub
//			Log.d("keyBoardEvent"," code:"+code+
//					" value:"+(value==1?"down":"up"));
//		}
//
//		@Override
//		//带算法的IC卡号
//		public void icCardBandAlgEvent(String icCardID) {
//			// TODO Auto-generated method stub
//			
//			Log.d("icCardBandAlgEvent"," icCardID:"+icCardID);
//			
//		}
//
//		@Override
//		//不带算法的IC卡号
//		public void icCardBandRawEvent(byte[] icCardID) {
//			for(int i = 0;i<icCardID.length;i++)
//			{
//				Log.d("icCardBandRawEvent"," icCardID:"+icCardID[i]);
//			}
//			
//		}
//		
//		
//	}

	
	

}

 