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

#ifndef MAC_TX_H
#define MAC_TX_H


/* ------------------------------------------------------------------------------------------------
 *                                         Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_types.h"


/* ------------------------------------------------------------------------------------------------
 *                                          Define
 * ------------------------------------------------------------------------------------------------
 */
/* bit value used to form values of macTxActive */
#define MAC_TX_ACTIVE_PHYSICALLY_BV      0x80

/* state values for macTxActive; note zero is reserved for inactive state */
#define MAC_TX_ACTIVE_NO_ACTIVITY           0x00 /* zero reserved for boolean use, e.g. !macTxActive */
#define MAC_TX_ACTIVE_INITIALIZE            0x01
#define MAC_TX_ACTIVE_QUEUED                0x02
#define MAC_TX_ACTIVE_GO                    (0x03 | MAC_TX_ACTIVE_PHYSICALLY_BV)
#define MAC_TX_ACTIVE_DONE                  (0x04 | MAC_TX_ACTIVE_PHYSICALLY_BV)
#define MAC_TX_ACTIVE_LISTEN_FOR_ACK        (0x05 | MAC_TX_ACTIVE_PHYSICALLY_BV)
#define MAC_TX_ACTIVE_POST_ACK              (0x06 | MAC_TX_ACTIVE_PHYSICALLY_BV)


/* ------------------------------------------------------------------------------------------------
 *                                          Define
 * ------------------------------------------------------------------------------------------------
 */
#define MAC_TX_IS_PHYSICALLY_ACTIVE()       (macTxActive & MAC_TX_ACTIVE_PHYSICALLY_BV)


/* ------------------------------------------------------------------------------------------------
 *                                   Global Variable Externs
 * ------------------------------------------------------------------------------------------------
 */
extern uint8 macTxActive;
extern uint8 macTxBe;
extern uint8 macTxType;
extern uint8 macTxCsmaBackoffDelay;


/* ------------------------------------------------------------------------------------------------
 *                                       Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void macTxInit(void);
void macTxHaltCleanup(void);
void macTxStartQueuedFrame(void);
void macTxChannelBusyCallback(void);
void macTxDoneCallback(void);
void macTxAckReceivedCallback(uint8 seqn, uint8 pendingFlag);
void macTxAckNotReceivedCallback(void);
void macTxTimestampCallback(void);
void macTxCollisionWithRxCallback(void);


/**************************************************************************************************
 */
#endif
