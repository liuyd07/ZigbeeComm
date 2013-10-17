#ifndef SAMPLEAPP_H
#define SAMPLEAPP_H

/*********************************************************************
    Filename:       SampleApp.h
    Revised:        $Date: 2007-03-08 13:26:13 -0800 (Thu, 08 Mar 2007) $
    Revision:       $Revision: 13719 $

    Description:

       This file contains the Sample Application definitions.

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
#define SAMPLEAPP_ENDPOINT           20

#define SAMPLEAPP_PROFID             0x0F08
#define SAMPLEAPP_DEVICEID           0x0001
#define SAMPLEAPP_DEVICE_VERSION     0
#define SAMPLEAPP_FLAGS              0

#define SAMPLEAPP_MAX_CLUSTERS       2
#define SAMPLEAPP_PERIODIC_CLUSTERID 1
#define SAMPLEAPP_FLASH_CLUSTERID     2

// 发送消息间隔时间
#define SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT   2000     // Every 2 seconds   2008年9月17日更改默认发送消息间隔为2秒

// Application Events (OSAL) - These are bit weighted definitions.
#define SAMPLEAPP_SEND_PERIODIC_MSG_EVT       0x0001
#define UART_RX_CB_EVT  0x0002             //自定义事件，串口接收中断

// Group ID for Flash Command
#define SAMPLEAPP_FLASH_GROUP                  0x0001

// Flash Command Duration - in milliseconds
#define SAMPLEAPP_FLASH_DURATION               1000


extern uint8 state2;

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * 普通应用初始化任务
 */
extern void SampleApp_Init( uint8 task_id );

extern void menu_Init(uint8 task_id);

/*
 * 普通应用任务事件处理器
 */
extern UINT16 SampleApp_ProcessEvent( uint8 task_id, uint16 events );

extern uint16 menu_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SAMPLEAPP_H */
