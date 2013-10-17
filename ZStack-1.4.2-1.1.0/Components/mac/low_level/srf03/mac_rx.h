/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-03-12 18:12:24 -0700 (Mon, 12 Mar 2007) $
    Revision:       $Revision: 13738 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

#ifndef MAC_RX_H
#define MAC_RX_H

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_types.h"


/* ------------------------------------------------------------------------------------------------
 *                                           Defines
 * ------------------------------------------------------------------------------------------------
 */
#define RX_FILTER_OFF                   0
#define RX_FILTER_ALL                   1
#define RX_FILTER_NON_BEACON_FRAMES     2
#define RX_FILTER_NON_COMMAND_FRAMES    3

/* bit value used to form values of macRxActive */
#define MAC_RX_ACTIVE_PHYSICAL_BV       0x80

#define MAC_RX_ACTIVE_NO_ACTIVITY       0x00  /* zero reserved for boolean use, e.g. !macRxActive */
#define MAC_RX_ACTIVE_STARTED           (0x01 | MAC_RX_ACTIVE_PHYSICAL_BV)
#define MAC_RX_ACTIVE_DONE              0x02


/* ------------------------------------------------------------------------------------------------
 *                                          Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MAC_RX_IS_PHYSICALLY_ACTIVE()   ((macRxActive & MAC_RX_ACTIVE_PHYSICAL_BV) || macRxOutgoingAckFlag)


/* ------------------------------------------------------------------------------------------------
 *                                   Global Variable Externs
 * ------------------------------------------------------------------------------------------------
 */
extern uint8 macRxActive;
extern uint8 macRxFilter;
extern uint8 macRxOutgoingAckFlag;


/* ------------------------------------------------------------------------------------------------
 *                                         Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void macRxInit(void);
void macRxRadioPowerUpInit(void);
void macRxTxReset(void);
void macRxHaltCleanup(void);
void macRxThresholdIsr(void);
void macRxAckTxDoneCallback(void);


/**************************************************************************************************
 */
#endif
