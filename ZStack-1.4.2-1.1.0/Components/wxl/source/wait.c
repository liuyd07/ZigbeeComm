/******************************************************************************
Filename:     wait.c
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0
******************************************************************************/
#include "hal.h"


//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
void halWait(BYTE wait){
   UINT32 largeWait;

   if(wait == 0)
   {return;}
   largeWait = ((UINT16) (wait << 7));
   largeWait += 114*wait;


   largeWait = (largeWait >> CLKSPD);
   while(largeWait--);

   return;
}
