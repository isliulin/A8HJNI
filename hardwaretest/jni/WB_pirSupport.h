#ifndef WB_PIRSUPPORT_H__
#define WB_PIRSUPPORT_H__



typedef  enum {

	PIR_NEAR  = 0,
	PIR_LEAVE = 1,

}PIR_STATE;

typedef void(*WBPirCallBackFunc)(PIR_STATE );

typedef struct{



}WBPir_ops,*pWBPir_ops;




pWBPir_ops crateWBPirServer(int gpioPin,WBPirCallBackFunc upFunc);
void destroyWBPirServer(pWBPir_ops *base);



#endif
