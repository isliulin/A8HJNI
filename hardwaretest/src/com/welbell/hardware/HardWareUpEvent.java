package com.welbell.hardware;

public interface HardWareUpEvent {
	
	static final byte KEY_DOWN = 1;
	static final byte KEY_UP = 0;
	
	public void someoneCloseEvent();
	public void doorLockKeyEvent(byte keyState);
	public void icCardBandAlgEvent(String icCardID);
	public void icCardBandRawEvent(byte []icCardID);
	public void keyBoardEvent(int code ,int value);
}
