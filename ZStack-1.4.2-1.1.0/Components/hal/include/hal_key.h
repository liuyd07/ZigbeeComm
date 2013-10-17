#ifndef HAL_KEY_H
#define HAL_KEY_H

/**************************************************************************************************
  Filename:       hal_key.h
  Revised:        $Date: 2005/04/29 01:36:04 $
  Revision:       $Revision$
  Description:

  This file contains the interface to the KEY Service.
  This also contains the Task functions.

  Notes:

  Copyright (c) 2006 by Texas Instruments, Inc.
  All Rights Reserved.  Permission to use, reproduce, copy, prepare
  derivative works, modify, distribute, perform, display or sell this
  software and/or its documentation for any purpose is prohibited
  without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 *                                             INCLUDES
 **************************************************************************************************/
#include "hal_board.h"

/**************************************************************************************************
 * MACROS
 **************************************************************************************************/

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/

/* Interrupt option - Enable or disable */
#define HAL_KEY_INTERRUPT_DISABLE    0x00
#define HAL_KEY_INTERRUPT_ENABLE     0x01

/* Key state - shift or nornal */
#define HAL_KEY_STATE_NORMAL          0x00
#define HAL_KEY_STATE_SHIFT           0x01

/* Switches (keys) */
#define HAL_KEY_SW_1 0x01  // Joystick up
#define HAL_KEY_SW_4 0x02  // Joystick right
#define HAL_KEY_SW_6 0x04  // 取消cancel
#define HAL_KEY_SW_2 0x08  // Joystick left
#define HAL_KEY_SW_3 0x10  // Joystick down
#define HAL_KEY_SW_5 0x20  // 确定OK
#define HAL_KEY_SW_7 0x40  // Button S2 if available

#define HAL_KEY_SW_9 0x90;     //电池板按键2
#define HAL_KEY_SW_8 0X80;     //电池板按键1

/* Joystick */
#define HAL_KEY_UP     0x01  // Joystick up
#define HAL_KEY_RIGHT  0x02  // Joystick right
#define HAL_KEY_CANCEL 0x04  // Joystick cancel
#define HAL_KEY_LEFT   0x08  // Joystick left
#define HAL_KEY_DOWN   0x10  // Joystick down
#define HAL_KEY_ENTER   0x20  // Joystick enter

#define HAL_BT_KEY_S1   0x80
#define HAL_BT_KEY_S2   0x90



/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/
typedef void (*halKeyCBack_t) (uint8 keys, uint8 state);

/**************************************************************************************************
 *                                             GLOBAL VARIABLES
 **************************************************************************************************/
extern bool Hal_KeyIntEnable;

/**************************************************************************************************
 *                                             FUNCTIONS - API
 **************************************************************************************************/

/*
 * Initialize the Key Service
 */
extern void HalKeyInit( void );

/*
 * Configure the Key Service
 */
extern void HalKeyConfig( bool interruptEnable, const halKeyCBack_t cback);

/*
 * Read the Key status
 */
extern uint8 HalKeyRead( void);

/*
 * Enter sleep mode, store important values
 */
extern void HalKeyEnterSleep ( void );

/*
 * Exit sleep mode, retore values
 */
extern uint8 HalKeyExitSleep ( void );

/*
 * This is for internal used by hal_driver
 */
extern void HalKeyPoll ( void );

/*
 * This is for internal used by hal_sleep
 */
extern bool HalKeyPressed( void );

/**************************************************************************************************
**************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
