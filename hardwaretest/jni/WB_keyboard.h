#ifndef WB_KEYBOARD_H
#define WB_KEYBOARD_H


/*按键上报回调函数
参数1：按键值
参数2：按键状态(up/down)

*/
typedef void (*KeyEventUpFunc)(int ,int);

typedef struct WB_KeyBoardOps{
	/*设置按键回调函数
	 * 参数1：服务对象指针
	 * 参数2：按键回调函数指针
	*/
	int (*setKeyEventUpFunc)(struct WB_KeyBoardOps *,KeyEventUpFunc);
}WB_KeyBoardOps,*pWB_KeyBoardOps;

//创建按键服务
/*
    参数：
  devPath：按键设备节点路劲.注意：此处不需填写最后的一个设备号
	 *如想监听“/dev/input/event0-9”; 只需填写如:/dev/input/event即可。
  返回值：
  创建成功返回对象指针，创建失败返回NULL
*/
pWB_KeyBoardOps createKeyBoardServer(const char *devPath);
//销毁按键服务
void destroyKeyBoardServer(pWB_KeyBoardOps *server);
/*
 * 注意：
 * 1.本库占不支持热拔插事件
 * */

#endif
