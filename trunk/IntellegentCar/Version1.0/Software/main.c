#include <reg51.h>
#include <intrins.h>
#include <utils.h>
#include <string.h>

#include "global.h"
#include "DS18B20.h"


#define false 0
#define true 1

sfr P4 = 0xc0;

sfr PCON2 = 0x97; // ��ʱ�ӷ�Ƶ
sfr IAC_CONTR = 0xc7; // ������Ƹ�λ
sfr WDT_CONTR = 0xc1; // ���Ź����ƼĴ���

sfr AUXR = 0x8e;
//sfr S2BUF = 0x9b; // ����2 Buffer
sfr S3BUF = 0xad; // ����3 Buffer
sfr S4BUF = 0x85; // ����4 Buffer
sfr AUXR1 = 0xa2; // ����1�����л��ļĴ���

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
����ָ��:
ֹͣ: FE AB 07 01 00 00 B1
ǰ��: FE AB 07 01 01 00 B2
��ת: FE AB 07 01 02 00 B3
��ת: FE AB 07 01 03 00 B4
����: FE AB 07 01 04 00 B5
�ٶ�: FE AB 07 01 05 xx xx
�¶�: FE AB 07 02 01 00 B3

��Ӧ:
7λ: D5 33 07 01 00 00 10
8λ: D5 33 08 02 01 xx xx xx
*/
#define MAX_DATA_LENGTH 7 /* ���ڽ���������󳤶� */
#define MIN_DATA_LENGTH 7 /* ���ڽ���������С���� */
#define WIFI_CMD_HEAD1 0xFE
#define WIFI_CMD_HEAD2 0xAB
#define WIFI_REPLY_HEAD1 0xD5
#define WIFI_REPLY_HEAD2 0x33

#define WIFI_CMD_TYPE_POS 3 // ָ���������������е�����
#define WIFI_CMD_ACTION_POS 4 // ָ������������е�����

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


#define PWM_CYCLE 0x1000L // ���Ϊ0x7fff/32767
#define SPEED_MAX (PWM_CYCLE)
#define SPEED_MIN (PWM_CYCLE / 2)

sbit STBY1 = P3^5;
sbit STBY2 = P3^4;
sbit PWMA_F_L = P3^7; // ʹ��PWM2�������1
sbit PWMA_F_R = P2^1; // ʹ��PWM3�������2
sbit PWMA_B_L = P2^2; // ʹ��PWM4�������3
sbit PWMA_B_R = P2^3; // ʹ��PWM5�������4
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

#define Debug_S3_To_S1 1 // ����3���յ�������ת��������1�����������
bit g_bIdealState = true;
ulong g_speedCurrent = 0;

void stopCar()
{
	STBY1 = 0; // ���ʹ�ܴ�
	STBY2 = 0;
}

void goForward()
{
	AIN_F_L_1 = 0; // �����ת
	AIN_F_L_2 = 1;
	AIN_F_R_1 = 0;
	AIN_F_R_2 = 1;
	AIN_B_L_1 = 0; // �����ת
	AIN_B_L_2 = 1;
	AIN_B_R_1 = 0;
	AIN_B_R_2 = 1;

	P_SW2 = 0x80; // XSFR
	PWM2T1 = g_speedCurrent;
	PWM3T1 = g_speedCurrent;
	PWM4T1 = g_speedCurrent;
	PWM5T1 = g_speedCurrent;
	P_SW2 &= (~0x80); // �ر�XSFR

	STBY1 = 1; // ���ʹ�ܴ�
	STBY2 = 1;
}

void goBackward()
{
	AIN_F_L_1 = 1; // �����ת
	AIN_F_L_2 = 0;
	AIN_F_R_1 = 1;
	AIN_F_R_2 = 0;
	AIN_B_L_1 = 1; // �����ת
	AIN_B_L_2 = 0;
	AIN_B_R_1 = 1;
	AIN_B_R_2 = 0;

	P_SW2 = 0x80; // XSFR
	PWM2T1 = g_speedCurrent;
	PWM3T1 = g_speedCurrent;
	PWM4T1 = g_speedCurrent;
	PWM5T1 = g_speedCurrent;
	P_SW2 &= (~0x80); // �ر�XSFR

	STBY1 = 1; // ���ʹ�ܴ�
	STBY2 = 1;
}

void goLeft()
{
	AIN_F_L_1 = 0; // �����ת
	AIN_F_L_2 = 1;
	AIN_F_R_1 = 1;
	AIN_F_R_2 = 0;
	AIN_B_L_1 = 0; // �����ת
	AIN_B_L_2 = 1;
	AIN_B_R_1 = 1;
	AIN_B_R_2 = 0;

	P_SW2 = 0x80; // XSFR
	PWM2T1 = g_speedCurrent;
	PWM3T1 = g_speedCurrent;
	PWM4T1 = g_speedCurrent;
	PWM5T1 = g_speedCurrent;
	P_SW2 &= (~0x80); // �ر�XSFR

	STBY1 = 1; // ���ʹ�ܴ�
	STBY2 = 1;
}

void goRight()
{
	AIN_F_L_1 = 1; // �����ת
	AIN_F_L_2 = 0;
	AIN_F_R_1 = 0;
	AIN_F_R_2 = 1;
	AIN_B_L_1 = 1; // �����ת
	AIN_B_L_2 = 0;
	AIN_B_R_1 = 0;
	AIN_B_R_2 = 1;
	
	P_SW2 = 0x80; // XSFR
	PWM2T1 = g_speedCurrent;
	PWM3T1 = g_speedCurrent;
	PWM4T1 = g_speedCurrent;
	PWM5T1 = g_speedCurrent;
	P_SW2 &= (~0x80); // �ر�XSFR

	STBY1 = 1; // ���ʹ�ܴ�
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
		//while (!(S3CON & 0x2)); /* ע�����ﲻҪд���while(~(S3CON & 0x2))�� */
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
			g_bufferSerial[2] = 0x8; // ����Ϊ8
			/* �����ȡ�ɹ�������Ҫ�������ν��� */
			temp = getTemprature();
			g_bufferSerial[5] = (uchar)temp;
			g_bufferSerial[6] = (uchar)(temp << 8);
			len = LEN_REPLY_TEMPRATURE;
		break;
		default:
		break;
	}
	/* ���һ���ֽ� */
	g_bufferSerial[len - 1] = getSumForArray(g_bufferSerial, len);
	//g_bufferSerial[len] = 0x0a;//���ӻ��з�(Android������õ���BufferedReader.readLine()����Ҫ�ӻ��з�)
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
			// �¶ȵĻ�ȡ��reply�н��У����⴫��
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
			/* ͷ�����������ֽڣ���һ���ֽ� */
			g_bufferSerial[g_lenReceive++] = byte;
		}

	} else if (g_lenReceive == 1) { 
		if (byte == WIFI_CMD_HEAD2) {
			/* ���յ�����ͷ����2���ֽڣ�ȷ������ͷ������ */
			g_bufferSerial[g_lenReceive++] = byte;
		} else {
			/* ���յ������ݵ�2���ֽڲ���ʵ��ͷ����2���������������ݣ�Ҳ������ͷ����1���ֽ� */
			if (byte == WIFI_CMD_HEAD1) {
				/* ���������ͷ����1�ֽڣ������滻 */
				g_lenReceive = 0;
				g_bufferSerial[g_lenReceive++] = byte;
			} else {
				/* ǰ����յ��Ķ��������� */
				g_lenReceive = 0;
			}
		}

	} else if (g_lenReceive == 2) {
		if ((byte > MAX_DATA_LENGTH) || (byte < MIN_DATA_LENGTH)) {
			/* ���ݷǷ������� */
			g_lenReceive = 0;
		} else {
			len = g_bufferSerial[g_lenReceive++] = byte;
		}

	} else if (g_lenReceive < len) {
		g_bufferSerial[g_lenReceive++] = byte;

		if (g_lenReceive >= len) {
				/* ���յ��������� */
				g_lenReceive = 0; /* �´����¿�ʼ�������� */
		
				if (!checksumWiFiData(len)) {
					replyWiFiMessage(false);
					return;
				}	
				doWiFiCmd();
			}
	}
}


/* �����ж� */
void Uart1() interrupt 4
{
    if (RI) {
        RI = 0;
    }
    if (TI) {
        TI = 0;
    }
}

/* �����ж� */
void Uart3() interrupt 17
{
	/* !!!�����ж��в�Ҫ�ٷ��������ݣ���������ִ��ڽ����쳣����������������������н��д����ȡ���ݵĻ���Ҳ������Ϊ�ж�Ƶ��̫�죬�������ݶ�ʧ */
    /* ������������еľ��涼ȥ��֮�󣬳���ȴҲ����ͨ������3�򴮿�1ת������ */

	uchar temp = 0;
	g_bIdealState = false; // �˳�����ģʽ

	/* ������ */
    if (S3CON & 0x1) {
		temp = S3BUF;
		/* ��Ӧ�ж�֮�������������㣬Ҳ����������������㣬ֻҪ�����ĵ�˵�����ͻ���ճɹ��ı�־λ�����㼴�ɣ���ʹ�������ٴο��� */

		/* ʵ��֤���������ڴ����ж϶���������У����д������ݵ�����д���� */
		sendSerialMessage3(temp);
		handleReceivedWiFiData(temp);
		S3CON &= (~0x1);
    }

	/* д���� */
    if (S3CON & 0x2) {
        S3CON &= (~0x2);
    }
}



void init_AUXR1()
{

	/**
      * ����STC15ϵ�е�Ƭ��������2ֻ��ʹ�ö�ʱ��2��Ϊ�����ʷ����������ܹ�ѡ��������ʱ����Ϊ�����ʷ�������������1Ĭ��ѡ��ʱ��2��Ϊ�����ʷ�������Ҳ
	 * ����ѡ��ʱ��1��Ϊ�����ʷ�����������3Ĭ��ѡ��ʱ��2��Ϊ�䲨���ʷ�������Ҳ����ѡ��ʱ��3��Ϊ�䲨���ʷ�����������4Ĭ��ѡ��ʱ��2��Ϊ�䲨
      * �ʷ�������Ҳ����ѡ��ʱ��4��Ϊ�䲨���ʷ�������
	 **/
    SCON = 0x50; // 8λ�ɱ䲨���ʣ�Ĭ�ϲ���P3.0,P3.1��Ϊ����1
	/* ���п�ʹ�ö�ʱ��2��Ϊ�䲨���ʷ�����ʱ��������=(��ʱ��T2�������)/4����BAUD=(FOSC/(65536-T2L))/4 */
    T2L = (65536 - (FOSC/4/BAUD));
    T2H = (65536 - (FOSC/4/BAUD)) >> 8;
    
    AUXR = 0x14; // ��ʱ��2���ٶ��Ǵ�ͳ8051��12��������Ƶ��������ʱ��2����
    AUXR |= 0x1; // ѡ��ʱ��2��Ϊ����1�Ĳ����ʷ�����
    
}

void init_AUXR3()
{
#if 0
	S3CON = (1 << 6) | (1 << 4); // ʹ�ö�ʱ��3��Ϊ�����ʷ�������������գ�
	T3L = (65536 - (FOSC/4/BAUD));
	T3H = (65536 - (FOSC/4/BAUD)) >> 8;
	T4T3M = (1 << 3) | (1 << 1); // ����Ƶ������T3���У�
#else
	S3CON = (1 << 4); // ʹ�ö�ʱ2��Ϊ�����ʷ�������������գ�
#endif
	IE2 = (1 << 3); // ����3�ж�����
}

void init_Motor_Driver()
{
	STBY1 = 0; // ���ʹ�ܴ�
	STBY2 = 0;
	AIN_F_L_1 = 0; // �����ת
	AIN_F_L_2 = 1;
	AIN_B_L_1 = 0; // �����ת
	AIN_B_L_2 = 1;
	P0M0 &= (~0xf0); // PWM��صĳ�ʼ��ʱȫ��׼˫�����������������ʹ��
	P0M1 &= (~0xf0);
	P1M0 &= (~0xc0);
	P1M1 &= (~0xc0);
	P2M0 &= (~0x9e); // PWM3/4/5׼˫�����(PWM��صĳ�ʼ��ʱȫ��׼˫�����������������ʹ��)
	P2M1 &= (~0x9e);
	P3M0 &= (~0x80); // PWM2׼˫�����
	P3M1 &= (~0x80);
	P4M0 &= (~0x2c);
	P4M1 &= (~0x2c);
	P_SW2 = 0x80; // XSFR

	PWMA_F_L = 0;
	PWMA_F_R = 0;
	PWMA_B_L = 0;
	PWMA_B_R = 0;
	PWMCKS = 0x0; // PWMʱ��ԴΪϵͳʱ�Ӿ���Ƶ֮���ʱ��
	PWMCKS |= 0x0; // PWMʱ��Ϊϵͳʱ��/(PS[3:0] + 1) 

	PWMC = PWM_CYCLE; // PWM��������

	/* PWM2: ǰ��PWM3: ǰ�ң�PWM4: ����PWM5: ���� */
	PWM2T1 = SPEED_MAX; //����5V��ѹ����СҲ��ҪSPEED_MAX/2���ܴ������
	PWM2T2 = 0;
	PWM2CR = 0x0; // PWM4��P2.2���

	PWM3T1 = SPEED_MAX;
	PWM3T2 = 0;
	PWM3CR = 0x0; // PWM4��P2.2���

	PWM4T1 = SPEED_MAX;
	PWM4T2 = 0;
	PWM4CR = 0x0; // PWM4��P2.2���
	
	PWM5T1 = SPEED_MAX;
	PWM5T2 = 0;
	PWM5CR = 0x0; // PWM4��P2.2���
	
	PWMCFG = 0x0; // PWM2/3/4/5��ʼ��ƽΪ�͵�ƽ


	PWMIF = 0x0;
	PWMCR = 0x0f; // ʹ��PWM2/3/4/5������
	PWMCR |= 0x80; // ʹ��PWM����������ʼ����
	P_SW2 &= (~0x80); // �ر�XSFR

	g_speedCurrent = SPEED_MAX;
}

void init_WiFi()
{
	//WiFiEnable = 1;
}

void init()
{
	PCON2 = 0x0; // ϵͳʱ����IRC����һ�£�����Ƶ��Ҳ���������
	init_AUXR1(); // �����ж�ʱ��2��������������Ҫ���ں���(�������3Ҳʹ�ö�ʱ��2��Ϊ�����ʷ������Ļ�)
	init_AUXR3(); 
	init_Motor_Driver();
    init_WiFi();
    ES = 1; // ���������ж�
    EA = 1; // �������ж�

}

void main()
{
    int nTemp;
	init();
    
    while (1) {
#if 0
		delayS(60);
		if (g_bIdealState) {
			PCON |= 0x1; // �������ģʽ
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
