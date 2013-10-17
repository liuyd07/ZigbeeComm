/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-02-20 14:29:12 -0800 (Tue, 20 Feb 2007) $
    Revision:       $Revision: 13518 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

#ifndef HAL_BOARD_CFG_H
#define HAL_BOARD_CFG_H

/*
 *     =============================================================
 *     |             Chipcon SOC_BB Battery Board, plus            |
 *     |             Chipcon CC2430EM Evaluation Module            |
 *     | --------------------------------------------------------- |
 *     |  mcu   : 8051 core                                        |
 *     |  clock : 32MHz                                            |
 *     =============================================================
 */


/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"


/* ------------------------------------------------------------------------------------------------
 *                                       Board Indentifier
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_BOARD_CC2430BB


/* ------------------------------------------------------------------------------------------------
 *                                          Clock Speed
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_CPU_CLOCK_MHZ     32


/* ------------------------------------------------------------------------------------------------
 *                                       LED Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_NUM_LEDS            1
#define HAL_LED_BLINK_DELAY()   st( { volatile uint32 i; for (i=0; i<0x5800; i++) { }; } )

/* D1 - Red */
#define LED1_BV           BV(0)
#define LED1_SBIT         P1_0
#define LED1_DDR          P1DIR
#define LED1_POLARITY     ACTIVE_LOW


/* ------------------------------------------------------------------------------------------------
 *                                    Push Button Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!    /* double negation forces result to be '1' */

/* S1 */
#define PUSH1_BV          BV(0)
#define PUSH1_SBIT        P0_1
#define PUSH1_POLARITY    ACTIVE_LOW


/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ----------- Board Initialization ---------- */
#define HAL_BOARD_INIT()                                         \
{                                                                \
  /* set direction for GPIO outputs  */                          \
  LED1_DDR |= LED1_BV;                                           \
                                                                 \
  /* configure tristates */                                      \
  P0INP |= PUSH1_BV;                                             \
}

/* ----------- Debounce ---------- */
#define HAL_DEBOUNCE(expr)    { int i; for (i=0; i<500; i++) { if (!(expr)) i = 0; } }

/* ----------- Push Buttons ---------- */
#define HAL_PUSH_BUTTON1()        (PUSH1_POLARITY (PUSH1_SBIT))
#define HAL_PUSH_BUTTON2()        (0)
#define HAL_PUSH_BUTTON3()        (0)
#define HAL_PUSH_BUTTON4()        (0)
#define HAL_PUSH_BUTTON5()        (0)
#define HAL_PUSH_BUTTON6()        (0)

/* ----------- LED's ---------- */
#define HAL_TURN_OFF_LED1()       st( LED1_SBIT = LED1_POLARITY (0); )
#define HAL_TURN_OFF_LED2()
#define HAL_TURN_OFF_LED3()       HAL_TURN_OFF_LED1()
#define HAL_TURN_OFF_LED4()

#define HAL_TURN_ON_LED1()        st( LED1_SBIT = LED1_POLARITY (1); )
#define HAL_TURN_ON_LED2()
#define HAL_TURN_ON_LED3()        HAL_TURN_ON_LED1()
#define HAL_TURN_ON_LED4()

#define HAL_TOGGLE_LED1()         st( if (LED1_SBIT) { LED1_SBIT = 0; } else { LED1_SBIT = 1;} )
#define HAL_TOGGLE_LED2()
#define HAL_TOGGLE_LED3()         HAL_TOGGLE_LED1()
#define HAL_TOGGLE_LED4()

#define HAL_STATE_LED1()          (LED1_POLARITY (LED1_SBIT))
#define HAL_STATE_LED2()          0
#define HAL_STATE_LED3()          HAL_STATE_LED1()
#define HAL_STATE_LED4()          0


/* ------------------------------------------------------------------------------------------------
 *                                     Driver Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* Set to TRUE enable ADC usage, FALSE disable it */
#ifndef HAL_ADC
#define HAL_ADC TRUE
#endif

/* Set to TRUE enable DMA usage, FALSE disable it */
#ifndef HAL_DMA
#define HAL_DMA TRUE
#endif

/* Set to TRUE enable AES usage, FALSE disable it */
#ifndef HAL_AES
#define HAL_AES TRUE
#endif

/* Set to TRUE enable LCD usage, FALSE disable it */
#ifndef HAL_LCD
#define HAL_LCD TRUE
#endif

/* Set to TRUE enable LED usage, FALSE disable it */
#ifndef HAL_LED
#define HAL_LED TRUE
#endif
#if (!defined BLINK_LEDS) && (HAL_LED == TRUE)
#define BLINK_LEDS
#endif

/*
  Set to TRUE enable KEY usage, FALSE disable it
  Notes: On 2430EB/DB analog joystick is used to simulate
         keys. Keys won't work unless HAL_ADC is also set
         to TRUE
*/
#ifndef HAL_KEY
#define HAL_KEY TRUE
#endif

/* Set to TRUE enable UART usage, FALSE disable it */
#ifndef HAL_UART
#if (defined ZAPP_P1) || (defined ZAPP_P2) || (defined ZTOOL_P1) || (defined ZTOOL_P2)
#define HAL_UART TRUE
#else
#define HAL_UART FALSE
#endif /* ZAPP, ZTOOL */
#endif /* HAL_UART */

#if HAL_UART
  #define HAL_UART_0_ENABLE  TRUE
  #define HAL_UART_1_ENABLE  FALSE

  #if HAL_DMA
    #if !defined( HAL_UART_DMA )
      /* Can only run DMA on one USART or the other, not both at the same time.
       * So define to 1 for USART0, 2 for USART1, or 0 for neither.
       */
      #define HAL_UART_DMA  1
    #endif
  #else
    #undef  HAL_UART_DMA
    #define HAL_UART_DMA 0
  #endif

  /* The preferred method of implementation is by DMA for faster buad rate
   * support. Customer may prefer to use the DMA channels for something else,
   * in which case USART can be driven by ISR. Also, if the 2nd USART is to be
   * used, this module does not currently support using 2 more DMA channels for
   * it, so it must use ISR.
   */
  #if !defined( HAL_UART_ISR )
    #define HAL_UART_ISR     FALSE
  #endif

  /* It is not common for embedded applications to open/close/re-open a USART,
   * so make the close function optional.
   */
  #if !defined( HAL_UART_CLOSE )
    #define HAL_UART_CLOSE  FALSE
  #endif
#else
  #define HAL_UART_0_ENABLE  FALSE
  #define HAL_UART_1_ENABLE  FALSE
  #define HAL_UART_DMA       FALSE
  #define HAL_UART_ISR       FALSE
  #define HAL_UART_CLOSE     FALSE
#endif
/*******************************************************************************************************
*/
#endif
