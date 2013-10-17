#ifndef TRANSMITAPP_H
#define TRANSMITAPP_H

/*********************************************************************
    Filename:       TransmitApp.h
    Revised:        $Date: 2006-10-05 11:31:56 -0700 (Thu, 05 Oct 2006) $
    Revision:       $Revision: 12197 $

    Description:

       This file contains the Transmit Application definitions.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/*********************************************************************
 * CONSTANTS
 */

// These constants are only for example and should be changed to the
// device's needs
#define TRANSMITAPP_ENDPOINT           1

#define TRANSMITAPP_PROFID             0x0F05
#define TRANSMITAPP_DEVICEID           0x0001
#define TRANSMITAPP_DEVICE_VERSION     0
#define TRANSMITAPP_FLAGS              0

#define TRANSMITAPP_MAX_CLUSTERS       1
#define TRANSMITAPP_CLUSTERID_TESTMSG  1

// Application Events (OSAL) - These are bit weighted definitions.
#define TRANSMITAPP_SEND_MSG_EVT       0x0001
#define TRANSMITAPP_RCVTIMER_EVT       0x0002
#define TRANSMITAPP_SEND_ERR_EVT       0x0004

/*********************************************************************
 * MACROS
 */

// Define to enable fragmentation transmit test
//#define TRANSMITAPP_FRAGMENTED

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Transmit Application
 */
extern void TransmitApp_Init( byte task_id );

/*
 * Task Event Processor for the Transmit Application
 */
extern UINT16 TransmitApp_ProcessEvent( byte task_id, UINT16 events );

extern void TransmitApp_ChangeState( void );
extern void TransmitApp_SetSendEvt( void );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* TRANSMITAPP_H */
