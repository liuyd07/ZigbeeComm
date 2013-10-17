#ifndef MT_AF_H
#define MT_AF_H
#include <ioCC2430.h>
//º¯ÊıÉùÃ÷
extern void initUARTtest(void);
extern void UartTX_Send_String(char *Data,int len);
extern void Uart_Baud_rate(int Baud_rate);
extern char UartRX_Receive_Char (void);
extern void UartTX_Send_Single(char single_Data);

#define UART_ENABLE_RECEIVE         0x40
#define Baud_rate_2400          24
#define Baud_rate_4800          48
#define Baud_rate_9600          96
#define Baud_rate_14400         144
#define Baud_rate_19200         192
#define Baud_rate_28800         288
#define Baud_rate_38400         384
#define Baud_rate_57600         576
#define Baud_rate_76800         768
#define Baud_rate_115200        1152
#define Baud_rate_230400        2304

#endif
