#include "hwInterfaceManage.h"
static CPU_VER cpuVer;

static int  PA(int num) {
	if(cpuVer == A20)
		return num;
	else
		return num;
}
static int PB(int num) {
	if(cpuVer == A20)
		return num+24;
	else if(cpuVer == A64)
		return num+32;
	return -1;
}
static int PC(int num) {
	if(cpuVer == A20)
		return num+54;
	else if(cpuVer == A64)
		return num+64;
	return -1;
}
static int PD(int num) {
	if(cpuVer == A20)
		return num+85;
	else if(cpuVer == A64 )
		return num+96;
	return -1;
}
static int PE(int num) {
	if(cpuVer == A20)
		return num+119;
	else if(cpuVer == A64 )
		return num+128;
	return -1;
}
static int PF(int num) {
	if(cpuVer == A20)
		return num+137;
	else if(cpuVer == A64 )
		return num+160;
	return -1;
}
static int  PG(int num) {
	if(cpuVer == A20)
		return  num+149;
	else if(cpuVer == A64)
		return num+192;
	return -1;
}
int  PH(int num) {
	if(cpuVer == A20)
		return num+167;
	else if(cpuVer == A64)
		return num+224;
	return -1;
}

static int  PI(int num) {
	if(cpuVer == A20 )
		return num+201;
	else if(cpuVer == A64 )
		return num+256;
	return -1;
}

static int  PJ(int num) {
	if(cpuVer == A20 )
			return -1;
	else if(cpuVer == A64 )
			return num+288;
	return -1;
}
static int Pk(int num) {
	if(cpuVer == A20 )
		return -1;
	else if(cpuVer == A64 )
		return num+320;
	return -1;
}
static int PL(int num) {
	if(cpuVer == A20 )
		return -1;
	else if(cpuVer == A64 )
		return num+352;
	return -1;
}
static int  PM(int num){
	if(cpuVer == A20 )
		return -1;
	else if(cpuVer == A64 )
		return (num+384);
	return -1;
}
static int  PN(int num) {
	if(cpuVer == A20 )
		return -1;
	else if(cpuVer == A64 )
		return (num+416);
	return -1;

}
static int PO(int num) {
	if(cpuVer == A20 )
		return -1;
	else if(cpuVer == A64 )
		return (num+448);
	return -1;
}
static int getDoorBellPin(void)
{
	if(cpuVer == A20 )
		return PH(1);
	else if(cpuVer == A64 )
		return -1;//未设定
	return -1;
}
static int getDoorLockPin(void)
{
	if(cpuVer == A20 )
		return PG(3);
	else if(cpuVer == A64 )
		return PB(2);//未设定
	return -1;
}
static int getOpenDoorKeyPin(void)
{
	if(cpuVer == A20 )
		return PG(2);
	else if(cpuVer == A64 )
		return PB(3);
	return -1;
}


static int getLightSensorPin(void)
{
	if(cpuVer == A20 )
		return PH(3);
	else if(cpuVer == A64 )
		return PB(4);//未设定
	return -1;
}
static int getCameraLightPin(void)
{
	if(cpuVer == A20 )
		return PH(11);
	else if(cpuVer == A64 )
		return PL(11);
	return -1;
}
static int getKeyLightPin(void)
{
	if(cpuVer == A20 )
		return PH(0);
	else if(cpuVer == A64 )
		return PL(12);
	return -1;
}
static int getLcdSwichPin(void){
	if(cpuVer == A20 )
		return PH(7);
	else if(cpuVer == A64 )
		return PH(6);
	return -1;
}
static int getRestartPin(void){
	if(cpuVer == A20 )
		return PG(5);
	else if(cpuVer == A64 )
		return PG(5);
	return -1;
}
static int getPirPin(void)
{	if(cpuVer == A20 )
		return PH(12);
	else if(cpuVer == A64 )
		return PL(7);
	return -1;
}
static char *getIcCardUART(void)
{
	if(cpuVer == A20 )
			return "/dev/ttyS6";
	else if(cpuVer == A64 )
			return "/dev/ttyS2";
	return -1;
}
static HwInterfaceOps ops = {
		.getDoorLockPin = getDoorLockPin,
		.getOpenDoorKeyPin = getOpenDoorKeyPin,
		.getLightSensorPin = getLightSensorPin,
		.getCameraLightPin = getCameraLightPin,
		.getKeyLightPin = getKeyLightPin,
		.getLcdSwichPin = getLcdSwichPin,
		.getRestartPin = getRestartPin,
		.getPirPin = getPirPin,
		.getIcCardUART = getIcCardUART,
};

pHwInterfaceOps crateHwInterfaceServer(CPU_VER ver)
{
	cpuVer = ver;
	return &ops;
}
