/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-03-07 16:52:39 -0800 (Wed, 07 Mar 2007) $
    Revision:       $Revision: 13715 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

#ifndef MAC_CSP_TX_H
#define MAC_CSP_TX_H


/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_mcu.h"
#include "mac_mcu.h"


/* ------------------------------------------------------------------------------------------------
 *                                         Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void macCspTxReset(void);

void macCspTxPrepCsmaUnslotted(void);
void macCspTxPrepCsmaSlotted(void);
void macCspTxPrepSlotted(void);

void macCspTxGoCsma(void);
void macCspTxGoSlotted(void);

void macCspForceTxDoneIfPending(void);

void macCspTxRequestAckTimeoutCallback(void);
void macCspTxCancelAckTimeoutCallback(void);

void macCspTxStopIsr(void);
void macCspTxIntIsr(void);


/**************************************************************************************************
 */
#endif
