
#ifndef __GPIO_OPS__H
#define __GPIO_OPS__H


#define GPIO_DIR_OUT 1
#define GPIO_DIR_IN  0
#define GPIO_OUT_HIG 1
#define GPIO_OUT_LOW 0
typedef enum INTERRUPT_MODE{
	NONE = 0,
	RISING ,
	FALLING,
	BOTH,
}INTERRUPT_MODE;
typedef struct {
	unsigned int pin;
	unsigned int state;
	void *interruptArg;
}GpioPinState,*pGpioPinState;

typedef int (*T_InterruptFunc)(pGpioPinState );
//GPIO服务的函数操作集
typedef struct GpioOps{

	int (*setOutputValue)(struct GpioOps *,int);//设置输出值
	int (*getInputValue)(struct GpioOps *);//获取输入值
	int (*setInterruptFunc)(struct GpioOps *,T_InterruptFunc,void *,INTERRUPT_MODE);
	//设置中断触发函数以及触发方式
}GpioOps,*pGpioOps;
//创建gpio服务对象
pGpioOps gpio_getServer(unsigned int gpio);
//销毁gpio服务对象
void     gpio_releaseServer(pGpioOps *ops);


#endif
