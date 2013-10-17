//adc_Voltage.c

#include "hal.h"
#include "lcd128_64.h"
#include "menu.h"
#include "string.h"

extern INT8U ScanKey(void);

void Voltage_main(void)
{
  INT8U ii;
  INT8U buf[7]="     V";
  INT16U adcvalue,temp;
  float voltage;
  TopDisp = 14;
  //ADC_SINGLE_CONVERSION(ADC_REF_1_25_V|ADC_14_BIT|ADC_VDD_3);
  ClearScreen();
  Page0Display();
  Print(6,20,"ESC to Exit",1);
  while(1)
  {
    ii=100;
    while(ii--)
    {
      ADC_SINGLE_CONVERSION(ADC_REF_1_25_V|ADC_14_BIT|ADC_VDD_3);
      while(!ADC_SAMPLE_READY());  //等待AD结束
      adcvalue = halGetAdcValue();
      //adcvalue >>= 2;
      temp += adcvalue>>2;

      if(ii<100)temp >>= 1;
    }
      voltage = temp*3*1.25/8192;
      buf[0] = (INT8U)voltage+48;
      buf[1] = '.';
      buf[2] = (INT8U)(voltage*10)%10+48;
      buf[3] = (INT8U)(voltage*100)%10+48;
      buf[4] = (INT8U)(voltage*1000)%10+48;
    //  buf[5] = 'V';
    //  buf[6] = '\0';
      Print(3,40,buf,1);
    if(ScanKey() == K_CANCEL)
    {
      TopDisp = 12;//返回传感菜单
      return;
    }
  }//while
}




