#ifndef SPIMGR_H
#define SPIMGR_H

/***************************************************************************************************
    Filename:       SPIMgr.h
    Revised:        $Date: 2007-05-16 10:09:09 -0700 (Wed, 16 May 2007) $
    Revision:       $Revision: 14309 $

    Description:

       This header describes the functions that handle the serial port.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
***************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************************************
 *                                               INCLUDES
 ***************************************************************************************************/
#include "Onboard.h"

/***************************************************************************************************
 *                                               MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 *                                             CONSTANTS
 ***************************************************************************************************/
#define SOP_VALUE       0x02
#define SPI_MAX_LENGTH  128

/* Default values */
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  #define SPI_MGR_DEFAULT_PORT           ZTOOL_PORT
#elif defined (ZAPP_P1) || defined (ZAPP_P2)
  #define SPI_MGR_DEFAULT_PORT           ZAPP_PORT
#endif

#if !defined( SPI_MGR_DEFAULT_OVERFLOW )
  #define SPI_MGR_DEFAULT_OVERFLOW       FALSE
#endif

#define SPI_MGR_DEFAULT_BAUDRATE         HAL_UART_BR_38400
#define SPI_MGR_DEFAULT_THRESHOLD        SPI_THRESHOLD
#define SPI_MGR_DEFAULT_MAX_RX_BUFF      SPI_RX_BUFF_MAX
#if !defined( SPI_MGR_DEFAULT_MAX_TX_BUFF )
  #define SPI_MGR_DEFAULT_MAX_TX_BUFF      SPI_TX_BUFF_MAX
#endif
#define SPI_MGR_DEFAULT_IDLE_TIMEOUT     SPI_IDLE_TIMEOUT

/* Application Flow Control */
#define SPI_MGR_ZAPP_RX_NOT_READY         0x00
#define SPI_MGR_ZAPP_RX_READY             0x01

/*
 * Initialization
 */
extern void SPIMgr_Init (void);

/*
 * Process ZTool Rx Data
 */
void SPIMgr_ProcessZToolData ( uint8 port, uint8 event );

/*
 * Process ZApp Rx Data
 */
void SPIMgr_ProcessZAppData ( uint8 port, uint8 event );

/*
 * Calculate the check sum
 */
extern uint8 SPIMgr_CalcFCS( uint8 *msg_ptr, uint8 length );

/*
 * Register TaskID for the application
 */
extern void SPIMgr_RegisterTaskID( byte taskID );

/*
 * Register max length that application can take
 */
extern void SPIMgr_ZAppBufferLengthRegister ( uint16 maxLen );

/*
 * Turn Application flow control ON/OFF
 */
extern void SPIMgr_AppFlowControl ( uint8 status );

/***************************************************************************************************
***************************************************************************************************/

#endif  //SPIMGR_H
