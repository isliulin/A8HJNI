#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "common/debugLog.h"
#include "serial/serialServer.h"
#include "common/Utils.h"
#include "temperature/tean_temperatureDetection.h"

typedef struct TemperatureDetectionServer{
	TemperatureDetectionOps ops;
	pSerialOps serialServer;
	char serialNumber[4];
	float parameter;

}TemperatureDetectionServer,*pTemperatureDetectionServer;

static int getGlobalTemperature(struct TemperatureDetectionOps * ops, float *data,int len);
static int getSpecialTemperature(struct TemperatureDetectionOps * ops,float  *centre,float *max,float *mini);
static int setBaudRate(struct TemperatureDetectionOps * ops,int rate);
static int setTemperatureCompensation(struct TemperatureDetectionOps * ops,float parameter);
static int compensation(struct TemperatureDetectionOps * ops,float *s,int slen,float *d,int dlen);

static TemperatureDetectionOps ops = {
		.getGlobalTemperature = getGlobalTemperature,
		.getSpecialTemperature = getSpecialTemperature,
		.setBaudRate = setBaudRate,
		.setTemperatureCompensation = setTemperatureCompensation,
};
static int compensation(struct TemperatureDetectionOps * ops,float *s,int slen,float *d,int dlen){
	pTemperatureDetectionServer server  = (pTemperatureDetectionServer)ops;
	if(server == NULL || server->serialServer == NULL){
		goto fail0;
	}
	int i;
	for(i = 0;i<slen&&i<dlen;i++){
		d[i] = s[i] + ((0.965 - server->parameter)*30);
	}
	return 0;
	fail0:
		return -1;
}
static int setBaudRate(struct TemperatureDetectionOps * ops,int rate){
	return -1;
}
static int setTemperatureCompensation(struct TemperatureDetectionOps * ops,float parameter){

	pTemperatureDetectionServer server  = (pTemperatureDetectionServer)ops;

	if(server == NULL || server->serialServer == NULL){

		goto fail0;
	}
	if(parameter < 0.93 ||parameter >1.0){
		goto fail0;
	}
	server->parameter = parameter;
	return 0;
	fail0:
		LOGE("fail to %s\n",__func__);
		return -1;

}
static int charDataTofloatData(char *cdata,int clen,float *fdata,int flen){
	if(cdata == NULL || fdata == NULL){
		goto fail0;
	}
	int i,j,temp;
	for(i = 0 ,j = 0; i < clen&&j<flen ; i += 2,j++){

		temp = cdata[i] <<8 | cdata[i+1];
		fdata[j] = (temp) / 10.0;
	}
	return 0;
	fail0:
		return -1;
}

static int getGlobalTemperature(struct TemperatureDetectionOps * ops, float *data,int len){
		pTemperatureDetectionServer server  = (pTemperatureDetectionServer)ops;
		if(server == NULL || server->serialServer == NULL){
			LOGE("fail to len is null!\n");
			goto fail0;
		}
		if(len >1024){
			LOGE("fail to len is too len!\n");
			goto fail0;
		}
		char cmd[32] = "VCMD=DAT";
		char buf[1024*4] = {0};
		int ret,rlen;
		ret  = server->serialServer->write(server->serialServer,cmd,strlen(cmd));
		if(ret < 0){
			LOGE("fail to write!\n");
			goto fail0;
		}

		rlen = server->serialServer->read(server->serialServer,buf,2070,3000);
		if(rlen != 2070){
			LOGE("fail to read %d! ",rlen);
			goto fail0;
		}
		//偷下懒,只对比前后两个字节

		if(buf[0] != 0x53 || buf[1] != 0x54 || buf[2068] != 0x0A || buf[2069] != 0x0D){
			goto fail0;
		}
		ret  = charDataTofloatData(&buf[5],2048,data,len);
		if(ret != 0){
			goto fail0;
		}
		//温度补偿
		ret  = compensation(server,data,len,data,len);
		if(ret != 0){
				goto fail0;
		}
		return 0;
		fail0:
			LOGE("fail to %s\n",__func__);
		return -1;
}
static int hextoint(char a[]){
	int len,t,sum=0,i;
	len=strlen(a);
	if( (strncmp(a,"0x",2) == 0)|| (strncmp(a,"0X",2) == 0)  ){
		memset(a,0,2);
		memcpy(a,a+2,len-2);
		memset(a+len-2,0,2);
	}
	len=strlen(a);
	for(i=0;i<len;i++){
		if((a[i]>='0'&&a[i]<='9')||(a[i]>='a'&&a[i]<='f')||(a[i]>='A'&&a[i]<='F')){
			if(a[i]>='0'&&a[i]<='9')
				t=a[i]-'0';
			else if(a[i]>='a'&&a[i]<='f')
				t=a[i]-'a'+10;//十六进制a转化为十进制是10,以此类推到f
			else
				t=a[i]-'A'+10;
			sum=sum*16+t;
		}
		else{
			break;
		}
	}
	return sum;
}

static int getSpecialTemperature(struct TemperatureDetectionOps * ops,float  *centre,float *max,float *min){

	pTemperatureDetectionServer server  = (pTemperatureDetectionServer)ops;
	if(server == NULL || server->serialServer == NULL){
		LOGE("fail to server == NULL || server->serialServer == NULL!\n");
		goto fail0;
	}
	char cmd[] = "VCMD=TMP";
	char buf[1024*4] = {0};
	int ret;
	ret  = server->serialServer->write(server->serialServer,cmd,sizeof(cmd));
	if(ret < 0){
		LOGE("fail to write!\n");
		goto fail0;
	}
	ret = server->serialServer->read(server->serialServer,buf,73,2000);
	if(ret < 0){
		LOGE("fail to read!\n");
		goto fail0;
	}

	if(ret != 73){
		LOGE("fail to len is wrong %d!\n",ret);
		getUtilsOps()->printData(buf,ret);
		goto fail0;
	}

	if(strstr(buf,"over") == NULL){
		goto fail0;
	}
	char centreS[32] = {0};
	char maxS[32] = {0};
	char minS[32] = {0};
	char ExtremumS[32] = {0};
	char *index = NULL;
	index= strstr(buf,"AvgTmp=");
	if(index)
		strncpy(centreS,index+strlen("AvgTmp="),6);

	index= strstr(buf,"Max=");
	if(index)
		strncpy(maxS,index+strlen("Max="),6);

	index= strstr(buf,"Min=");
	if(index)
		strncpy(minS,index+strlen("Min="),6);

	index= strstr(buf,"Extremum=");
	if(index)
		strncpy(ExtremumS,index+strlen("Extremum="),6);
	*centre = hextoint(centreS)/10.0;
	*max = hextoint(maxS)/10.0;
	*min = hextoint(minS)/10.0;

	//温度补偿
	compensation(server,centre,1,centre,1);
	compensation(server,max,1,max,1);
	compensation(server,min,1,min,1);
	return 0;
	fail0:
		LOGE("fail serial read or write!\n");
	return -1;
}

static int checkmoduleState(pTemperatureDetectionServer server ){
	int ret  = 0;
	char writeCmd[128] = "VCMD=SAT";
	char readBuf[1024] = {0};
	ret = server->serialServer->write(server->serialServer,writeCmd,strlen(writeCmd));
	if(ret < 0){
		goto fail0;
	}
	ret = server->serialServer->read(server->serialServer,readBuf,22,1000);
	if(ret < 0){
		goto fail0;
	}

	if(strstr(readBuf,"serial") == 	NULL){
		goto fail0;
	}

	return 0;
	fail0:
		return -1;
}
pTemperatureDetectionOps createTeanTemperatureDetectionServer(const char *uartPath){
	int ret;


	pTemperatureDetectionServer server = (pTemperatureDetectionServer)malloc(sizeof(TemperatureDetectionServer));
	if(NULL == server){
		 goto fail0;
	}
	memset(server,0,sizeof(TemperatureDetectionServer));
	//获取一个串口对象
	server->serialServer = createSerialServer(uartPath,115200, 8, 1, 'n');

	if(NULL == server->serialServer){
		goto fail1;
	}
	server->ops = ops;
	//0.93 0.94 0.95 0.96  0.965  0.97 0.98 0.99 1.0
	// 1.0-0.965 = 0.035*30
	server->parameter = 0.965;//取个中间值
	//检测模块状态
	ret = checkmoduleState(server);
	if(ret < 0){
		goto fail1;
	}

	return (pTemperatureDetectionOps)server;
	fail1:
		destroySerialServer(&(server->serialServer) );
	fail0:
		return NULL;

}
void destroyTeanTemperatureDetectionServer(pTemperatureDetectionOps *server){
	if(server == NULL || *server == NULL){
		return ;
	}
	pTemperatureDetectionServer pthis = (pTemperatureDetectionServer)*server;
	destroySerialServer(&pthis->serialServer);
	free(pthis);
	*server = NULL;
}
