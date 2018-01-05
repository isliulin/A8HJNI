package com.welbell.hardwaretest;


import com.welbell.hardware.HardWareUpEvent;
import com.welbell.hardware.HardwareSupport;
import com.wlbell.hardwaretest.R;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;

public class MainActivity extends Activity {

	static final String TAG = "HardwareDemo";
	RecvHardwareEvent upEvent = new RecvHardwareEvent();
	HardwareSupport hardwareResource = new HardwareSupport();
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		Log.d("TAG","ver:"+hardwareResource.getHardWareVersion());
		hardwareResource.addEventCallBack(upEvent);
		hardwareResource.executeRootShell("ifconfig eth0 192.168.1.88");
	}
	
	private class  RecvHardwareEvent implements HardWareUpEvent{
		@Override
		public void someoneCloseEvent() {
			// TODO Auto-generated method stub
			Log.d("RecvHardwareEvent","有人靠近了!");
			//hardwareResource.reboot();
		}

		@Override
		public void doorLockKeyEvent(byte keyState) {
			// TODO Auto-generated method stub
			if(keyState == this.KEY_DOWN){
				Log.d("RecvHardwareEvent","按键按下");
			}else if(keyState == this.KEY_UP){
					Log.d("RecvHardwareEvent","按键抬起");	
			}	
		}
		@Override
		/*键盘事件回调函数
		 * code:键值
		 * value：按键状态
		 * 
		 * */
		public void keyBoardEvent(int code, int value) {
			// TODO Auto-generated method stub
			Log.d("keyBoardEvent"," code:"+code+
					" value:"+(value==1?"down":"up"));
		}

		@Override
		public void icCardBandAlgEvent(String icCardID) {
			// TODO Auto-generated method stub
			
			Log.d("icCardBandAlgEvent"," icCardID:"+icCardID);
			
		}

		@Override
		public void icCardBandRawEvent(byte[] icCardID) {
			for(int i = 0;i<icCardID.length;i++)
			{
				Log.d("icCardBandRawEvent"," icCardID:"+icCardID[i]);
			}
			
		}
		
	}

	
	

}

 