#include <reg51.h>
#include <intrins.h>
#include <utils.h>
#include <string.h>

#include "global.h"
#include "DS18B20.h"


#define false 0
#define true 1

sfr P4 = 0xc0;

sfr PCON2 = 0x97; // 主时钟分频
sfr IAC_CONTR = 0xc7; // 软件控制复位
sfr WDT_CONTR = 0xc1; // 看门狗控制寄存器

sfr AUXR = 0x8e;
//sfr S2BUF = 0x9b; // 串口2 Buffer
sfr S3BUF = 0xad; // 串口3 Buffer
sfr S4BUF = 0x85; // 串口4 Buffer
sfr AUXR1 = 0xa2; // 串口1进行切换的寄存器

sfr S3CON = 0xac;
sfr T3H = 0xd4;
sfr T3L = 0xd5;
sfr T4T3M = 0xd1;
sfr IE2 = 0xaf;

sfr T2H = 0xd6;
sfr T2L = 0xd7;

/*------- WiFi --------*/
//sbit WiFiEnable = P0^4;

/*------- WiFi --------*/

/*------- Protocol --------*/
/*
控制指令:
停止: FE AB 07 01 00 00 B1
前进: FE AB 07 01 01 00 B2
左转: FE AB 07 01 02 00 B3
右转: FE AB 07 01 03 00 B4
后退: FE AB 07 01 04 00 B5
速度: FE AB 07 01 05 xx xx
温度: FE AB 07 02 01 00 B3

响应:
7位: D5 33 07 01 00 00 10
8位: D5 33 08 02 01 xx xx xx
*/
#define MAX_DATA_LENGTH 7 /* 串口接收数据最大长度 */
#define MIN_DATA_LENGTH 7 /* 串口接收数据最小长度 */
#define WIFI_CMD_HEAD1 0xFE
#define WIFI_CMD_HEAD2 0xAB
#define WIFI_REPLY_HEAD1 0xD5
#define WIFI_REPLY_HEAD2 0x33

#define WIFI_CMD_TYPE_POS 3 // 指令类型所在数据中的索引
#define WIFI_CMD_ACTION_POS 4 // 指令动作所在数据中的索引

#define LEN_REPLY_CONTROL 7
#define LEN_REPLY_TEMPRATURE 8

uchar g_bufferSerial[12] = {0};
uchar g_lenReceive = 0;
uchar g_replyMotorControl[8] = {0xD5, 0x33, 0x07, 0x01, 0x00, 0x00, 0x00};
uchar g_replyTempratureGet[9] = {0xD5, 0x33, 0x08, 0x02, 0x01, 0x00, 0x00, 0x00};

enum WiFi_CMD_Type {
	TYPE_MOTOR_CONTROL = 1,
	TYPE_TEMPRATURE_GET = 2
};

enum WiFi_CMD_ACTION {
	ACTION_STOP = 0,
	ACTION_FORWARD = 1,
	ACTION_LEFT = 2,
	ACTION_RIGHT = 3,
	ACTION_BACKWARD = 4,
	ACTION_SPEED = 5,
	ACTION_TEMPRATURE = 6
};

/*------- Protocol --------*/


/*------- motor--------*/
sfr P1M0 = 0x92;
sfr P1M1 = 0x91;
sfr P0M0 = 0x94;
sfr P0M1 = 0x93;
sfr P2M0 = 0x96;
sfr P2M1 = 0x95;
sfr P3M0 = 0xb2;
sfr P3M1 = 0xb1;
sfr P4M0 = 0xb4;
sfr P4M1 = 0xb3;

sfr PWMCFG = 0xf1;
sfr PWMCR = 0xf5;
sfr PWMIF = 0xf6;
sfr P_SW2 = 0xba;

#define PWMC (*(uintvx *)0xfff0)
#define PWMCH (*(ucharvx *)0xfff0)
#define PWMCL (*(ucharvx *)0xfff1)
#define PWMCKS (*(uintvx *)0xfff2)

#define PWM2T1 (*(uintvx *)0xff00)
#define PWM2T1H (*(ucharvx *)0xff00)
#define PWM2T1L (*(ucharvx *)0xff01)
#define PWM2T2 (*(uintvx *)0xfff02)
#define PWM2T2H (*(ucharvx *)0xff02)
#define PWM2T2L (*(ucharvx *)0xff03)
#define PWM2CR (*(uintvx *)0xff04)

#define PWM3T1 (*(uintvx *)0xff10)
#define PWM3T1H (*(ucharvx *)0xff10)
#define PWM3T1L (*(ucharvx *)0xff11)
#define PWM3T2 (*(uintvx *)0xfff12)
#define PWM3T2H (*(ucharvx *)0xff12)
#define PWM3T2L (*(ucharvx *)0xff13)
#define PWM3CR (*(uintvx *)0xff14)

#define PWM4T1 (*(uintvx *)0xff20)
#define PWM4T1H (*(ucharvx *)0xff20)
#define PWM4T1L (*(ucharvx *)0xff21)
#define PWM4T2 (*(uintvx *)0xfff22)
#define PWM4T2H (*(ucharvx *)0xff22)
#define PWM4T2L (*(ucharvx *)0xff23)
#define PWM4CR (*(uintvx *)0xff24)

#define PWM5T1 (*(uintvx *)0xff30)
#define PWM5T1H (*(ucharvx *)0xff30)
#define PWM5T1L (*(ucharvx *)0xff31)
#define PWM5T2 (*(uintvx *)0xfff32)
#define PWM5T2H (*(ucharvx *)0xff32)
#define PWM5T2L (*(ucharvx *)0xff33)
#define PWM5CR (*(uintvx *)0xff34)


#define PWM_CYCLE 0x1000L // 最大为0x7fff/32767
#define SPEED_MAX (PWM_CYCLE)
#define SPEED_MIN (PWM_CYCLE / 2)

sbit STBY1 = P3^5;
sbit STBY2 = P3^4;
sbit PWMA_F_L = P3^7; // 使用PWM2驱动电机1
sbit PWMA_F_R = P2^1; // 使用PWM3驱动电机2
sbit PWMA_B_L = P2^2; // 使用PWM4驱动电机3
sbit PWMA_B_R = P2^3; // 使用PWM5驱动电机4
sbit AIN_F_L_1 = P2^5;
sbit AIN_F_L_2 = P2^6;
sbit AIN_F_R_1 = P4^5;
sbit AIN_F_R_2 = P2^7;
sbit AIN_B_L_1 = P0^2;
sbit AIN_B_L_2 = P0^3;
sbit AIN_B_R_1 = P0^5;
sbit AIN_B_R_2 = P0^6;
/*------- motor--------*/

#define FREQUENCY_DIVISION_1 0x0
#define FREQUENCY_DIVISION_2 0x1
#define FREQUENCY_DIVISION_4 0x2
#define FREQUENCY_DIVISION_8 0x3
#define FREQUENCY_DIVISION_16 0x4
#define FREQUENCY_DIVISION_32 0x5
#define FREQUENCY_DIVISION_64 0x6
#define FREQUENCY_DIVISION_128 0x7

#define SOFT_RESET_USER_DATA 0x20;
#define SOFT_RESET_ISP_DATA 0x60;

#define FOSC 22118400L // 22.184MHz
#define BAUD 115200L
#define NONE_PARITY 0

#define Debug_S3_To_S1 1 // 串口3接收到的数据转发给串口1输出，调试用
bit g_bIdealState = true;
ulong g_speedCurrent = 0;

void stopCar()
{
	STBY1 = 0; // 电机使能打开
	STBY2 = 0;
}

void goForward()
{
	AIN_F_L_1 = 0; // 电机正转
	AIN_F_L_2 = 1;
	AIN_F_R_1 = 0;
	AIN_F_R_2 = 1;
	AIN_B_L_1 = 0; // 电机正转
	AIN_B_L_2 = 1;
	AIN_B_R_1 = 0;
	AIN_B_R_2 = 1;

	P_SW2 = 0x80; // XSFR
	PWM2T1 = g_speedCurrent;
	PWM3T1 = g_speedCurrent;
	PWM4T1 = g_speedCurrent;
	PWM5T1 = g_speedCurrent;
	P_SW2 &= (~0x80); // 关闭XSFR

	STBY1 = 1; // 电机使能打开
	STBY2 = 1;
}

void goBackward()
{
	AIN_F_L_1 = 1; // 电机反转
	AIN_F_L_2 = 0;
	AIN_F_R_1 = 1;
	AIN_F_R_2 = 0;
	AIN_B_L_1 = 1; // 电机反转
	AIN_B_L_2 = 0;
	AIN_B_R_1 = 1;
	AIN_B_R_2 = 0;

	P_SW2 = 0x80; // XSFR
	PWM2T1 = g_speedCurrent;
	PWM3T1 = g_speedCurrent;
	PWM4T1 = g_speedCurrent;
	PWM5T1 = g_speedCurrent;
	P_SW2 &= (~0x80); // 关闭XSFR

	STBY1 = 1; // 电机使能打开
	STBY2 = 1;
}

void goLeft()
{
	AIN_F_L_1 = 0; // 电机正转
	AIN_F_L_2 = 1;
	AIN_F_R_1 = 1;
	AIN_F_R_2 = 0;
	AIN_B_L_1 = 0; // 电机正转
	AIN_B_L_2 = 1;
	AIN_B_R_1 = 1;
	AIN_B_R_2 = 0;

	P_SW2 = 0x80; // XSFR
	PWM2T1 = g_speedCurrent;
	PWM3T1 = g_speedCurrent;
	PWM4T1 = g_speedCurrent;
	PWM5T1 = g_speedCurrent;
	P_SW2 &= (~0x80); // 关闭XSFR

	STBY1 = 1; // 电机使能打开
	STBY2 = 1;
}

void goRight()
{
	AIN_F_L_1 = 1; // 电机正转
	AIN_F_L_2 = 0;
	AIN_F_R_1 = 0;
	AIN_F_R_2 = 1;
	AIN_B_L_1 = 1; // 电机正转
	AIN_B_L_2 = 0;
	AIN_B_R_1 = 0;
	AIN_B_R_2 = 1;
	
	P_SW2 = 0x80; // XSFR
	PWM2T1 = g_speedCurrent;
	PWM3T1 = g_speedCurrent;
	PWM4T1 = g_speedCurrent;
	PWM5T1 = g_speedCurrent;
	P_SW2 &= (~0x80); // 关闭XSFR

	STBY1 = 1; // 电机使能打开
	STBY2 = 1;
}

void setSpeed(uchar speed) /* 1---100 */
{
	g_speedCurrent = SPEED_MIN + (SPEED_MAX - SPEED_MIN) / 100 * speed;
	speed = 0;
}

void sendSerialMessage3(uchar byte)
{
#if Debug_S3_To_S1
	SBUF = byte;
#endif

}

uchar getSumForArray(uchar array[], uchar len)
{
	uchar i = 0, temp = 0;
	for (i = 0; i < len - 1; ++i) {
		temp += array[i];
	}
	return temp;
}

bit checksumWiFiData(uchar len)
{
	if (getSumForArray(g_bufferSerial, len) != g_bufferSerial[len - 1])
		return false;
	return true;
}

void sendDataToWiFi(uchar *pArray, uchar len)
{
	uchar i = 0;
	for (; i < len; ++i) {
		S3BUF = pArray[i];
		//while (!(S3CON & 0x2)); /* 注意这里不要写错成while(~(S3CON & 0x2))了 */
		S3CON &= (~0x2);
		delayMS(20);
	}
}

void replyWiFiMessage(bit bSuccess)
{
	uchar len = 0;
	int temp;
	g_bufferSerial[0] = WIFI_REPLY_HEAD1;
	g_bufferSerial[1] = WIFI_REPLY_HEAD2;
	switch (g_bufferSerial[WIFI_CMD_TYPE_POS]) {
		case TYPE_MOTOR_CONTROL:
			g_bufferSerial[5] = (bSuccess? 0x00: 0x01);
			len = LEN_REPLY_CONTROL;
		break;
		case TYPE_TEMPRATURE_GET:
			g_bufferSerial[2] = 0x8; // 长度为8
			/* 如果获取成功，还需要函数传参进来 */
			temp = getTemprature();
			g_bufferSerial[5] = (uchar)temp;
			g_bufferSerial[6] = (uchar)(temp << 8);
			len = LEN_REPLY_TEMPRATURE;
		break;
		default:
		break;
	}
	/* 最后一个字节 */
	g_bufferSerial[len - 1] = getSumForArray(g_bufferSerial, len);
	//g_bufferSerial[len] = 0x0a;//增加换行符(Android端如果用到了BufferedReader.readLine()才需要加换行符)
	sendDataToWiFi(g_bufferSerial, len);
	
}

void doWiFiCmd()
{
	bit bOK = 1;
	uchar temp = 0;
	switch (g_bufferSerial[WIFI_CMD_TYPE_POS]) {
		case TYPE_MOTOR_CONTROL:
			switch (g_bufferSerial[WIFI_CMD_ACTION_POS]) {
				case ACTION_STOP: stopCar(); break;
				case ACTION_FORWARD: goForward(); break;
				case ACTION_LEFT: goLeft(); break;
				case ACTION_RIGHT: goRight(); break;
				case ACTION_BACKWARD: goBackward(); break;
				case ACTION_SPEED:
					temp = g_bufferSerial[5];
					if ((temp >= 1) && (temp <= 100))
						setSpeed(temp);
					else
						bOK = 0;
				break;
				default:
					bOK = 0;
				break;
			}
		if (bOK) {
			replyWiFiMessage(true);
		}
		break;
		case TYPE_TEMPRATURE_GET:
			// 温度的获取在reply中进行，避免传参
			replyWiFiMessage(true);
		break;
		default:
			bOK = 0;
		break;
	}
}

void handleReceivedWiFiData(uchar byte)
{
    uchar temp = 0;
	static uchar len = 0;
	if (g_lenReceive == 0) {
		temp = 0;
		len = 0;
		if (byte == WIFI_CMD_HEAD1) {
			/* 头部共有两个字节，第一个字节 */
			g_bufferSerial[g_lenReceive++] = byte;
		}

	} else if (g_lenReceive == 1) { 
		if (byte == WIFI_CMD_HEAD2) {
			/* 接收到数据头部第2个字节，确认数据头部正常 */
			g_bufferSerial[g_lenReceive++] = byte;
		} else {
			/* 接收到的数据第2个字节不是实际头部第2个，可能是脏数据，也可能是头部第1个字节 */
			if (byte == WIFI_CMD_HEAD1) {
				/* 如果是数据头部第1字节，进行替换 */
				g_lenReceive = 0;
				g_bufferSerial[g_lenReceive++] = byte;
			} else {
				/* 前面接收到的都是脏数据 */
				g_lenReceive = 0;
			}
		}

	} else if (g_lenReceive == 2) {
		if ((byte > MAX_DATA_LENGTH) || (byte < MIN_DATA_LENGTH)) {
			/* 数据非法，清零 */
			g_lenReceive = 0;
		} else {
			len = g_bufferSerial[g_lenReceive++] = byte;
		}

	} else if (g_lenReceive < len) {
		g_bufferSerial[g_lenReceive++] = byte;

		if (g_lenReceive >= len) {
				/* 接收到完整数据 */
				g_lenReceive = 0; /* 下次重新开始接收数据 */
		
				if (!checksumWiFiData(len)) {
					replyWiFiMessage(false);
					return;
				}	
				doWiFiCmd();
			}
	}
}


/* 串口中断 */
void Uart1() interrupt 4
{
    if (RI) {
        RI = 0;
    }
    if (TI) {
        TI = 0;
    }
}

/* 串口中断 */
void Uart3() interrupt 17
{
	/* !!!串口中断中不要再发串口数据，这样会出现串口接收异常。并且数据如果在主程序中进行处理读取数据的话，也可能因为中断频率太快，导致数据丢失 */
    /* 但是如果把所有的警告都去除之后，程序却也可以通过串口3向串口1转发数据 */

	uchar temp = 0;
	g_bIdealState = false; // 退出空闲模式

	/* 读数据 */
    if (S3CON & 0x1) {
		temp = S3BUF;
		/* 响应中断之后必须用软件清零，也可以在外面进行清零，只要满足文档说明发送或接收成功的标志位被清零即可，以使缓冲区再次可用 */

		/* 实践证明，可以在串口中断读处理过程中，进行串口数据的连续写操作 */
		sendSerialMessage3(temp);
		handleReceivedWiFiData(temp);
		S3CON &= (~0x1);
    }

	/* 写数据 */
    if (S3CON & 0x2) {
        S3CON &= (~0x2);
    }
}



void init_AUXR1()
{

	/**
      * 对于STC15系列单片机，串口2只能使用定时器2作为波特率发生器，不能够选择其他定时器作为波特率发生器；而串口1默认选择定时器2作为波特率发生器，也
	 * 可以选择定时器1作为波特率发生器；串口3默认选择定时器2作为其波特率发生器，也可以选择定时器3作为其波特率发生器；串口4默认选择定时器2作为其波
      * 率发生器，也可以选择定时器4作为其波特率发生器；
	 **/
    SCON = 0x50; // 8位可变波特率，默认采用P3.0,P3.1作为串口1
	/* 串行口使用定时器2作为其波特率发生器时，波特率=(定时器T2的溢出率)/4，即BAUD=(FOSC/(65536-T2L))/4 */
    T2L = (65536 - (FOSC/4/BAUD));
    T2H = (65536 - (FOSC/4/BAUD)) >> 8;
    
    AUXR = 0x14; // 定时器2的速度是传统8051的12倍，不分频，并允许定时器2运行
    AUXR |= 0x1; // 选择定时器2作为串口1的波特率发生器
    
}

void init_AUXR3()
{
#if 0
	S3CON = (1 << 6) | (1 << 4); // 使用定时器3作为波特率发生器；允许接收；
	T3L = (65536 - (FOSC/4/BAUD));
	T3H = (65536 - (FOSC/4/BAUD)) >> 8;
	T4T3M = (1 << 3) | (1 << 1); // 不分频；允许T3运行；
#else
	S3CON = (1 << 4); // 使用定时2作为波特率发生器；允许接收；
#endif
	IE2 = (1 << 3); // 串口3中断允许
}

void init_Motor_Driver()
{
	STBY1 = 0; // 电机使能打开
	STBY2 = 0;
	AIN_F_L_1 = 0; // 电机正转
	AIN_F_L_2 = 1;
	AIN_B_L_1 = 0; // 电机正转
	AIN_B_L_2 = 1;
	P0M0 &= (~0xf0); // PWM相关的初始化时全部准双向输出，否则不能正常使用
	P0M1 &= (~0xf0);
	P1M0 &= (~0xc0);
	P1M1 &= (~0xc0);
	P2M0 &= (~0x9e); // PWM3/4/5准双向输出(PWM相关的初始化时全部准双向输出，否则不能正常使用)
	P2M1 &= (~0x9e);
	P3M0 &= (~0x80); // PWM2准双向输出
	P3M1 &= (~0x80);
	P4M0 &= (~0x2c);
	P4M1 &= (~0x2c);
	P_SW2 = 0x80; // XSFR

	PWMA_F_L = 0;
	PWMA_F_R = 0;
	PWMA_B_L = 0;
	PWMA_B_R = 0;
	PWMCKS = 0x0; // PWM时钟源为系统时钟经分频之后的时钟
	PWMCKS |= 0x0; // PWM时钟为系统时钟/(PS[3:0] + 1) 

	PWMC = PWM_CYCLE; // PWM计数周期

	/* PWM2: 前左；PWM3: 前右；PWM4: 后左；PWM5: 后右 */
	PWM2T1 = SPEED_MAX; //对于5V电压，最小也需要SPEED_MAX/2才能带动电机
	PWM2T2 = 0;
	PWM2CR = 0x0; // PWM4到P2.2输出

	PWM3T1 = SPEED_MAX;
	PWM3T2 = 0;
	PWM3CR = 0x0; // PWM4到P2.2输出

	PWM4T1 = SPEED_MAX;
	PWM4T2 = 0;
	PWM4CR = 0x0; // PWM4到P2.2输出
	
	PWM5T1 = SPEED_MAX;
	PWM5T2 = 0;
	PWM5CR = 0x0; // PWM4到P2.2输出
	
	PWMCFG = 0x0; // PWM2/3/4/5初始电平为低电平


	PWMIF = 0x0;
	PWMCR = 0x0f; // 使能PWM2/3/4/5发生器
	PWMCR |= 0x80; // 使能PWM发生器，开始计数
	P_SW2 &= (~0x80); // 关闭XSFR

	g_speedCurrent = SPEED_MAX;
}

void init_WiFi()
{
	//WiFiEnable = 1;
}

void init()
{
	PCON2 = 0x0; // 系统时钟与IRC保持一致，不分频，也不对外输出
	init_AUXR1(); // 里面有定时器2的启动，所以需要放在后面(如果串口3也使用定时器2作为波特率发生器的话)
	init_AUXR3(); 
	init_Motor_Driver();
    init_WiFi();
    ES = 1; // 开启串口中断
    EA = 1; // 开启总中断

}

void main()
{
    int nTemp;
	init();
    
    while (1) {
#if 0
		delayS(60);
		if (g_bIdealState) {
			PCON |= 0x1; // 进入空闲模式
			_nop_();
			_nop_();
			_nop_();
			_nop_();
			
		}
		g_bIdealState = true;	
			
#else
		delayS(TEMPRATURE_UPLOAD_INTERVAL);
		EA = 0;
		nTemp = getTemprature();
		g_replyTempratureGet[5] = (uchar)(nTemp >> 8);
		g_replyTempratureGet[6] = (uchar)nTemp;
		g_replyTempratureGet[8 - 1] = getSumForArray(g_replyTempratureGet, 8);
				
		EA = 1;
		sendDataToWiFi(g_replyTempratureGet, 8);
#if 0
		sendSerialMessage3(nTemp >> 8);
		delay1ms();
		sendSerialMessage3(nTemp);
#endif
#endif
    }
}
