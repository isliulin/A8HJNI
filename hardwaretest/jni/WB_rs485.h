#ifndef WB_RS485_H__
#define WB_RS485_H__

typedef struct Rs485Ops {
	int (*sendMsg)(struct Rs485Ops *,char *,int );
	int (*recvMsg)(struct Rs485Ops *,int ,char *,int );
}Rs485Ops,*pRs485Ops;
pRs485Ops createRs485Server(int nBaudRate, int nDataBits, int nStopBits, int nParity);
void destroyRs485Server(pRs485Ops *server);



#endif
