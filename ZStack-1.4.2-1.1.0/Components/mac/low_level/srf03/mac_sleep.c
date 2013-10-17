/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-02-01 11:24:21 -0800 (Thu, 01 Feb 2007) $
    Revision:       $Revision: 13416 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/



/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

/* hal */
#include "hal_types.h"

/* exported low-level */
#include "mac_low_level.h"

/* low-level specific */
#include "mac_sleep.h"
#include "mac_radio.h"
#include "mac_tx.h"
#include "mac_rx.h"
#include "mac_rx_onoff.h"

/* target specific */
#include "mac_radio_defs.h"

/* debug */
#include "mac_assert.h"


/* ------------------------------------------------------------------------------------------------
 *                                         Global Variables
 * ------------------------------------------------------------------------------------------------
 */
uint8 macSleepState = MAC_SLEEP_STATE_RADIO_OFF;


/**************************************************************************************************
 * @fn          macSleepWakeUp
 *
 * @brief       Wake up the radio from sleep mode.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macSleepWakeUp(void)
{
  /* don't wake up radio if it's already awake */
  if (macSleepState == MAC_SLEEP_STATE_AWAKE)
  {
    return;
  }

  /* wake up MAC timer */
  MAC_RADIO_TIMER_WAKE_UP();

  /* if radio was completely off, restore from that state first */
  if (macSleepState == MAC_SLEEP_STATE_RADIO_OFF)
  {
    /* turn on radio power */
    MAC_RADIO_TURN_ON_POWER();
    
    /* power-up initialization of receive logic */
    macRxRadioPowerUpInit();
  }

  /* turn on the oscillator */
  MAC_RADIO_TURN_ON_OSC();
  
  /* update sleep state here before requesting to turn on receiver */
  macSleepState = MAC_SLEEP_STATE_AWAKE;

  /* turn on the receiver if enabled */
  macRxOnRequest();
}


/**************************************************************************************************
 * @fn          macSleep
 *
 * @brief       Puts radio into the selected sleep mode.
 *
 * @param       sleepState - selected sleep level, see #defines in .h file
 *
 * @return      TRUE if radio was successfully put into selected sleep mode.
 *              FALSE if it was not safe for radio to go to sleep.
 **************************************************************************************************
 */
uint8 macSleep(uint8 sleepState)
{
  halIntState_t  s;
 
  /* disable interrupts until macSleepState can be set */
  HAL_ENTER_CRITICAL_SECTION(s);

  /* assert checks */
  MAC_ASSERT(macSleepState == MAC_SLEEP_STATE_AWAKE); /* radio must be awake to put it to sleep */
  MAC_ASSERT(macRxFilter == RX_FILTER_OFF); /* do not sleep when scanning or in promiscuous mode */

  /* if either RX or TX is active or any RX enable flags are set, it's not OK to sleep */
  if (macRxActive || macRxOutgoingAckFlag || macTxActive || macRxEnableFlags)
  {
    HAL_EXIT_CRITICAL_SECTION(s);
    return(FALSE);
  }

  /* turn off the receiver */
  macRxOff();

  /* update sleep state variable */
  macSleepState = sleepState;

  /* macSleepState is now set, re-enable interrupts */
  HAL_EXIT_CRITICAL_SECTION(s);
  
  /* put MAC timer to sleep */
  MAC_RADIO_TIMER_SLEEP();

  /* put radio in selected sleep mode */
  if (sleepState == MAC_SLEEP_STATE_OSC_OFF)
  {
    MAC_RADIO_TURN_OFF_OSC();
  }
  else
  {
    MAC_ASSERT(sleepState == MAC_SLEEP_STATE_RADIO_OFF); /* unknown sleep state */
    MAC_RADIO_TURN_OFF_POWER();
  }

  /* radio successfully entered sleep mode */
  return(TRUE);
}



/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */
#if ((MAC_SLEEP_STATE_AWAKE == MAC_SLEEP_STATE_OSC_OFF) ||  \
     (MAC_SLEEP_STATE_AWAKE == MAC_SLEEP_STATE_RADIO_OFF))
#error "ERROR!  Non-unique state values."
#endif


/**************************************************************************************************
*/
