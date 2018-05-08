#include <string.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <stdio.h>
#include <strings.h>
#include "hwInterface/gpioServer.h"
#include "taskManage/threadManage.h"
#include "common/debugLog.h"
#include "binder/binderClient.h"
#include "common/nativeNetServer.h"

#define SYSFS_GPIO_EXPORT           "/sys/class/gpio/export"
#define SYSFS_GPIO_UNEXPORT         "/sys/class/gpio/unexport"
#define SYSFS_GPIO_RST_DIR_OUT      "out"
#define SYSFS_GPIO_RST_DIR_IN       "in"
#define SYSFS_GPIO_RST_VAL_H        "1"
#define SYSFS_GPIO_RST_VAL_L        "0"


typedef struct GpioServer {
	GpioOps ops;
#if USER_BINDER == 1
	pBinderClientOps binderClient;
#else
	pNativeNetServerOps netClient;
#endif
	pThreadOps interruptThreadId;

	T_InterruptFunc interruptFunc;
	void *interruptArg;
	char gpioPath[64];
	unsigned int gpioPin;
	INTERRUPT_MODE interruptMode;
	int closeFd;
} GpioServer, *pGpioServer;
/*
 创建步骤gpio步骤：
 1. 导出
 # echo 55 > /sys/class/gpio/export
 2. 设置方向
 # echo out >/sys/class/gpio/gpio55/direction
 3. 查看方向
 # cat /sys/class/gpio/gpio55/direction
 4. 设置输出
 # echo 1 > /sys/class/gpio/gpio55/value
 5. 查看输出值
 # cat  /sys/class/gpio/gpio55/value
 6. 取消导出
 # echo 55 > /sys/class/gpio/unexport
 */
static int setOutputValue(struct GpioOps *base, int value);
static int getInputValue(struct GpioOps *base);
static int readgpioStateFromFd(int fd);
static void *gpioloopHandle(void *arg);
static int setInterruptFunc(struct GpioOps *base, T_InterruptFunc callBackfunc,
		void *interruptArg, INTERRUPT_MODE mode);
static int gpio_edge(int pin, INTERRUPT_MODE edge);
static GpioOps g_gpioOps = { .setOutputValue = setOutputValue, .getInputValue =
		getInputValue, .setInterruptFunc = setInterruptFunc, };
static int setOutputValue(struct GpioOps *base, int value) {
	LOGE("setOutputValue");
	int fd;
	int ret;
	char cmdStr[128] = { 0 };
	char valueStr[3] = { 0 };
	sprintf(valueStr, "%u", value);
	pGpioServer gpioServer = (pGpioServer) base;
	sprintf(cmdStr, "%s/direction", gpioServer->gpioPath);
	fd = open(cmdStr, O_WRONLY);
	if (fd > 0) {
		ret = write(fd, SYSFS_GPIO_RST_DIR_OUT, strlen(SYSFS_GPIO_RST_DIR_OUT) + 1);
		if (ret < 0) {
			LOGE("fail to write %s", cmdStr);
			close(fd);
			goto user_root;
		}
	}else {
		goto user_root;
	}
	bzero(cmdStr, sizeof(cmdStr));
	sprintf(cmdStr, "%s/value", gpioServer->gpioPath);
	fd = open(cmdStr, O_WRONLY);
	if (fd < 0) {
		LOGE("fail to open %s", cmdStr);
		close(fd);
		goto user_root;
	}
	ret = write(fd, valueStr, strlen(valueStr) + 1);
	if (ret < 0) {
		LOGE("fail to write %s", cmdStr);
		close(fd);
		goto user_root;
	}
	goto succeed;
user_root:
	bzero(cmdStr,sizeof(cmdStr));
	sprintf(cmdStr,"echo out > %s/direction",gpioServer->gpioPath);
	LOGE("cmdStr:%s ",cmdStr);
#if USER_BINDER == 1
	gpioServer->binderClient->runScript(gpioServer->binderClient,cmdStr);
#else
	gpioServer->netClient->runScript(gpioServer->netClient,cmdStr);
#endif
	bzero(cmdStr,sizeof(cmdStr));
	sprintf(cmdStr,"echo %d > %s/value",value,gpioServer->gpioPath);
	LOGE("cmdStr:%s",cmdStr);
#if USER_BINDER == 1
	gpioServer->binderClient->runScript(gpioServer->binderClient,cmdStr);
#else
	gpioServer->netClient->runScript(gpioServer->netClient,cmdStr);
#endif
succeed:
	return 0;
	fail0: return -1;
}
static int getInputValue(struct GpioOps *base) {
	char path[64] = { 0 };
	char value_str[3];
	int fd, ret;
	pGpioServer gpioServer = (pGpioServer) base;
	sprintf(path, "%s/direction", gpioServer->gpioPath);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		LOGE("fail to open %s", path);
		goto fail0;
	}
	ret = write(fd, SYSFS_GPIO_RST_DIR_IN, strlen(SYSFS_GPIO_RST_DIR_OUT) + 1);
	if (ret < 0) {
		LOGE("fail to write %s", path);
		goto fail1;
	}
	close(fd);

	snprintf(path, sizeof(path), "%s/value", gpioServer->gpioPath);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		LOGE("Failed to open gpio value for reading!\n");
		goto fail0;
	}
	if (read(fd, value_str, 3) < 0) {
		LOGE("Failed to read value!\n");
		goto fail1;
	}
	close(fd);
	return (atoi(value_str));
	fail1: close(fd);
	fail0: return -1;
}

static void *gpioloopHandle(void *arg) {
	pGpioServer gpioServer = (pGpioServer) (*(pGpioServer *) arg);
	if (gpioServer == NULL)
		goto exit;
	GpioPinState gpioState;
	LOGE("gpioloopHandle!");
	INTERRUPT_MODE interruptMode = gpioServer->interruptMode;
	int old_state = 1;
	bzero(&gpioState, sizeof(gpioState));
	gpioState.interruptArg = gpioServer->interruptArg;
	gpioState.pin = gpioServer->gpioPin;
	while (gpioServer->interruptThreadId->check(gpioServer->interruptThreadId)) {
		usleep(100 * 1000);
		gpioState.state = getInputValue((pGpioOps) gpioServer);
		if(gpioState.state == -1)
			continue;
		if (gpioState.state != old_state) {
			if (interruptMode == RISING && gpioState.state == 1) {
				gpioServer->interruptFunc(&gpioState);
			} else if (interruptMode == FALLING && gpioState.state == 0) {
				gpioServer->interruptFunc(&gpioState);
			} else if (interruptMode == BOTH) {
				gpioServer->interruptFunc(&gpioState);
			} else {

			}
		}
		old_state = gpioState.state;

	}

	exit: return NULL;
}
static void *gpioInterruptHandle(void *arg) {
#define EVENT_NUMS  2
	pGpioServer gpioServer = (pGpioServer) (*(pGpioServer *) arg);
	GpioPinState gpioState;
	int getGpioValueFd;
	char gpioValuePath[64] = { 0 };
	int ret;
	char buff[10];
	int epfd, nfds;
	struct epoll_event ev, events[EVENT_NUMS];
	sprintf(gpioValuePath, "%s/value", gpioServer->gpioPath);
	LOGE("gpioInterruptHandle!");

	epfd = epoll_create(EVENT_NUMS);
	getGpioValueFd = open(gpioValuePath, O_RDONLY); //fd 即为open /sys/class/gpio/gpioN/value返回的句柄
	if (getGpioValueFd < 0) {
		LOGE("fail to open %s", gpioValuePath);
		return NULL;
	}
	ev.data.fd = getGpioValueFd;
	//设置要处理的事件类型

	ev.events = EPOLLPRI; //EPOLLPRI;// |EPOLLET|
	epoll_ctl(epfd, EPOLL_CTL_ADD, getGpioValueFd, &ev);

	gpioServer->closeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	bzero(&ev, sizeof(ev));
	ev.data.fd = gpioServer->closeFd;
	ev.events = EPOLLIN | EPOLLPRI;
	epoll_ctl(epfd, EPOLL_CTL_ADD, gpioServer->closeFd, &ev);
//
	ret = read(getGpioValueFd, buff, 10);
	if (ret == -1)
		LOGE("fail to read\n");
	while (gpioServer->interruptThreadId->check(gpioServer->interruptThreadId)) {
		nfds = epoll_wait(epfd, events, EVENT_NUMS, -1);
		int i;
		for (i = 0; i < nfds; ++i) {
			if (events[i].data.fd == getGpioValueFd) {
				ret = readgpioStateFromFd(getGpioValueFd);
				if(ret < 0) continue;
				gpioState.pin = gpioServer->gpioPin;
				gpioState.state = ret;
				gpioState.interruptArg = gpioServer->interruptArg;
				gpioServer->interruptFunc(&gpioState);

				bzero(&gpioState, sizeof(gpioState));
			} else if (events[i].data.fd == gpioServer->closeFd) {
				LOGW(
						"[pin:%d]Receive the thread exit notification!", gpioServer->gpioPin);
				goto exit;
			}
		}
	}
	exit:
	LOGW("[pin:%d]gpioInterruptHandle EXIT", gpioServer->gpioPin);
	close(getGpioValueFd);
	return NULL;
}
static void *gpioInterruptHandleBypoll(void *arg) {
#define EVENT_NUMS  2
	pGpioServer gpioServer = (pGpioServer) (*(pGpioServer *) arg);
	GpioPinState gpioState;
	int getGpioValueFd;
	char gpioValuePath[64] = { 0 };
	char value_str[3] = {0};
	int ret;
	char buff[10];
	int epfd, nfds;
	//struct epoll_event ev, events[EVENT_NUMS];
	struct pollfd fds[2];

	sprintf(gpioValuePath, "%s/value", gpioServer->gpioPath);
	LOGE("gpioInterruptHandle by poll!");

	getGpioValueFd = open(gpioValuePath, O_RDONLY); //fd 即为open /sys/class/gpio/gpioN/value返回的句柄
	if (getGpioValueFd < 0) {
		LOGE("fail to open %s", gpioValuePath);
		return NULL;
	}
	fds[0].fd = getGpioValueFd;
	fds[0].events = POLLPRI;

	gpioServer->closeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	fds[1].fd = gpioServer->closeFd ;
	fds[1].events  = POLLIN|POLLPRI;
//
	ret = read(getGpioValueFd, buff, 10);
	if (ret == -1)
		LOGE("fail to read\n");
	while (gpioServer->interruptThreadId->check(gpioServer->interruptThreadId)) {
		LOGE("start poll\n");
		ret = poll(fds, 1, -1);
		if(ret < 0)
			continue;
		ret = readgpioStateFromFd(getGpioValueFd);
		if(ret < 0)
		{
			 continue;
		}
		gpioState.state = ret;
		gpioState.pin = gpioServer->gpioPin;
		gpioState.interruptArg = gpioServer->interruptArg;
		LOGE("trigger gpio:%d", gpioServer->gpioPin);
		gpioServer->interruptFunc(&gpioState);
		LOGE("do someing!gpio:%d\n", gpioServer->gpioPin);
	}
	return NULL;
}
static int readgpioStateFromFd(int fd)
{
	char value_str[3] = {0};
	int ret;
	ret = lseek(fd, 0, SEEK_SET);
	if (ret < 0)
	{
		return ret;
	}
	if (read(fd, value_str, sizeof(value_str)/sizeof(value_str[0])) < 0) {
		LOGE("Failed to read value!\n");
		return -1;
	}
	return (atoi(value_str));
}
static int setInterruptFunc(struct GpioOps *base, T_InterruptFunc callBackfunc,
		void *interruptArg, INTERRUPT_MODE mode) {
	int ret;
	pGpioServer gpioServer = (pGpioServer) base;
	gpioServer->interruptFunc = callBackfunc;

	ret = gpio_edge(gpioServer->gpioPin, mode);
	if (ret == 0) {
		//此处只是将指针存储到线程管理中
		gpioServer->interruptThreadId = pthread_register(gpioInterruptHandle,
				&base, sizeof(pGpioServer), NULL);

		if (gpioServer->interruptThreadId == NULL) {
			gpioServer->interruptFunc = NULL;
			LOGE(" fail to get gpioServer->interruptThreadId!");
			return -1;
		}
	} else {
		gpioServer->interruptThreadId = pthread_register(gpioloopHandle, &base,
				sizeof(pGpioServer), NULL);
		if (gpioServer->interruptThreadId == NULL) {
			gpioServer->interruptFunc = NULL;
			LOGE(" fail to get gpioServer->interruptThreadId!");
			return -1;
		}
	}
	gpioServer->interruptMode = mode;
	gpioServer->interruptArg = interruptArg;
	ret = gpioServer->interruptThreadId->start(gpioServer->interruptThreadId);
	if (ret != 0)
		return -1;

	return 0;
}
pGpioOps gpio_getServer( int gpio) {
	char cmdStr[128] = { 0 };
	char goioStr[6] = { 0 };
	int ret = -1;
	if(gpio <= 0){
		LOGE("fail to gpio_getServer gpio:%d",gpio);
		return NULL;
	}
	sprintf(goioStr, "%u", gpio);
	int gpiofd;
	int exportFd;
	pGpioServer gpioServer = (pGpioServer) malloc(sizeof(GpioServer));
	if (gpioServer == NULL) {
		goto fail0;
	}
	bzero(gpioServer, sizeof(GpioServer));
#if USER_BINDER == 1
	gpioServer->binderClient = binder_getServer();
	if (gpioServer->binderClient == NULL) {
		LOGE("fail to crate binderClient!\n");
		goto fail1;
	}
#else
	gpioServer->netClient = createNativeNetServer();
	if(gpioServer->netClient == NULL)
	{
		LOGE("fail to crate netClient!\n");
		goto fail1;
	}
	LOGE("gpioServer->netClient !");
#endif

	exportFd = open(SYSFS_GPIO_EXPORT, O_WRONLY);
	if (exportFd < 0) {
		LOGE("fail to open %s!", SYSFS_GPIO_EXPORT);
		goto fail2;
	}
	//尝试先打开判断文件是否存在
	bzero(cmdStr, sizeof(cmdStr));
	sprintf(cmdStr, "/sys/class/gpio/gpio%d/value", gpio);
	gpiofd = open(cmdStr, O_RDWR);
	if (gpiofd > 0) {
		LOGD(" %s is exist!", cmdStr);
		goto succeed;
	}

	ret = write(exportFd, goioStr, strlen(goioStr) + 1);
	if (ret < 0) {
		bzero(cmdStr, sizeof(cmdStr));
		//用root权限打开
		//# echo 34 > /sys/class/gpio/export
		sprintf(cmdStr, "echo %d %s", gpio, SYSFS_GPIO_EXPORT);


#if USER_BINDER == 1
		ret = gpioServer->binderClient->runScript(gpioServer->binderClient,
				cmdStr);
#else
		ret = gpioServer->netClient->runScript(gpioServer->netClient,
					  cmdStr);
#endif
		if (ret < 0) {
			LOGE("fail to write %s!", SYSFS_GPIO_EXPORT);
			goto fail3;
		}

	}

	//尝试再次打开
	bzero(cmdStr, sizeof(cmdStr));
	sprintf(cmdStr, "/sys/class/gpio/gpio%d/value", gpio);
	gpiofd = open(cmdStr, O_RDWR);
	if (gpiofd <= 0) {
		LOGE("fail to open %s!", cmdStr);
		goto fail3;
	}

	succeed: close(exportFd);
	close(gpiofd);
	bzero(gpioServer->gpioPath, sizeof(gpioServer->gpioPath));
	sprintf(gpioServer->gpioPath, "/sys/class/gpio/gpio%d", gpio);
	gpioServer->ops = g_gpioOps;
	gpioServer->gpioPin = gpio;
	return (pGpioOps) (&gpioServer->ops);

	fail3: close(exportFd);
#if USER_BINDER == 1
	fail2: binder_releaseServer(&gpioServer->binderClient);
#else
	fail2: destroyNativeNetServer(&gpioServer->netClient);
#endif

	fail1: free(gpioServer);
	fail0: return NULL;
}

// none表示引脚为输入，不是中断引脚
// rising表示引脚为中断输入，上升沿触发
// falling表示引脚为中断输入，下降沿触发
// both表示引脚为中断输入，边沿触发
// 0-->none, 1-->rising, 2-->falling, 3-->both
static int gpio_edge(int pin, INTERRUPT_MODE edge) {
	const char dir_str[] = "none\0rising\0falling\0both";
	char ptr;
	char path[64];
	int fd;
	switch (edge) {
	case NONE:
		ptr = 0;
		break;
	case RISING:
		ptr = 5;
		break;
	case FALLING:
		ptr = 12;
		break;
	case BOTH:
		ptr = 20;
		break;
	default:
		ptr = 0;
		break;
	}
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", pin);
	fd = open(path, O_WRONLY | O_CREAT);
	if (fd < 0) {
		LOGE("Failed to open gpio edge for writing!\n");
		close(fd);
		return -1;
	}
	if (write(fd, &dir_str[ptr], strlen(&dir_str[ptr])) < 0) {
		LOGE("Failed to set edge!\n");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

void gpio_releaseServer(pGpioOps *ops) {
	pGpioServer gpioServer = (pGpioServer) (*ops);
	int fd, ret;
	char cmdStr[128] = { 0 };
	char goioStr[6] = { 0 };
	if (gpioServer == NULL)
		return;
	if (gpioServer->closeFd > 0) {
		eventfd_write(gpioServer->closeFd, 1);
		LOGW("[pin:%d]send close sig", gpioServer->gpioPin);
	}
	if (gpioServer->interruptThreadId) {
		pthread_destroy(&gpioServer->interruptThreadId);
	}
	sprintf(goioStr, "%u", gpioServer->gpioPin);
	fd = open(SYSFS_GPIO_UNEXPORT, O_WRONLY);
	if (fd < 0) {
		LOGE("fail to open %s!", SYSFS_GPIO_UNEXPORT);
		goto fail0;
	}
	ret = write(fd, goioStr, strlen(goioStr) + 1);
	if (ret < 0) {

		bzero(cmdStr, sizeof(cmdStr));
		sprintf(cmdStr, "echo %d %s", gpioServer->gpioPin, SYSFS_GPIO_UNEXPORT);
#if USER_BINDER == 1
		ret = gpioServer->binderClient->runScript(gpioServer->binderClient,
				cmdStr);
#else
		ret = gpioServer->netClient->runScript(gpioServer->netClient,
						cmdStr);
#endif
		if (ret < 0) {
			LOGE("fail to write %s!", SYSFS_GPIO_UNEXPORT);
			goto fail1;
		}
	}
	close(fd);
	free(gpioServer);
	*ops = NULL;
	return;
#if USER_BINDER == 1
	fail1: binder_releaseServer(&gpioServer->binderClient);
#else
	fail1:destroyNativeNetServer(&gpioServer->netClient);
#endif

	fail0: free(gpioServer);
	*ops = NULL;
	return;
}

