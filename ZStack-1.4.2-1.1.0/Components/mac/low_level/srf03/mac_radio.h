/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-02-26 16:59:26 -0800 (Mon, 26 Feb 2007) $
    Revision:       $Revision: 13615 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

#ifndef MAC_RADIO_H
#define MAC_RADIO_H

/* ------------------------------------------------------------------------------------------------
 *                                         Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_types.h"


/* ------------------------------------------------------------------------------------------------
 *                                      Global Variables
 * ------------------------------------------------------------------------------------------------
 */
extern uint8 macPhyChannel;
extern uint8 macPhyTxPower;


/* ------------------------------------------------------------------------------------------------
 *                                        Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void macRadioInit(void);
void macRadioReset(void);
void macRadioUpdateTxPower(void);
void macRadioUpdateChannel(void);
uint8 macRadioComputeLQI(int8 rssiDbm, uint8 correlation);


/**************************************************************************************************
 */
#endif
