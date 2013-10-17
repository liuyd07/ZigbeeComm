#include "temp.h"
#include "hal.h"
#include "lcd128_64.h"
#include "stdio.h"
#include "menu.h"

extern INT8U ScanKey(void);

void InitTempSensor(void){
   DISABLE_ALL_INTERRUPTS();
   SET_MAIN_CLOCK_SOURCE(0);
   *((BYTE __xdata*) 0xDF26) = 0x80;
}
INT8 GetTemperature(void){
  UINT8   i;
  UINT16  accValue;
  UINT16  value;

  accValue = 0;
  for( i = 0; i < 4; i++ )
  {
    ADC_SINGLE_CONVERSION(ADC_REF_1_25_V | ADC_14_BIT | ADC_TEMP_SENS);
    ADC_SAMPLE_SINGLE();
    while(!ADC_SAMPLE_READY());

    value =  ADCL >> 2;
    value |= (((UINT16)ADCH) << 6);

    accValue += value;
  }
  value = accValue >> 2; // divide by 4

  return ADC14_TO_CELSIUS(value);
}


void Temperature_main(void)
{
    char t_value;
    char t_string[4];
    TopDisp = 13;
    ClearScreen();
    Page0Display();
    Print(6,20,"ESC to Exit",1);
    while(TRUE)
    {
      t_value = GetTemperature();
      sprintf(t_string, (char*)"%dC", (INT8)t_value);
      Print(3,52,(INT8U*)t_string,1);
      if(ScanKey() == K_CANCEL)
      {
        TopDisp = 12;  //´«¸Ð²Ëµ¥
        return;
      }
    }
}
