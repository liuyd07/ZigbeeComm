/*********************************************************************
    Filename:       OnBoard.c
    Revised:        $Date: 2007-03-22 17:18:21 -0700 (Thu, 22 Mar 2007) $
    Revision:       $Revision: 13827 $

    Description:    This file contains the UI and control for the
                    peripherals on the EVAL development board
    Notes:          This file targets the Chipcon CC2430DB/CC2430EB

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "OnBoard.h"
#include "OSAL.h"
#include "MTEL.h"
#include "DebugTrace.h"

/* Hal */
#include "hal_lcd.h"
#include "hal_mcu.h"
#include "hal_timer.h"
#include "hal_key.h"
#include "hal_led.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Task ID not initialized
#define NO_TASK_ID 0xFF

// Minimum length RAM "pattern" for Stack check
#define MIN_RAM_INIT 12

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

uint8 OnboardKeyIntEnable;
uint8 OnboardTimerIntEnable;

// 64-bit Extended Address of this device
uint8 aExtendedAddress[8];

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

extern uint8 macMcuRandomByte(void);

/*********************************************************************
 * LOCAL VARIABLES
 */

// Registered keys task ID, initialized to NOT USED.
static byte registeredKeysTaskID = NO_TASK_ID;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void ChkReset( void );

/*********************************************************************
 * @fn      InitBoard()
 * @brief   Initialize the CC2420DB Board Peripherals
 * @param   level: COLD,WARM,READY
 * @return  None
 */
void InitBoard( byte level )
{
  if ( level == OB_COLD )
  {
    // Initialize HAL
    HAL_BOARD_INIT();
    // Interrupts off
    osal_int_disable( INTS_ALL );
    // Turn all LEDs off
    HalLedSet( HAL_LED_ALL, HAL_LED_MODE_OFF );
    // Check for Brown-Out reset
    ChkReset();

  /* Timer2 for Osal timer
   * This development board uses ATmega128 Timer/Counter3 to provide
   * system clock ticks for the OSAL scheduler. These functions perform
   * the hardware specific actions required by the OSAL_Timers module.
   */
   OnboardTimerIntEnable = FALSE;
 	 HalTimerConfig (OSAL_TIMER,                        // 8bit timer2
                  HAL_TIMER_MODE_CTC,                 // Clear Timer on Compare
                  HAL_TIMER_CHANNEL_SINGLE,           // Channel 1 - default
                  HAL_TIMER_CH_MODE_OUTPUT_COMPARE,   // Output Compare mode
                  OnboardTimerIntEnable,              // Use interrupt
                  Onboard_TimerCallBack);             // Channel Mode

  }
  else  // !OB_COLD
  {
#ifdef ZTOOL_PORT
    MT_IndReset();
#endif

     /* Initialize Key stuff */
    OnboardKeyIntEnable = HAL_KEY_INTERRUPT_DISABLE;
    HalKeyConfig( OnboardKeyIntEnable, OnBoard_KeyCallback);
  }
}

/*********************************************************************
 * @fn      ChkReset()
 * @brief   Check reset bits - if reset cause is unknown, assume a
 *          brown-out (low power), assume batteries are not reliable,
 *          hang in a loop and sequence through the LEDs.
 * @param   None
 * @return  None
 *********************************************************************/
void ChkReset( void )
{
  uint8 led;
  uint8 rib;

  // Isolate reset indicator bits
  rib = SLEEP & LRESET;

  if ( rib == RESETPO )
  {
    // Put code here to handle Power-On reset
  }
  else if ( rib == RESETEX )
  {
    // Put code here to handle External reset
  }
  else if ( rib == RESETWD )
  {
    // Put code here to handle WatchDog reset
  }
  else
  {
    // Unknown, hang and blink
    HAL_DISABLE_INTERRUPTS();
    led = HAL_LED_4;
    while ( 1 ) {
      HalLedSet( led, HAL_LED_MODE_ON );
      MicroWait( 62500 );
      MicroWait( 62500 );
      HalLedSet( led, HAL_LED_MODE_OFF );
      MicroWait( 37500 );
      MicroWait( 37500 );
      if ( !(led >>= 1) )
        led = HAL_LED_4;
    }
  }
}

/*********************************************************************
 *                        "Keyboard" Support
 *********************************************************************/

/*********************************************************************
 * Keyboard Register function
 *
 * The keyboard handler is setup to send all keyboard changes to
 * one task (if a task is registered).
 *
 * If a task registers, it will get all the keys. You can change this
 * to register for individual keys.
 *********************************************************************/
byte RegisterForKeys( byte task_id )
{
  // Allow only the first task
  if ( registeredKeysTaskID == NO_TASK_ID )
  {
    registeredKeysTaskID = task_id;
    return ( true );
  }
  else
    return ( false );
}

/*********************************************************************
 * @fn      OnBoard_SendKeys
 *
 * @brief   Send "Key Pressed" message to application.
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  status
 *********************************************************************/
byte OnBoard_SendKeys( byte keys, byte state )
{
  keyChange_t *msgPtr;

  if ( registeredKeysTaskID != NO_TASK_ID )
  {
    // Send the address to the task
    msgPtr = (keyChange_t *)osal_msg_allocate( sizeof(keyChange_t) );
    if ( msgPtr )
    {
      msgPtr->hdr.event = KEY_CHANGE;
      msgPtr->state = state;
      msgPtr->keys = keys;

      osal_msg_send( registeredKeysTaskID, (uint8 *)msgPtr );
    }
    return ( ZSuccess );
  }
  else
    return ( ZFailure );
}

/*********************************************************************
 * @fn      OnBoard_KeyCallback
 *
 * @brief   Callback service for keys
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  void
 *********************************************************************/
void OnBoard_KeyCallback ( uint8 keys, uint8 state )
{
  uint8 shift;

  // shift key (S1) is used to generate key interrupt
  // applications should not use S1 when key interrupt is enabled
  shift = (OnboardKeyIntEnable == HAL_KEY_INTERRUPT_ENABLE) ? false : ((keys & HAL_KEY_SW_6) ? true : false);

  if ( OnBoard_SendKeys( keys, shift ) != ZSuccess )
  {
    // Process SW1 here
    if ( keys & HAL_KEY_SW_1 )  // Switch 1
    {
    }
    // Process SW2 here
    if ( keys & HAL_KEY_SW_2 )  // Switch 2
    {
    }
    // Process SW3 here
    if ( keys & HAL_KEY_SW_3 )  // Switch 3
    {
    }
    // Process SW4 here
    if ( keys & HAL_KEY_SW_4 )  // Switch 4
    {
    }
    // Process SW5 here
    if ( keys & HAL_KEY_SW_5 )  // Switch 5
    {
    }
    // Process SW6 here
    if ( keys & HAL_KEY_SW_6 )  // Switch 6
    {
    }
  }
}

/*********************************************************************
 *                    SLEEP MANAGEMENT FUNCTIONS
 *
 * These functions support processing of MAC and ZStack power mode
 * transitions, used when the system goes into or awakes from sleep.
 */

 /*********************************************************************
 * @fn      OnBoard_stack_used()
 *
 * @brief
 *
 *   Runs through the stack looking for touched memory.
 *
 * @param   none
 *
 * @return  number of bytes used by the stack
 *********************************************************************/
uint16 OnBoard_stack_used( void )
{
  byte *pStack = (byte*)MCU_RAM_END;
  byte *pHold;
  byte found = false;
  byte x;

  // Look from the end of RAM for MIN_RAM_INIT number of "pattern" bytes
  // This should be the high water stack mark.
  while ( !found && pStack )
  {
    // Found an init value?
    if ( *pStack == STACK_INIT_VALUE )
    {
      // Look for a bunch in a row
      pHold = pStack;
      for ( x = 0; x < MIN_RAM_INIT; x++ )
      {
        if ( *pHold != STACK_INIT_VALUE )
          break;
        else
          pHold--;
      }
      // Did we find the needed minimum number in a row
      if ( x >= MIN_RAM_INIT )
        found = true;
    }
    if ( !found )
      pStack--;
  }

  if ( pStack )
    return ( (uint16)((byte*)MCU_RAM_END - pStack) );
  else
    return ( 0 );
}

/*********************************************************************
 * @fn      _itoa
 *
 * @brief   convert a 16bit number to ASCII
 *
 * @param   num -
 *          buf -
 *          radix -
 *
 * @return  void
 *
 *********************************************************************/
void _itoa(uint16 num, byte *buf, byte radix)
{
  char c,i;
  byte *p, rst[5];

  p = rst;
  for ( i=0; i<5; i++,p++ )
  {
    c = num % radix;  // Isolate a digit
    *p = c + (( c < 10 ) ? '0' : '7');  // Convert to Ascii
    num /= radix;
    if ( !num )
      break;
  }

  for ( c=0 ; c<=i; c++ )
    *buf++ = *p--;  // Reverse character order

  *buf = '\0';
}

/*********************************************************************
 * @fn        Onboard_rand
 *
 * @brief    Random number generator
 *
 * @param   none
 *
 * @return  uint16 - new random number
 *
 *********************************************************************/
uint16 Onboard_rand( void )
{
  uint16 randNum;

  randNum = macMcuRandomByte();
  randNum += (macMcuRandomByte() << 8);
  return ( randNum );
}

/*********************************************************************
 * @fn        Onboard_wait
 *
 * @brief    Random number generator
 *
 * @param   uint16 - time to wait
 *
 * @return  none
 *
 *********************************************************************/
void Onboard_wait( uint16 timeout )
{
  while (timeout--)
  {
    asm("NOP");
    asm("NOP");
    asm("NOP");
  }
}

/*********************************************************************
 * @fn      Osal_TimerCallBack()
 *
 * @brief   Update the timer per tick
 *
 * @param   none
 *
 * @return  local clock in milliseconds
 **********************************************************************/
void Onboard_TimerCallBack ( uint8 timerId, uint8 channel, uint8 channelMode)
{

  if ((timerId == OSAL_TIMER) && (channelMode == HAL_TIMER_CH_MODE_OUTPUT_COMPARE))
  {
    osal_update_timers();
  }
}

/*********************************************************************
 *                    EXTERNAL I/O FUNCTIONS
 *
 * User defined functions to control external devices. Add your code
 * to the following functions to control devices wired to DB outputs.
 *
 *********************************************************************/

void BigLight_On( void )
{
  // Put code here to turn on an external light
}

void BigLight_Off( void )
{
  // Put code here to turn off an external light
}

void BuzzerControl( byte on )
{
  // Put code here to turn a buzzer on/off
}

void Dimmer( byte lvl )
{
  // Put code here to control a dimmer
}

// No dip switches on this board
byte GetUserDipSw( void )
{
  return 0;
}

/*********************************************************************
*********************************************************************/
