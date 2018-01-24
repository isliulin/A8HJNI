#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <setjmp.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "taskManage/threadManage.h"
#include "common/bufferManage.h"
#include "common/debugLog.h"
#include "common/Utils.h"
#include "communicationServer.h"

typedef struct CommunicationServer {
	CommunicationOps ops;
	int writeFd;
	int readFd;
	pThreadOps recvThreadId; //接收数据的线程
	int stopRecvThreadFd; //用于停止线程的eventfd
	pThreadOps parseThreadId; //处理数据的线程
	pBufferOps bufferOps; //缓存的BUF
	CommunicationCallBackFunc updataFunc;
} CommunicationServer, *pCommunicationServer;

static int _send(struct CommunicationOps *ops, void * data, unsigned int len);
static int _recv(struct CommunicationOps * ops, void *data, unsigned int len);
static int setRecvCallbackFunc(struct CommunicationOps *ops,
		CommunicationCallBackFunc callback);
static CommunicationOps ops = {
		.setRecvCallbackFunc = setRecvCallbackFunc,
		.send = _send,
		.recv = _recv,
};

static void *recvThreadFunc(void *arg) {

#define EVENT_NUMS  2
	int epfd, nfds;
	int readLen = 0;
	struct epoll_event ev, events[EVENT_NUMS];
	char readBuff[1024] = { 0 };
	pCommunicationServer server = *((pCommunicationServer*) arg);
	LOGD("udpRecvThreadFunc");
	if (server == NULL) {
		goto exit;
	}
	server->stopRecvThreadFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (server->stopRecvThreadFd < 0) {
		goto exit;
	}
	epfd = epoll_create(EVENT_NUMS);
	bzero(&ev, sizeof(ev));
	ev.data.fd = server->readFd;
	ev.events = EPOLLET | EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server->readFd, &ev);

	bzero(&ev, sizeof(ev));
	ev.data.fd = server->stopRecvThreadFd;
	ev.events = EPOLLIN | EPOLLPRI;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server->stopRecvThreadFd, &ev);

	while (server->recvThreadId->check(server->recvThreadId)) {
		nfds = epoll_wait(epfd, events, EVENT_NUMS, -1);
		int i;
		for (i = 0; i < nfds; ++i) {
			if (events[i].data.fd == server->readFd) {
				//*读取数据 并加入缓存队列
				int sockaddrLen = sizeof(struct sockaddr);
				bzero(readBuff, sizeof(readBuff));
				readLen = _recv((pCommunicationOps)server,readBuff,sizeof(readBuff));
				//加入队列
				if (readLen > 0) {
					server->bufferOps->push(server->bufferOps, readBuff,
							readLen);
				} else {
					LOGE("fail to _read serialDevFd");
				}
			} else if (events[i].data.fd == server->stopRecvThreadFd) {
				goto exit;
			}
		}
	}
	exit:
	LOGE("udpRecvThreadFunc exit!");
	return NULL;
}
static void * parseThreadFunc(void* arg) {
	pCommunicationServer server = *((pCommunicationServer*) arg);
	if (server == NULL) {
		goto exit;
	}
	int ret = 0;
	char recvBuf[2046] = { 0 };
	char validBuf[2046] = { 0 };
	int recvLen = 0;
	int pullLen = 0;
	int validLen;
	while (server->recvThreadId->check(server->recvThreadId)) {
		//读取队列数据
		//上报给用户
		ret = server->bufferOps->wait(server->bufferOps);
		switch (ret) {
		case TRI_DATA:
			do {
				bzero(recvBuf, sizeof(recvBuf));
				bzero(validBuf, sizeof(validBuf));
				pullLen = server->bufferOps->pull(server->bufferOps, recvBuf,
						sizeof(recvBuf));
				if (pullLen > 0) {
					if (server->updataFunc) {

						recvLen = server->updataFunc(recvBuf, pullLen);
						if (recvLen > 0) {
							ret = server->bufferOps->deleteLeft(
									server->bufferOps, recvLen);
							if (ret < 0) {
								LOGW("fail to deleteLeft!");
							}
						}
					}
				}
			} while (pullLen >= sizeof(recvBuf));
			break;
		case TRI_EXIT:
			goto exit;
			break;
		default:
			break;
		}
	}
	exit:
	LOGE("UdpParseThreadFunc exit!");
	return NULL;

}
static int setRecvCallbackFunc(struct CommunicationOps *ops,
		CommunicationCallBackFunc callback) {
	pCommunicationServer server = (pCommunicationServer)ops;
	if (server == NULL || server->updataFunc != NULL) {
		goto fail0;
	}
	server->updataFunc = callback;
	server->bufferOps = createBufferServer(512);
	if (server->bufferOps == NULL) {
		LOGE("fail to createBufferServer");
		goto fail0;
	}
	server->parseThreadId = pthread_register(parseThreadFunc, &server,
			sizeof(CommunicationServer), NULL);
	if (server->parseThreadId == NULL) {
		LOGE("fail to pthread_register parseThreadId");
		goto fail1;
	}

	server->recvThreadId = pthread_register(recvThreadFunc, &server,
			sizeof(CommunicationServer), NULL);
	if (server->recvThreadId == NULL) {
		LOGE("fail to pthread_register parseThreadId");
		goto fail2;
	}
	server->parseThreadId->start(server->parseThreadId);
	server->recvThreadId->start(server->recvThreadId);
	return 0;
	fail2: pthread_destroy(&server->parseThreadId);
	fail1: destroyBufferServer(&server->bufferOps);
	fail0: return -1;
}
static int _send(struct CommunicationOps *ops, void * data, unsigned int len) {

	pCommunicationServer server = (pCommunicationServer) ops;
	int				writen;
	int				nSendTimes = 0;
	int				nSendLen = 0;
	if (server == NULL||server->writeFd <=0||data == NULL||len <=0) {
		return -1;
	}
	nSendTimes = 0;
	nSendLen = 0;
	while (nSendTimes++ < 5 && nSendLen < len) {
		writen = write(server->writeFd, data + nSendLen, len - nSendLen);
		if (writen <= 0) {
			break;
		}
		nSendLen += writen;
	}
	return nSendLen;
}
static int _recv(struct CommunicationOps * ops, void *data, unsigned int len) {
	pCommunicationServer server = (pCommunicationServer) ops;
	if (server == NULL||server->readFd <=0||data == NULL||len <=0) {
		return -1;
	}
	int total,readlen = 0;
	int ret;
	ioctl(server->readFd, FIONREAD, &total);
	if(total <=0 )
		return 0;
	total = total > len ? len : total;
	while (readlen < total) {
		ret = read(server->readFd, data + readlen, total - readlen);
		if (ret <= 0) {
			LOGE("fail to read ");
			break;
		}
		readlen += ret;
	}
	return readlen;
}
static int _setNoblock(int fd, int bNoBlock)
{
	if (fd < 0)
		return -1;
	if (ioctl(fd, FIONBIO, &bNoBlock) < 0)
		return -1;
	return 0;
}
pCommunicationOps createCommunicationServer(int writeFd,int readFd) {
	pCommunicationServer server = malloc(sizeof(CommunicationServer));
	if (server == NULL) {
		LOGE("fail to malloc pCommunicationServer");
		goto fail0;
	}
	bzero(server, sizeof(CommunicationServer));
	server->writeFd = writeFd;
	server->readFd = readFd;
	_setNoblock(writeFd,1);
	_setNoblock(readFd,1);
	server->ops = ops;
	return (pCommunicationOps)server;
	fail0: return NULL;
}
void destroyCommunicationServer(pCommunicationOps *server) {
	pCommunicationServer pthis = (pCommunicationServer)(*server);
	if(pthis == NULL)
		return ;
	if(pthis->recvThreadId)
	{
		eventfd_write(pthis->stopRecvThreadFd,1);
		pthis->recvThreadId->stop(pthis->recvThreadId);
		pthread_destroy(&pthis->recvThreadId);
	}
	if(pthis->parseThreadId)
	{
		pthis->bufferOps->exitWait(pthis->bufferOps);
		pthread_destroy(&pthis->parseThreadId);
	}
	if(pthis->bufferOps != NULL)
	{
			destroyBufferServer(&pthis->bufferOps);
	}
	if(pthis->readFd >0){
		close(pthis->readFd);
	}
	if(pthis->writeFd >0)
	{
		close(pthis->writeFd);
	}
	free(pthis);
	*server = NULL;
}


















