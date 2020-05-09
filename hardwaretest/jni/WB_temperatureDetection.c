#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "common/debugLog.h"
#include "temperature/mlfm24_temperatureDetection.h"
#include "temperature/tean_temperatureDetection.h"
#include "common/Utils.h"
#include "WB_temperatureDetection.h"


pTemperatureDetectionOps createTemperatureDetectionServer(const char *uartPath){
	int ret;


	pTemperatureDetectionOps ops = NULL;
	ops = createTeanTemperatureDetectionServer(uartPath);
	if( ops !=NULL){
		LOGD("createTeanTemperatureDetectionServer is succeed!\n");
		return (pTemperatureDetectionOps)(ops);
	}

	ops = createMlfm24TemperatureDetectionServer(uartPath);
	if( ops !=NULL){

		return (pTemperatureDetectionOps)ops;
	}
	fail0:
		return NULL;
}
void destroyTemperatureDetectionServer(pTemperatureDetectionOps *server){
	if(server == NULL || *server == NULL){
		return ;
	}
	destroyMlfm24TemperatureDetectionServer(server);
	destroyTeanTemperatureDetectionServer(server);
	*server = NULL;
}
