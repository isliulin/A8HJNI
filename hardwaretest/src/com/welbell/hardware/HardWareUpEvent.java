package com.welbell.hardware;

public interface HardWareUpEvent {
	
	static final byte KEY_DOWN = 1;
	static final byte KEY_UP = 0;
	static final byte UI_DOORCARD_TYPE_IC = 0;
	static final byte UI_DOORCARD_TYPE_CPU = 1;
	static final byte UI_DOORCARD_TYPE_ID  = 2;           
	
	public void someoneCloseEvent();
	public void doorLockKeyEvent(byte keyState);
	public void doorMagneticEvent(byte keyState);
	public void preventSeparateEvent(byte keyState);
	public void doorCardBandAlgEvent(byte type,String icCardID);
	public void doorCardBandRawEvent(byte type ,byte []icCardID);
	public void keyBoardEvent(int code ,int value);
	public void buletoothEvent(String data);
}
