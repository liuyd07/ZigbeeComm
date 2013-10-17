#ifndef SERIALAPP_H
#define SERIALAPP_H

/*********************************************************************
    Filename:       SerialApp.h
    Revised:        $Date: 2007-04-17 16:50:56 -0700 (Tue, 17 Apr 2007) $
    Revision:       $Revision: 14040 $

    Description:

       This file contains the Serial Transfer Application definitions.

    Notes:

    Copyright (c) 2007 by Texas Instruments, Inc.
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
#define SERIALAPP_ENDPOINT           11

#define SERIALAPP_PROFID             0x0F05
#define SERIALAPP_DEVICEID           0x0001
#define SERIALAPP_DEVICE_VERSION     0
#define SERIALAPP_FLAGS              0

#define SERIALAPP_MAX_CLUSTERS       2
#define SERIALAPP_CLUSTERID1         1
#define SERIALAPP_CLUSTERID2         2

#define SERIALAPP_MSG_RTRY_EVT       0x0001
#define SERIALAPP_RSP_RTRY_EVT       0x0002
#define SERIALAPP_MSG_SEND_EVT       0x0004

#define SERIALAPP_MAX_RETRIES        10
  
// When end device is involved in the communication the RTRY_TIMEOUT 
// shall be set to two times end device poll rate to be safe.
// If only router and coordinator are communicating, these values 
// can be set to smaller value like 100 ms.
#define SERIALAPP_MSG_RTRY_TIMEOUT   POLL_RATE*2
#define SERIALAPP_RSP_RTRY_TIMEOUT   POLL_RATE*2
#define SERIALAPP_RSP_LOST_TIMEOUT   POLL_RATE*2

// OTA Flow Control Delays
#define SERIALAPP_ACK_DELAY          1
#define SERIALAPP_NAK_DELAY          250

// OTA Flow Control Status
#define OTA_SUCCESS                  ZSuccess
#define OTA_DUP_MSG                 (ZSuccess+1)
#define OTA_SER_BUSY                (ZSuccess+2)

/* Max buffer length that SerialApp can accept from UART */
#define SERIALAPP_MAX_RX_BUFF_LEN   10

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern byte SerialApp_TaskID;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Serial Transfer Application
 */
extern void SerialApp_Init( byte task_id );

/*
 * Task Event Processor for the Serial Transfer Application
 */
extern UINT16 SerialApp_ProcessEvent( byte task_id, UINT16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SERIALAPP_H */