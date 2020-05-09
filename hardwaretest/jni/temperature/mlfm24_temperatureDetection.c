#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "common/debugLog.h"
#include "serial/serialServer.h"
#include "common/Utils.h"
#include "temperature/mlfm24_temperatureDetection.h"


typedef struct TemperatureDetectionServer{
	TemperatureDetectionOps ops;
	pSerialOps serialServer;
	char serialNumber[4];

}TemperatureDetectionServer,*pTemperatureDetectionServer;

static int getGlobalTemperature(struct TemperatureDetectionOps * ops, float *data,int len);
static int getSpecialTemperature(struct TemperatureDetectionOps * ops,float  *centre,float *max,float *mini);
static int setBaudRate(struct TemperatureDetectionOps * ops,int rate);
static int setTemperatureCompensation(struct TemperatureDetectionOps * ops,float parameter);


static  TemperatureDetectionOps ops = {
		.getGlobalTemperature = getGlobalTemperature,
		.getSpecialTemperature = getSpecialTemperature,
		.setBaudRate = setBaudRate,
		.setTemperatureCompensation = setTemperatureCompensation,
};
static int setBaudRate(struct TemperatureDetectionOps * ops,int rate){
	pTemperatureDetectionServer server  = (pTemperatureDetectionServer)ops;
	if(server == NULL || server->serialServer == NULL){

		goto fail0;
	}
	int i;
	int rateArray[] = {9600,19200,38400,115200,256000,512000,921600};

	char rateIndex;
	for( i = 0;i < sizeof(rateArray)/sizeof(rateArray[0]);i++){
		if(rate == rateArray[i]){
			rateIndex = i;
			break;
		}
	}
	if(i == sizeof(rateArray)/sizeof(rateArray[0])){
		goto fail0;
	}

	char cmd[] = {0xEE,0xB4,0x55,0xAA,0x00,rateIndex+1,0xFF,0xFC,0xFD,0xFF};
	for( i = 0;i < sizeof(rateArray)/sizeof(rateArray[0]);i++){
		char buf[1024*4] = {0};
		int ret,rlen;
		//先设置本机波特率,再设置从机波特率
		ret  = server->serialServer->setBaudRate(server->serialServer,rateArray[i]);
		if(ret < 0){
			LOGE("fail to setBaudRate %d!",rateArray[i]);
			goto fail0;
		}
		usleep(500*1000);
		ret  = server->serialServer->write(server->serialServer,cmd,sizeof(cmd));
		if(ret < 0){
			LOGE("fail to serialServer->write!");
			continue;
		}
		rlen = server->serialServer->read(server->serialServer,buf,6,2000);
		if(rlen != 6){
			LOGE("fail to serialServer->read!");
			continue;
		}
		ret  = (memcmp(cmd,buf,2)||memcmp(&cmd[sizeof(cmd) -4],&buf[rlen-4],4));
		if(ret != 0){
			LOGE("fail to memcmp!");
			continue;
		}else {
			break;
		}
	}
	return 0;
	fail0:
		return -1;
}
static int setTemperatureCompensation(struct TemperatureDetectionOps * ops,float parameter){

	pTemperatureDetectionServer server  = (pTemperatureDetectionServer)ops;

	if(server == NULL || server->serialServer == NULL){

		goto fail0;
	}
	if(parameter <= 0.93 ||parameter >=1.0){
		goto fail0;
	}
	char *floatTochar = (char *)&parameter;
	char cmd[] = {0xEE,0xB2,0x55,0xAA,floatTochar[0],floatTochar[1],floatTochar[2],floatTochar[3],0xFF,0xFC,0xFD,0xFF};
	char buf[1024*4] = {0};

	int ret,rlen;
	ret  = server->serialServer->write(server->serialServer,cmd,sizeof(cmd));
	if(ret < 0){
		LOGE("fail to write!");
		goto fail0;
	}
	rlen = server->serialServer->read(server->serialServer,buf,6,2000);
	if(rlen != 6){
		LOGE("fail to read!");
		goto fail0;
	}

	getUtilsOps()->printData(buf,rlen);
	ret  = (memcmp(cmd,buf,2)||memcmp(&cmd[sizeof(cmd)-4],&buf[rlen-4],4));
	if(ret != 0){
		LOGE("fail to memcmp!");
		goto fail0;
	}
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
		fdata[j] = (temp - 2731) / 10.0;
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
		char cmd[] = {0xEE ,0xE1,0x01,0x55,0xFF,0xFC,0xFD,0xFF};
		char buf[1024*4] = {0};
		int ret,rlen;
		ret  = server->serialServer->write(server->serialServer,cmd,sizeof(cmd));
		if(ret < 0){
			LOGE("fail to write!\n");
			goto fail0;
		}

		rlen = server->serialServer->read(server->serialServer,buf,2055,3000);
		if(rlen != 2055){
			LOGE("fail to read %d! ",rlen);
			goto fail0;
		}
		if(buf[0] != 0xe1){
			goto fail0;
		}
		ret  = charDataTofloatData(&buf[1],2048,data,len);
		if(ret != 0){
			goto fail0;
		}
		memcpy(server->serialNumber,&buf[2055-4],4);
		return 0;
		fail0:
			LOGE("fail to %s\n",__func__);
			return -1;
}
static int getSpecialTemperature(struct TemperatureDetectionOps * ops,float  *centre,float *max,float *mini){

	pTemperatureDetectionServer server  = (pTemperatureDetectionServer)ops;
	if(server == NULL || server->serialServer == NULL){
		LOGE("fail to server == NULL || server->serialServer == NULL!\n");
		goto fail0;
	}
	char cmd[] = {0xEE,0xB3,0xFF,0xFC,0xFD,0xFF};
	char buf[1024*4] = {0};
	int ret;
	ret  = server->serialServer->write(server->serialServer,cmd,sizeof(cmd));
	if(ret < 0){
		LOGE("fail to write!\n");
		goto fail0;
	}
	ret = server->serialServer->read(server->serialServer,buf,12,1500);
	if(ret < 0){
		LOGE("fail to read!\n");
		goto fail0;
	}

	if(ret != 12){
		LOGE("fail to len is wrong %d!\n",ret);
		getUtilsOps()->printData(buf,ret);
		goto fail0;
	}
	ret  = (memcmp(cmd,buf,2)||memcmp(&cmd[2],&buf[8],4));
	if(ret != 0){
		LOGE("fail to memcmp!\n");
		goto fail0;
	}
	//(T - 2731) / 10.0;
	int i = buf[2]<<8|buf[3];
	*centre = (i-2731)/10.0;
	i = buf[4]<<8|buf[5];
	*max = (i-2731)/10.0;
	i = buf[6]<<8|buf[7];
	*mini = (i-2731)/10.0;
	return 0;
	fail0:
		LOGE("fail serial read or write!\n");
	return -1;
}
static int checkRate(pTemperatureDetectionServer server){
		int i,ret;
		int rateArray[5] = {115200,9600,19200,38400,921600};
		float  centre;
		float max;
		float mini;
		for( i = 0;i < sizeof(rateArray)/sizeof(rateArray[0]);i++){
			//先设置本机波特率,再设置从机波特率
			ret  = server->serialServer->setBaudRate(server->serialServer,rateArray[i]);
			if(ret < 0){
				LOGE("fail to setBaudRate!");
				goto fail0;
			}
			ret = getSpecialTemperature((pTemperatureDetectionOps) server,&centre,&max,&mini);
			if(ret != 0){
				continue;
			}else{
				LOGW("serial rate is %d\n",rateArray[i]);
				break;
			}
		}
		if(i  ==  sizeof(rateArray)/sizeof(rateArray[0]))
			goto fail0;

		return 0;
		fail0:
			return -1;
}
pTemperatureDetectionOps createMlfm24TemperatureDetectionServer(const char *uartPath){
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

//	匹配模块波特率
	ret = checkRate(server);
	if(ret != 0){
		goto fail1;
	}
	return (pTemperatureDetectionOps)server;
	fail1:
		free(server);
	fail0:
		return NULL;
}
void destroyMlfm24TemperatureDetectionServer(pTemperatureDetectionOps *server){
	if(server == NULL || *server == NULL){
		return ;
	}
	pTemperatureDetectionServer pthis = (pTemperatureDetectionServer)*server;
	destroySerialServer(&pthis->serialServer);
	free(pthis);
	*server = NULL;
}
