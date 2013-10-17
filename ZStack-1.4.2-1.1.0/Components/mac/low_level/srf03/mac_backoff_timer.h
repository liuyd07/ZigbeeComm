/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-02-05 18:04:02 -0800 (Mon, 05 Feb 2007) $
    Revision:       $Revision: 13426 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

#ifndef MAC_BACKOFF_TIMER_H
#define MAC_BACKOFF_TIMER_H


/* ------------------------------------------------------------------------------------------------
 *                                           Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void macBackoffTimerInit(void);
void macBackoffTimerReset(void);
uint32 macBackoffTimerCapture(void);
void macBackoffTimerCompareIsr(void);


/**************************************************************************************************
 */
#endif
