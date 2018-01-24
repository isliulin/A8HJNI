LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := NativeHardwareSupport


//LOCAL_SRC_FILES +=  binder/binder.c 		binder/binderClient.c
LOCAL_SRC_FILES += 	common/bufferManage.c	common/CallbackJavaMethod.c common/Utils.c common/netUdpServer.c
LOCAL_SRC_FILES += 	common/communicationServer.c common/nativeNetServer.c

LOCAL_SRC_FILES +=  gpio/hwInterfaceManage.c    gpio/gpioServer.c
LOCAL_SRC_FILES +=  serial/serialServer.c
LOCAL_SRC_FILES +=  taskManage/threadManage.c taskManage/timerTaskManage.c

LOCAL_SRC_FILES += A8DeviceControl.c JNI_OnLoad.c \
				   WB_icDoorCard.c WB_pirSupport.c WB_hardwareSupport.c WB_keyboard.c
LOCAL_SRC_FILES += WB_virtualHardwareSupport.c WB_guardThread.c


				   
LOCAL_CFLAGS += -O3 -DNDEBUG -fstrict-aliasing
#LOCAL_MULTILIB := both					
LOCAL_LDLIBS   := -llog 


# -lGLESv2		
#				-L/android/SDK/SmartHome/A8_Pad/android/out/target/product/welbell-A8_Pad/system/lib -lcutils -lutils -lbinder \
#				-lui -lgui -landroid_runtime -lstagefright_foundation 
				
LOCAL_CFLAGS := -DHAVE_PTHREADS
    
#如果是编译同时带有线网络和无线网络的分机客户端，使用"DFOR_LOCAL_CLIENT"定义，其他移动端设备使用"DFOR_GENERIC_CLIENT"
#如果用到指纹识别请设置以下参数
LOCAL_CFLAGS += -DUSER_FINGER
#如果用到IC卡请设置以下参数
LOCAL_CFLAGS += -DUSER_ICCARD
#如果用到门铃请设置以下参数
#LOCAL_CFLAGS += -DUSER_DOOR_BELL
#如果用到硬件的人脸识别请设置以下参数
#LOCAL_CFLAGS += -DUSER_FACE
#如果用到人体红外请设置以下参数
LOCAL_CFLAGS += -DUSER_PIR
#光敏控制
#LOCAL_CFLAGS += -DUSER_LIGHT_SENSOR



#A20平台使用DUSER_A20参数，A64平台使用DUSER_A64参数
LOCAL_CFLAGS += -DUSER_A64
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
