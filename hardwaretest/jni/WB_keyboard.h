#ifndef WB_KEYBOARD_H
#define WB_KEYBOARD_H



typedef void (*KeyEventUpFunc)(int ,int);

typedef struct WB_KeyBoardOps{
	int (*setKeyEventUpFunc)(struct WB_KeyBoardOps *,KeyEventUpFunc);
}WB_KeyBoardOps,*pWB_KeyBoardOps;



pWB_KeyBoardOps createKeyBoardServer(void);
void destroyKeyBoardServer(pWB_KeyBoardOps *server);




#endif
