#ifndef _LCD_H
#define _LCD_H

#include <ioCC2430.h>
#include <string.h>
#include "hal.h"

#define 	DATA    	1
#define 	COMMAND		0


#define		K_UP		0X20
#define		K_DOWN		0X21
#define		K_LEFT		0X22
#define		K_RIGHT		0X23
#define		K_CANCEL	0X24
#define		K_OK		0X25
#define		NO_1		0x01
#define 	NO_2		0x02

#define	LCD_595_LD 	P1_3
#define	LCD_595_CK 	P1_5
#define	LCD_595_DAT	P2_0

#define	LCD_BK   	P1_2
#define	LCD_RS		P1_7
#define	LCD_RW		P0_1
#define	LCD_CS1		P1_4
#define	LCD_E		P1_6

extern void InitDisplay(void);
extern void HalLcdInit(void);
//extern void LoadICO(INT8U y , INT8U x , INT8U n);
extern void ClearScreen(void);
extern void Printn(INT8U xx ,INT8U yy , INT32U no,INT8U yn,INT8U le);
extern void Printn8(INT8U xx ,INT8U yy , INT32U no,INT8U yn,INT8U le);
extern void Print6(INT8U xx, INT8U yy, INT8U ch1[], INT8U yn);
extern void Print8(INT16U y,INT16U x, INT8U ch[],INT16U yn);
extern void Print16(INT16U y,INT16U x,INT8U ch[],INT16U yn);
extern void Print(INT8U y, INT8U x, INT8U ch[], INT16U yn);
extern void ClearCol(INT8U Begin , INT8U End);
//extern void Rectangle(INT8U x1,INT8U y1,INT8U x2,INT8U y2);
extern void DoSetContrast(void);
extern void SetContrast(INT8U Gain, INT8U Step);
extern void SetRamAddr (INT8U Page, INT8U Col);
extern void Lcdwritedata(INT8U dat);
extern void LoadICO(void);
extern void TurnOnDisp(void);
//void MenuMenuDisp(void);


#endif

