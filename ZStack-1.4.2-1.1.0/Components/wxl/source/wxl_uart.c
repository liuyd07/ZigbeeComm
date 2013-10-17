//cd wxl      串口0发数据
#include <ioCC2430.h>
#include <wxl_uart.h>
//#include <string.h>

/****************************************************************
*函数功能 ：初始化串口1										
*入口参数 ：无												
*返 回 值 ：无							
*说    明 ：57600-8-n-1						
****************************************************************/
void initUARTtest(void)
{

    CLKCON &= ~0x40;              //晶振
    while(!(SLEEP & 0x40));      //等待晶振稳定
    CLKCON &= ~0x47;             //TICHSPD128分频，CLKSPD不分频
    SLEEP |= 0x04; 		 //关闭不用的RC振荡器
    PERCFG = 0x00;				//位置1 P0口
    P0SEL |= 0x0C;				//P0用作串口
    P2DIR &= ~0xC0;                             //P0优先作为串口0
    U0CSR |= 0x80;				//UART方式
    UTX0IF = 0;

}
/****************************************************************
*函数功能 ：串口发送字符串函数					
*入口参数 : data:数据									
*			len :数据长度							
*返 回 值 ：无											
*说    明 ：				
****************************************************************/
void UartTX_Send_String(char *Data,int len)
{
  int j;
  for(j=0;j<len;j++)
  {
    U0DBUF = *Data++;
    while(UTX0IF == 0);
    UTX0IF = 0;
  }
}
void UartTX_Send_Single(char single_Data)
{
    U0DBUF = single_Data;
    while(UTX0IF == 0);
    UTX0IF = 0;
}
/*******************************************************************************
描述：
    串口接收一个字符
函数名：char UartRX_Receive_Char (void)
*******************************************************************************/
char UartRX_Receive_Char (void)
{
   char c;
   unsigned char status;
   status = U0CSR;
   U0CSR |= UART_ENABLE_RECEIVE;
   while (!URX0IF);
   c = U0DBUF;
   URX0IF = 0;
   U0CSR = status;
   return c;
}
/*******************************************************************************
描述：
    波特率的设置
函数名：void Uart_Baud_rate(int Baud_rate)
*******************************************************************************/
void Uart_Baud_rate(int Baud_rate)
{
  switch (Baud_rate)
  {
    case 24:
      U0GCR |= 6;				
      U0BAUD |= 59;				//波特率设置
    break;
    case 48:
      U0GCR |= 7;				
      U0BAUD |= 59;				//波特率设置
    break;
    case 96:
      U0GCR |= 8;				
      U0BAUD |= 59;				//波特率设置
    break;
    case 144:
      U0GCR |= 8;				
      U0BAUD |= 216;				//波特率设置
    break;
    case 192:
      U0GCR |= 9;				
      U0BAUD |= 59;				//波特率设置
    break;
    case 288:
      U0GCR |= 9;				
      U0BAUD |= 216;				//波特率设置
    break;
    case 384:
      U0GCR |= 10;				
      U0BAUD |= 59;				//波特率设置
    break;
    case 576:
      U0GCR |= 10;				
      U0BAUD |= 216;				//波特率设置
    break;
    case 768:
      U0GCR |= 11;				
      U0BAUD |= 59;				//波特率设置
    break;
    case 1152:
      U0GCR |= 11;				
      U0BAUD |= 216;				//波特率设置
    break;
    case 2304:
      U0GCR |= 12;				
      U0BAUD |= 216;				//波特率设置
    break;
    default:
    break;
  }

}

