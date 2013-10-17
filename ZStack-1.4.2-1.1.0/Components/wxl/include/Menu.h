#ifndef _MENU_H
#define _MENU_H

#include "lcd128_64.h"
#include "hal_types.h"
#define Main_Menu_1            0
#define Zigbee_Menu_2          1
#define Uart_Menu_2            2
#define Sensor_Menu_2          3
#define Setting_Menu_2         4
#define Aboat_Menu_2           6
#define Send_Menu_3            7
#define Single_Send_Menu_4     8
#define Continuous_Send_Menu_4 9
#define Send_Broadcast_Menu_5  10
#define Nod_Menu_3             11
#define Send_Long_Addr_Menu_5  12
#define Send_Short_Addr_Menu_5 13
#define Uart_TX_Menu_3         14
#define Uart_RX_Menu_3         15
#define Uart_PP_Menu_3         16
#define Pingpong_Test_Menu_5   17
#define Sensor_ReadBattery_Menu_3 18
#define Sensor_Temp_Menu_3     19




/********************************************************************/
#define Send_Flag_Single      0
#define Send_Flag_Continuous  1
#define Broadcast_Continuous  10
#define Short_Send_Continuous 11
#define Pingpong_Send_Continuous  12
#define Open                  1
#define Close                 0
#define Send_Mode_Broadcast       0xFFFF

/********************************************************************/

#define HAL_ADC_REF_125V    0x00    /* 内部1.25V参考电压 */
#define HAL_ADC_DEC_064     0x00    /* 8位ADC*/
#define HAL_ADC_DEC_128     0x10    /* 10位ADC*/
#define HAL_ADC_DEC_512     0x30    /* 12位ADC*/
#define HAL_ADC_CHN_VDD3    0x0f    /* 输入通道电源电压检测: VDD/3 */
#define HAL_ADC_CHN_TEMP    0x0e    /* 温度传感器 */



#define Sensor_ReadBattery        1
#define Sensor_Temp               2


//**************************************************************************

extern unsigned char MenuItem[13][17];
extern INT8U NowItem;
extern INT8U FirstItem;
extern INT8U TopDisp;

extern INT8U  DrawMenu(INT8U MenuItem[][17] , INT8U MenuNo,uint8 key);
extern void MenuMenuDisp( uint8 keys );
extern void Menu_all(uint8 key);
extern void Page1Display(INT8U ss, INT8U tt);
extern void TurnOnDisp(void);
#endif
