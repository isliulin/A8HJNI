package com.welbell.hardware;

public interface HardWareUpEvent {
	
	static final byte KEY_DOWN = 1;
	static final byte KEY_UP = 0;
	
	static final byte IC_RAWDATA = 0;//IC卡裸数据
	static final byte IC_ALGDATA = 1;//IC卡带算法的数据
	
	public void someoneCloseEvent();
	public void doorLockKeyEvent(byte keyState);
	public void icCardBandAlgEvent(String icCardID);
	public void icCardBandRawEvent(byte []icCardID);
	public void keyBoardEvent(int code ,int value);
}
