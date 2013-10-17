/*********************************************************************
//光敏电阻     接P0_0
*********************************************************************/


#include <hal.h>
#include "menu.h"
extern INT8U ScanKey(void);

void adcPhotoInit(void)
{
  BYTE volatile nousetemp;
  //IO_ADC_PORT0_PIN(0, 1);   //
  ADC_ENABLE_CHANNEL(0);    //p00 为光敏输入
  ADC_SINGLE_CONVERSION(ADC_REF_1_25_V|ADC_14_BIT|ADC_AIN0); //参考电压为avdd,14位精度,P00
  nousetemp = ADCH;                                     //清寄存器

}

/*********************************************************************
//用指针作形参,得到一个字符串,直接打印到屏幕即可
*********************************************************************/
void readPhoto(INT8U *tempbuf)
{
  INT16 temp,adcvalue = 0;
  INT8U *abc = tempbuf;
//  INT8U ii = 0;
  INT8U jj=50;
  float value;

  while(jj--)
  {
    adcPhotoInit();
    while (!ADC_SAMPLE_READY());                      //等待转换完成

    temp = halGetAdcValue();

    if(temp & 0x8000)
    {
                temp = ~temp + 1;
    }

    temp >>= 2;
        asm("nop");
        asm("nop");
    adcvalue += temp;
    if(jj != 50)adcvalue >>= 1;
  }

  value = adcvalue*1.25/8192;


 //   value = 1.23;
  /*
    tempbuf[ii++] = (INT8U)value+48;
    tempbuf[ii++] = '.';
    tempbuf[ii++] = (INT8U)(value*10)%10+48;
    tempbuf[ii++] = (INT8U)(value*100)%10+48;
    tempbuf[ii++] = (INT8U)(value*1000)%10+48;
    tempbuf[ii] = '\0';
  */

    *(abc++) = (INT8U)value+48;
    *(abc++) = '.';
    *(abc++) = (INT8U)(value*10)%10+48;
    *(abc++) = (INT8U)(value*100)%10+48;
    *(abc++) = (INT8U)(value*1000)%10+48;
    *(abc++) = '\0';

}

void Photosenser_main(void)
{
  INT8U tempbuf[6];
  TopDisp = 11;
  ClearScreen();
  Page0Display();
  Print(6,20,"ESC to Exit",1);
  while(TRUE)
  {
    readPhoto(tempbuf);
    Print(3,44,tempbuf,1);
    if(ScanKey() == K_CANCEL)
    {
      TopDisp = 12;
      return;
    }
  }
}
