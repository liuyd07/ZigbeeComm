/*********************************************************************
  Filename:       SampleAppHw.c
  Revised:        $Date: 2007-03-08 13:26:13 -0800 (Thu, 08 Mar 2007) $
  Revision:       $Revision: 13719 $

  Description:

  Copyright (c) 2007 by Texas Instruments, Inc.
  All Rights Reserved.  Permission to use, reproduce, copy, prepare
  derivative works, modify, distribute, perform, display or sell this
  software and/or its documentation for any purpose is prohibited
  without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "hal_mcu.h"
#include "hal_defs.h"

#include "SampleAppHw.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#if defined( CC2430DB )
  /* NOTES:   Jumper should be between P7.1 and P7.3 on the CC2430DB.
   *          P7.1 -> CC2430 P1.3
   *          p7.3 -> CC2430 P0.4
   */
  #define JUMPERIN_BIT  0x08
  #define JUMPERIN_SEL  P1SEL
  #define JUMPERIN_DIR  P1DIR
  #define JUMPERIN_INP  P1INP
  #define JUMPERIN      P1
  
  #define JUMPEROUT_BIT 0x10
  #define JUMPEROUT_SEL P0SEL
  #define JUMPEROUT_DIR P0DIR
  #define JUMPEROUT_INP P0INP
  #define JUMPEROUT     P0

#elif defined( CC2430EB )
  /* NOTES:   Jumper should be between I/O A pin 9 and 11 on the CC2430EB.
   *          I/O A pin 9  -> CC2430 P0.2
   *          I/O A pin 11 -> CC2430 P0.3
   */
  #define JUMPERIN_BIT  0x04
  #define JUMPERIN_SEL  P0SEL
  #define JUMPERIN_DIR  P0DIR
  #define JUMPERIN_INP  P0INP
  #define JUMPERIN      P0
  
  #define JUMPEROUT_BIT 0x08
  #define JUMPEROUT_SEL P0SEL
  #define JUMPEROUT_DIR P0DIR
  #define JUMPEROUT_INP P0INP
  #define JUMPEROUT     P0

  #if defined (HAL_UART) && (HAL_UART==TRUE)
    #error The UART will not work with this configuration. The RX & TX pins are used.
  #endif
#else
  #error Unsupported board
#endif
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      readCoordinatorJumper
 *
 * @brief   Checks for a jumper to determine if the device should
 *          become a coordinator
 *
 * NOTES:   Jumper should be between P7.1 and P7.3 on the CC2430DB.
 *          P7.1 -> CC2430 P1.3
 *          p7.3 -> CC2430 P0.4
 *
 * NOTES:   Jumper should be between I/O A pin 9 and 11 on the CC2430EB.
 *          I/O A pin 9  -> CC2430 P0.2
 *          I/O A pin 11 -> CC2430 P0.3
 *
 * @return  TRUE if the jumper is there, FALSE if not
 */
uint8 readCoordinatorJumper( void )
{
  uint8 jumpered;
  uint8 x, y;
  uint8 result;
  uint8 saveJumpInSEL;
  uint8 saveJumpInDIR;
  uint8 saveJumpInINP;
  uint8 saveJumpOutSEL;
  uint8 saveJumpOutDIR;
  uint8 saveJumpOutINP;
  
  jumpered = TRUE;
  
  // Setup PORTs
  saveJumpInSEL = JUMPERIN_SEL;
  saveJumpInDIR = JUMPERIN_DIR;
  saveJumpInINP = JUMPERIN_INP;
  saveJumpOutSEL = P0SEL;
  saveJumpOutDIR = P0DIR;
  saveJumpOutINP = P0INP;
  
  JUMPERIN_SEL &= ~(JUMPERIN_BIT);
  JUMPERIN_DIR &= ~(JUMPERIN_BIT);
  JUMPERIN_INP &= ~(JUMPERIN_BIT);
  JUMPEROUT_SEL &= ~(JUMPEROUT_BIT);
  JUMPEROUT_DIR |= JUMPEROUT_BIT;
  JUMPEROUT_INP &= ~(JUMPERIN_BIT);
  
  for ( x = 0; x < 8; x++ )
  {    
    if ( x & 0x01 )
    {
      JUMPEROUT |= JUMPEROUT_BIT;      
      for ( y = 0; y < 8; y++ );
      result = JUMPERIN & JUMPERIN_BIT;
      if ( result != JUMPERIN_BIT )
        jumpered = FALSE;
    }
    else
    {      
      JUMPEROUT &= ~(JUMPEROUT_BIT);      
      for ( y = 0; y < 8; y++ );
      result = JUMPERIN & JUMPERIN_BIT;
      if ( result != 0x00 )
        jumpered = FALSE;
    }
  }
  
  // Restore directions
  JUMPERIN_SEL = saveJumpInSEL;
  JUMPERIN_DIR = saveJumpInDIR;
  JUMPERIN_INP = saveJumpInINP;
  JUMPEROUT_SEL = saveJumpOutSEL;
  JUMPEROUT_DIR = saveJumpOutDIR;
  JUMPEROUT_INP = saveJumpOutINP;
  
  return ( jumpered );
}
