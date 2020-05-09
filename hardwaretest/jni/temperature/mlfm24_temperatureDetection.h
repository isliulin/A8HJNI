#ifndef WB_MLFM24_TEMPERATUREDETECTION_H__
#define WB_MLFM24_TEMPERATUREDETECTION_H__

#ifdef __cplusplus             //告诉编译器，这部分代码按C语言的格式进行编译，而不是C++的
extern "C"{
#endif

#ifndef TemperatureDetection
#define TemperatureDetection

typedef struct TemperatureDetectionOps{
	/*
	 * 功能:设置波特率
	 *  参数:
	 * 		ops:操作对象
	 * 		rate:波特率 (9600,19200,38400,115200,921600)
	 * 	返回值:
	 * 		0:成功 -1:失败
	 * */
	int (*setBaudRate)(struct TemperatureDetectionOps * ops,int rate);
	/*功能:设置温度补偿系数
	 *  参数:
	 * 		ops:操作对象
	 * 		parameter:补偿系数 范围(0.93-1.0)
	 *
	 * 	返回值:
	 * 		0:成功 -1:失败
	 * */

	int (*setTemperatureCompensation)(struct TemperatureDetectionOps * ops,float parameter);

	/*功能:获取所有像素点温度
	 *  参数:
	 * 		ops:操作对象
	 * 		data:数据指针
	 * 		len:数据长度 <=1024
	 * 	返回值:
	 * 		0:成功 -1:失败
	 * */

	int (*getGlobalTemperature)(struct TemperatureDetectionOps * ops, float *data,int len);
	/*功能:获取特殊温度值
	 *  参数:
	 * 		centre:坐标中心温度
	 * 		max:最大值
	 * 		mini:最小值
	 * 	返回值:
	 * 		0:成功 -1:失败
	 * */
	int (*getSpecialTemperature)(struct TemperatureDetectionOps * ops,float  *centre,float *max,float *mini);



}TemperatureDetectionOps,*pTemperatureDetectionOps;

#endif
/*功能:创建温度模块对象
 *参数:
 *	 uartPath:串口地址
 *返回值:
 *	非NULL则为对象值，NULL则创建失败
 * */
pTemperatureDetectionOps createMlfm24TemperatureDetectionServer(const char *uartPath);
/*功能:销毁温度模块对象
 *参数:
 *	 server:对象指针
 *返回值:
 *	无
 * */
void destroyMlfm24TemperatureDetectionServer(pTemperatureDetectionOps *server);

/*
 * 使用案例
 *
 *	pTemperatureDetectionOps server = createTemperatureDetectionServer("/dev/ttyS1");
 *
 *	...
 *	float centre;
 *	float max;
 *	float mini;
 *  server->getSpecialTemperature(server,&centre,&max,&mini);
 *
 *	faloat globalTemp[1024];
 *  server->getGlobalTemperature(server, globalTemp,1024);
 *
 *
 *	destroyTemperatureDetectionServer(&server);
 *
 * */



#ifdef __cplusplus             //告诉编译器，这部分代码按C语言的格式进行编译，而不是C++的
}
#endif


#endif
