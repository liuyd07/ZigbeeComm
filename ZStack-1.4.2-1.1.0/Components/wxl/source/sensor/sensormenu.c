//mymenu.c

#include "guangming.h"
#include "temp.h"
#include "adc_voltage.h"
#include "menu.h"
#include "hal.h"
#include "lcd128_64.h"
#include "string.h"

void Sensor_Menu(void)
{
  INT8U sel;

  strcpy((char*)MenuItem[0] ,"1:Photosensor   ");
  strcpy((char*)MenuItem[1] ,"2:Temperature   ");
  strcpy((char*)MenuItem[2] ,"3:SYS Voltage   ");

  TopDisp = 12;
  FirstItem = 0;
  NowItem = 0;
  while(1)
  {
          sel = DrawMenu(MenuItem , 3);

          switch(sel)
          {
                  case 0:
                  {
                          Photosenser_main();
                  }break;
                  case 1:
                  {
                          Temperature_main();
                  }break;
                  case 2:
                  {
                          Voltage_main();
                  }break;
                  case 0xff:
                  {
                          return;
                  }
          }
  }
		
}
