
#include <reg51.h>
#include "DS18B20.h"
#include "global.h"
#include "utils.h"

/* DS18B20 */
sbit DS18B20 = P1^0;
#define TEMPRATURE_UPLOAD_INTERVAL 30 // seconds

static bit init_DS18B20()
{
	bit flag = 0;
	DS18B20 = 0;  // 复位低脉冲
	delayUS(700); // 至少480us，最大960us
	DS18B20 = 1;  // 释放总线

	delayUS(70);  // DS18B20等待15~60us
	flag = DS18B20; // DS18B20发送低脉冲以示在线
	delayUS(300); // 低脉冲保持60~240us
	
	return flag;
}

static uchar ReadOneChar()
{
	/* 所有读写时序至少60us，最大120us；两个读/写周期至少1us的恢复时间 */
	uchar i = 0;
	uchar dat = 0;
	for (i = 0; i < 8; ++i) {
		dat >>= 1;
		
		DS18B20 = 0; // 拉低电平准备读
		delayUS(1); // 至少保持1us的时间
		DS18B20 = 1; // 释放总线
		delayUS(8); // 需要在拉低电平后15us内对数据进行采样(一般要求放在15us的后半时间)(这个地方比较关键，时间不能长也不能短)
		if (DS18B20 == 1) // 先读取低字节
			dat |= 0x80;
		else
			dat |= 0x00;

		delayUS(55); // 读写时序至少60us的时间周期
	}
	return dat;
}

void writeOneChar(uchar dat)
{
	/* 所有读写时序至少60us，最大120us；两个读/写周期至少1us的恢复时间 */
	uchar i = 0;
	for (i = 0; i < 8; ++i) {
		DS18B20 = 0; //拉低电平准备写 
		DS18B20 = dat & 0x1; // 写数据
		delayUS(60); // DS18B20在被拉低电平后15~60us窗口期内采集数据
		DS18B20 = 1; // 释放总线
		delayUS(3); // 至少保持1us的恢复时间
		dat >>= 1;
	}
}


uint getTemprature()
{
	uchar nTry = 3;
	int nResult;
	for (; init_DS18B20() && --nTry; );
	if (0 == nTry)
		return 0x8000; // 返回一个非法的值
	else
		nTry = 3;

	writeOneChar(0xcc); // 跳过ROM指令
	writeOneChar(0x44); // 温度转换指令

	delayMS(720); // 进行温度转换时至少要拉到高电平500ms，以完成转换(针对寄生电源而言，VCC接电源时可以不用)
	
	for (; init_DS18B20() && --nTry; );
	if (0 == nTry)
		return 0x8000;
	
	writeOneChar(0xcc); // 跳过ROM指令
	writeOneChar(0xbe); // 读取温度指令

	nResult = ReadOneChar(); // 温度值低字节
	nResult |= (ReadOneChar() << 8); // 温度值高字节
	init_DS18B20();
	return nResult;
}
