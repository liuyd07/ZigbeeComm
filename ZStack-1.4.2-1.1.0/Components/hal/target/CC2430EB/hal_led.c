/**************************************************************************************************
    Filename:       hal_led.c
    Revised:        $Date: 2007-03-07 16:10:30 -0800 (Wed, 07 Mar 2007) $
    Revision:       $Revision: 13711 $

    Description:

    This file contains the interface to the HAL LED Service.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

/***************************************************************************************************
 *                                             INCLUDES
 ***************************************************************************************************/
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "osal.h"
#include "hal_board.h"

/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                              MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                              TYPEDEFS
 ***************************************************************************************************/
/* LED control structure */
typedef struct {
  uint8 mode;       /* Operation mode */
  uint8 todo;       /* Blink cycles left */
  uint8 onPct;      /* On cycle percentage */
  uint16 time;      /* On/off cycle time (msec) */
  uint32 next;      /* Time for next change */
} HalLedControl_t;

typedef struct
{
  HalLedControl_t HalLedControlTable[HAL_LED_DEFAULT_MAX_LEDS];
  uint8           sleepActive;
} HalLedStatus_t;


/***************************************************************************************************
 *                                           GLOBAL VARIABLES
 ***************************************************************************************************/


static uint8 HalLedState;              // LED state at last set/clr/blink update
static uint8 HalSleepLedState;         // LED state at last set/clr/blink update
static uint8 preBlinkState;            // Original State before going to blink mode
                                       // bit 0, 1, 2, 3 represent led 0, 1, 2, 3

#ifdef BLINK_LEDS
  static HalLedStatus_t HalLedStatusControl;
#endif

/***************************************************************************************************
 *                                            LOCAL FUNCTION
 ***************************************************************************************************/
#if (HAL_LED == TRUE)
void HalLedUpdate (void);
void HalLedOnOff (uint8 leds, uint8 mode);
#endif /* HAL_LED */

/***************************************************************************************************
 *                                            FUNCTIONS - API
 ***************************************************************************************************/

/***************************************************************************************************
 * @fn      HalLedInit
 *
 * @brief   Initialize LED Service
 *
 * @param   init - pointer to void that contains the initialized value
 *
 * @return  None
 ***************************************************************************************************/
void HalLedInit (void)
{
#if (HAL_LED == TRUE)
  /* Initialize all LEDs to OFF */
  HalLedSet (HAL_LED_ALL, HAL_LED_MODE_OFF);
  /* Initialize sleepActive to FALSE */
  HalLedStatusControl.sleepActive = FALSE;
#endif /* HAL_LED */
}

/***************************************************************************************************
 * @fn      HalLedSet
 *
 * @brief   Tun ON/OFF/TOGGLE given LEDs
 *
 * @param   led - bit mask value of leds to be turned ON/OFF/TOGGLE
 *          mode - BLINK, FLASH, TOGGLE, ON, OFF
 * @return  None
 ***************************************************************************************************/
uint8 HalLedSet (uint8 leds, uint8 mode)
{

#if (defined (BLINK_LEDS)) && (HAL_LED == TRUE)
  uint8 led;
  HalLedControl_t *sts;

  switch (mode)
  {
    case HAL_LED_MODE_BLINK:
      /* Default blink, 1 time, D% duty cycle */
      HalLedBlink (leds, 1, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
      break;

    case HAL_LED_MODE_FLASH:
      /* Default flash, N times, D% duty cycle */
      HalLedBlink (leds, HAL_LED_DEFAULT_FLASH_COUNT, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
      break;

    case HAL_LED_MODE_ON:
    case HAL_LED_MODE_OFF:
    case HAL_LED_MODE_TOGGLE:

      led = HAL_LED_1;
      leds &= HAL_LED_ALL;
      sts = HalLedStatusControl.HalLedControlTable;

      while (leds)
      {
        if (leds & led)
        {
          if (mode != HAL_LED_MODE_TOGGLE)
          {
            sts->mode = mode;  /* ON or OFF */
          }
          else
          {
            sts->mode ^= HAL_LED_MODE_ON;  /* Toggle */
          }
          HalLedOnOff (led, sts->mode);
          leds ^= led;
        }
        led <<= 1;
        sts++;
      }
      break;

    default:
      break;
  }

#elif (HAL_LED == TRUE)
  LedOnOff(leds, mode);
#endif /* BLINK_LEDS && HAL_LED   */

  return ( HalLedState );

}

/***************************************************************************************************
 * @fn      HalLedBlink
 *
 * @brief   Blink the leds
 *
 * @param   leds       - bit mask value of leds to be blinked
 *          numBlinks  - number of blinks
 *          percent    - the percentage in each period where the led
 *                       will be on
 *          period     - length of each cycle in milliseconds
 *
 * @return  None
 ***************************************************************************************************/
void HalLedBlink (uint8 leds, uint8 numBlinks, uint8 percent, uint16 period)
{
#if (defined (BLINK_LEDS)) && (HAL_LED == TRUE)
  uint8 led;
  HalLedControl_t *sts;

  if (leds && percent && period)
  {
    if (percent < 100)
    {
      led = HAL_LED_1;
      leds &= HAL_LED_ALL;
      sts = HalLedStatusControl.HalLedControlTable;

      while (leds)
      {
        if (leds & led)
        {
          /* Store the current state of the led before going to blinking */
          preBlinkState |= (led & HalLedState);
          
          sts->mode  = HAL_LED_MODE_OFF;                    /* Stop previous blink */
          sts->time  = period;                              /* Time for one on/off cycle */
          sts->onPct = percent;                             /* % of cycle LED is on */
          sts->todo  = numBlinks;                           /* Number of blink cycles */
          if (!numBlinks) sts->mode |= HAL_LED_MODE_FLASH;  /* Continuous */
          sts->next = osal_GetSystemClock();                /* Start now */
          sts->mode |= HAL_LED_MODE_BLINK;                  /* Enable blinking */
          leds ^= led;
        }
        led <<= 1;
        sts++;
      }
      osal_set_event (Hal_TaskID, HAL_LED_BLINK_EVENT);
    }
    else
    {
      HalLedSet (leds, HAL_LED_MODE_ON);                    /* >= 100%, turn on */
    }
  }
  else
  {
    HalLedSet (leds, HAL_LED_MODE_OFF);                     /* No on time, turn off */
  }
#elif (HAL_LED == TRUE)
  percent = (leds & HalLedState) ? HAL_LED_MODE_OFF : HAL_LED_MODE_ON;
  HalLedOnOff (leds, percent);                              /* Toggle */
#endif /* BLINK_LEDS && HAL_LED */
}

#if (HAL_LED == TRUE)
/***************************************************************************************************
 * @fn      HalLedUpdate
 *
 * @brief   Update leds to work with blink
 *
 * @param   none
 *
 * @return  none
 ***************************************************************************************************/
void HalLedUpdate (void)
{
  uint8 led;
  uint8 pct;
  uint8 leds;
  HalLedControl_t *sts;
  uint32 time;
  uint16 next;
  uint16 wait;

  next = 0;
  led  = HAL_LED_1;
  leds = HAL_LED_ALL;
  sts = HalLedStatusControl.HalLedControlTable;

  /* Check if sleep is active or not */
  if (!HalLedStatusControl.sleepActive)
  {
    while (leds)
    {
      if (leds & led)
      {
        if (sts->mode & HAL_LED_MODE_BLINK)
        {
          time = osal_GetSystemClock();
          if (time >= sts->next)
          {
            if (sts->mode & HAL_LED_MODE_ON)
            {
              pct = 100 - sts->onPct;               /* Percentage of cycle for off */
              sts->mode &= ~HAL_LED_MODE_ON;        /* Say it's not on */
              HalLedOnOff (led, HAL_LED_MODE_OFF);  /* Turn it off */

              if (!(sts->mode & HAL_LED_MODE_FLASH))
              {
                sts->todo--;                        /* Not continuous, reduce count */
                if (!sts->todo)
                {
                  sts->mode ^= HAL_LED_MODE_BLINK;  /* No more blinks */
                }
              }
            }
            else
            {
              pct = sts->onPct;                     /* Percentage of cycle for on */
              sts->mode |= HAL_LED_MODE_ON;         /* Say it's on */
              HalLedOnOff (led, HAL_LED_MODE_ON);   /* Turn it on */
            }

            if (sts->mode & HAL_LED_MODE_BLINK)
            {
              wait = (((uint32)pct * (uint32)sts->time) / 100);
              sts->next = time + wait;
            }
            else
            {
              /* no more blink, no more wait */
              wait = 0;
              /* After blinking, set the LED back to the state before it blinks */
              HalLedSet (led, ((preBlinkState & led)!=0)?HAL_LED_MODE_ON:HAL_LED_MODE_OFF);
              /* Clear the saved bit */
              preBlinkState &= ~led;
            }
          }
          else
          {
            wait = sts->next - time;  /* Time left */
          }

          if (!next || ( wait && (wait < next) ))
          {
            next = wait;
          }
        }
        leds ^= led;
      }
      led <<= 1;
      sts++;
    }

    if (next)
    {
      osal_start_timer (HAL_LED_BLINK_EVENT, next);   /* Schedule event */
    }
  }
}

/***************************************************************************************************
 * @fn      HalLedOnOff
 *
 * @brief   Turns specified LED ON or OFF
 *
 * @param   leds - LED bit mask
 *          mode - LED_ON,LED_OFF,
 *
 * @return  none
 ***************************************************************************************************/
void HalLedOnOff (uint8 leds, uint8 mode)
{
  if (leds & HAL_LED_1)
  {
    if (mode == HAL_LED_MODE_ON)
    {
      HAL_TURN_ON_LED1();
    }
    else
    {
      HAL_TURN_OFF_LED1();
    }
  }

  if (leds & HAL_LED_2)
  {
    if (mode == HAL_LED_MODE_ON)
    {
      HAL_TURN_ON_LED2();
    }
    else
    {
      HAL_TURN_OFF_LED2();
    }
  }

  if (leds & HAL_LED_3)
  {
    if (mode == HAL_LED_MODE_ON)
    {
      HAL_TURN_ON_LED3();
    }
    else
    {
      HAL_TURN_OFF_LED3();
    }
  }

  if (leds & HAL_LED_4)
  {
    if (mode == HAL_LED_MODE_ON)
    {
      HAL_TURN_ON_LED4();
    }
    else
    {
      HAL_TURN_OFF_LED4();
    }
  }

  /* Remember current state */
  if (mode)
  {
    HalLedState |= leds;
  }
  else
  {
    HalLedState &= ~leds;
  }
}
#endif /* HAL_LED */

/***************************************************************************************************
 * @fn      HalLedEnterSleep
 *
 * @brief   Store current LEDs state before sleep
 *
 * @param   none
 *
 * @return  none
 ***************************************************************************************************/
void HalLedEnterSleep( void )
{
  /* Sleep ON */
  HalLedStatusControl.sleepActive = TRUE;

#if (HAL_LED == TRUE)
  /* Save the state of each led */
  HalSleepLedState = 0;
  HalSleepLedState |= HAL_STATE_LED1();
  HalSleepLedState |= HAL_STATE_LED2() << 1;
  HalSleepLedState |= HAL_STATE_LED3() << 2;
  HalSleepLedState |= HAL_STATE_LED4() << 3;

  /* TURN OFF all LEDs to save power */
  HalLedOnOff (HAL_LED_ALL, HAL_LED_MODE_OFF);
#endif /* HAL_LED */

}

/***************************************************************************************************
 * @fn      HalLedExitSleep
 *
 * @brief   Restore current LEDs state after sleep
 *
 * @param   none
 *
 * @return  none
 ***************************************************************************************************/
void HalLedExitSleep( void )
{
#if (HAL_LED == TRUE)
  /* Load back the saved state */
  HalLedOnOff(HalSleepLedState, HAL_LED_MODE_ON);

  /* Restart - This takes care BLINKING LEDS */
  HalLedUpdate();
#endif /* HAL_LED */

  /* Sleep OFF */
  HalLedStatusControl.sleepActive = FALSE;
}

/***************************************************************************************************
***************************************************************************************************/




