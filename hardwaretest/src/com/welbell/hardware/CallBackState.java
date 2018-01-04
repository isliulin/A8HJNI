package com.welbell.hardware;

/**
 * Created by lijiarui on 2017/3/31.
 */
public class CallBackState {
    public static final byte UI_INFRARED_DEVICE = 0X30; // 红外
    public static final byte UI_DOORCARD_DEVICE = 0X31; // 门卡
    public static final byte UI_FACE_RECOGNITION = 0X32; // 人脸识别
    public static final byte UI_FINGERPRINT_RECOGNITION = 0X33; // 指纹识别
    public static final byte UI_DOORCARD_DEVICE_ALG = 0X35; // 门卡带校验算法
    public static final byte UI_OPENDOOR_KEY_DOWN = 0X36;   //内部开门按键按下时会调用
    public static final byte UI_KEYBOARD_EVENT = 0X41;
}
