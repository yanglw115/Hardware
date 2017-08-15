
#include <intrins.h>
#include <Utils.h>

void delayS(int s)
{
	int i = s;
	while (i--)
		delay1s();
}

void delayMS(int ms)
{
	int i = ms / 100;
	while (i--)
		delay100ms();
	i = ms / 10;
	while (i--)
		delay10ms();
	i = ms % 10;
	while (i--)
		delay1ms();
}


void delay1s()		//@22.1184MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 85;
	j = 12;
	k = 155;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void delay1ms()		//@22.1184MHz
{
	unsigned char i, j;

	_nop_();
	_nop_();
	i = 22;
	j = 128;
	do
	{
		while (--j);
	} while (--i);
}


void delay10ms()		//@22.1184MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 1;
	j = 216;
	k = 35;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void delay100ms()		//@22.1184MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 9;
	j = 104;
	k = 139;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void delay1us()		//@22.1184MHz
{
	unsigned char i;

	i = 3;
	while (--i);
}

void delayUS(int us)
{
	while (us--)
		delay1us();
}



