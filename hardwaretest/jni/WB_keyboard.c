#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <linux/input.h>
#include <termios.h>
#include <stdint.h>
#include <netinet/in.h>
#include <linux/input.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <common/debugLog.h>
#include "WB_keyboard.h"

#define MAX_EVENT_NUMBER 33

typedef struct WB_KeyBoardServer {
	WB_KeyBoardOps ops;
	KeyEventUpFunc eventUpFunc;
	char devPath[32];
	int epoolfd;
	int epoolMonitorFds[MAX_EVENT_NUMBER];
	pthread_t readKeyThread;
	int wakeFds[2]; //[0]:read [1]:write
} WB_KeyBoardServer, *pWB_KeyBoardServer;

static int setKeyEventUpFunc(pWB_KeyBoardOps ops, KeyEventUpFunc eventUpFunc);
static void addEventFdToEpool(pWB_KeyBoardServer server);
static int addFdToEpoolMonitorFds(int fd, int *fds);
static void closeEpoolMonitorFds(int *fds);
static void * readEventThreadFunc(void *arg);
static WB_KeyBoardOps ops = { .setKeyEventUpFunc = setKeyEventUpFunc, };
static int setKeyEventUpFunc(pWB_KeyBoardOps ops, KeyEventUpFunc eventUpFunc) {
	pWB_KeyBoardServer pthis = (pWB_KeyBoardServer) ops;
	if (pthis == NULL)
		return -1;
	pthis->eventUpFunc = eventUpFunc;
	return 0;
}
static int addFdToEpoolMonitorFds(int fd, int *fds) {
	int i;
	for (i = 0; i < MAX_EVENT_NUMBER; i++) {
		if (fds[i] <= 0) {
			fds[i] = fd;
			return 0;
		}
	}
	return -1;
}
static void closeEpoolMonitorFds(int *fds) {
	int i;
	for (i = 0; i < MAX_EVENT_NUMBER; i++) {
		if (fds[i] > 0) {
			close(fds[i]);
			fds[i] = 0;
		}
	}
}
static void addEventFdToEpool(pWB_KeyBoardServer server) {
	struct epoll_event ev;
	int ret;
	int i, j = 0, fd;
	int fds[MAX_EVENT_NUMBER - 1] = { 0 };
	char evnetDev[32] = { 0 };

	for (i = 0; i < sizeof(fds) / sizeof(fds[0]); i++) {
		sprintf(evnetDev, "%s%d", server->devPath, i);
	//	LOGD("open %s", evnetDev);
		if ((fd = open(evnetDev, O_RDONLY, 0)) >= 0) {
		//	LOGD("open %s succeed fd:%d", evnetDev, fd);
			fcntl(fd, F_SETFL, O_NONBLOCK);
			bzero(&ev, sizeof(ev));
			ev.data.fd = fd;
			ev.events = EPOLLIN;
			ret = epoll_ctl(server->epoolfd, EPOLL_CTL_ADD, fd, &ev);
			if (ret == 0) {
				ret = addFdToEpoolMonitorFds(fd, server->epoolMonitorFds);
				if (ret < 0) {
					LOGE("fail to addEventFdToEpool fail fd:%d ", fd);
				}
			}
		} else {
			LOGD(" fail to open%s%d ", server->devPath, i);
		}
	}
}
static void * readEventThreadFunc(void *arg) {
	int pollResult;
	int i, j;
	int readn;
	struct epoll_event epoolEvents[MAX_EVENT_NUMBER] = { 0 };
	struct input_event keyEvent[64];
	pWB_KeyBoardServer server = arg;
	char wakeCmd = -1;
	if (server == NULL)
		goto fail0;
	LOGD("readEventThreadFunc start");
	for (;;) {
		pollResult = epoll_wait(server->epoolfd, epoolEvents, MAX_EVENT_NUMBER,
				-1);
		for (i = 0; i < pollResult; i++) {
			if (epoolEvents[i].data.fd == server->wakeFds[0]) {
				wakeCmd = -1;
				int n;
				do {
					n = read(server->wakeFds[0], &wakeCmd, sizeof(wakeCmd));
				} while (n == -1 && errno == EINTR);
				if (wakeCmd == 0) //表示收到了exit指令,将退出此线程
					goto fail0;
			}
			do {
				readn = read(epoolEvents[i].data.fd, keyEvent,
						sizeof(keyEvent));
			} while (readn == -1 && errno == EINTR);
			if (readn <= 0)
				continue;
			for (j = 0; j < readn / sizeof(struct input_event); j++) {
				if (keyEvent[j].type == EV_KEY) {
					if (server->eventUpFunc) {
						server->eventUpFunc(keyEvent[j].code,
								keyEvent[j].value);
					}
				}
			}
		}
	}
	fail0: return NULL;
}
pWB_KeyBoardOps createKeyBoardServer(const char *devPath) {
	int result;
	pWB_KeyBoardServer server = malloc(sizeof(WB_KeyBoardServer));
	if (server == NULL)
		goto fail0;
	bzero(server, sizeof(WB_KeyBoardServer));
	server->epoolfd = epoll_create(MAX_EVENT_NUMBER);
	if (server->epoolfd < 0)
		goto fail0;

	result = pipe(server->wakeFds);
	if (result != 0) {
		goto fail1;
	}
	//设置管道成非阻塞模式
	result = fcntl(server->wakeFds[0], F_SETFL, O_NONBLOCK);
	if (result != 0) {
		goto fail2;
	}
	result = fcntl(server->wakeFds[1], F_SETFL, O_NONBLOCK);
	if (result != 0) {
		goto fail2;
	}

	struct epoll_event ev;
	bzero(&ev, sizeof(ev));
	ev.data.fd = server->wakeFds[0];
	ev.events = EPOLLIN;

	result = epoll_ctl(server->epoolfd, EPOLL_CTL_ADD, server->wakeFds[0], &ev);
	if (result == 0) {
		result = addFdToEpoolMonitorFds(server->wakeFds[0],
				server->epoolMonitorFds);
		if (result < 0) {
			LOGE("fail to addFdToEpoolMonitorFds fd:%d", server->wakeFds[0]);
		}
	}
	strcpy(server->devPath, devPath);
	addEventFdToEpool(server);
	if (pthread_create(&server->readKeyThread, 0, readEventThreadFunc,
			(void*) server) != 0) {
		goto fail2;
	}
	server->ops = ops;
	return (pWB_KeyBoardOps) server;
	fail2: closeEpoolMonitorFds(server->epoolMonitorFds);
	fail1: close(server->epoolfd);
	fail0: return NULL;
}
void destroyKeyBoardServer(pWB_KeyBoardOps *server) {
	if (server == NULL || *server == NULL)
		return;
	pWB_KeyBoardServer pthis = (pWB_KeyBoardServer) *server;
	char exitCmd = 0;
	int nWrite;
	do {
		nWrite = write(pthis->wakeFds[1], &exitCmd, sizeof(exitCmd));
	} while (nWrite == -1 && errno == EINTR);

	pthread_join(pthis->readKeyThread, NULL);
	closeEpoolMonitorFds(pthis->epoolMonitorFds);
	free(pthis);
	*server = NULL;
}

