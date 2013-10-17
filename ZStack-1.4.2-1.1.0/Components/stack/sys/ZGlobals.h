#ifndef ZGLOBALS_H
#define ZGLOBALS_H
/*********************************************************************
    Filename:       nwk_globals.h
    Revised:        $Date: 2006-11-02 11:26:08 -0800 (Thu, 02 Nov 2006) $
    Revision:       $Revision: 12495 $

    Description:

        User definable Z-Stack parameters.

    Notes:

    Copyright (c) 2007 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Values for ZCD_NV_LOGICAL_TYPE (zgDeviceLogicalType)
#define ZG_DEVICETYPE_COORDINATOR      0x00
#define ZG_DEVICETYPE_ROUTER           0x01
#define ZG_DEVICETYPE_ENDDEVICE        0x02
#define ZG_DEVICETYPE_SOFT             0x03

// Transmission retries numbers
#if !defined ( MAX_POLL_FAILURE_RETRIES )
  #define MAX_POLL_FAILURE_RETRIES 1
#endif
#if !defined ( MAX_DATA_RETRIES )
  #define MAX_DATA_RETRIES         2
#endif

// NIB parameters
#if !defined ( MAX_BCAST_RETRIES )
  #define MAX_BCAST_RETRIES        2
#endif
#if !defined ( PASSIVE_ACK_TIMEOUT )
  #define PASSIVE_ACK_TIMEOUT      5
#endif
#if !defined ( BCAST_DELIVERY_TIME )
  #define BCAST_DELIVERY_TIME      30
#endif

#if !defined ( APS_DEFAULT_MAXBINDING_TIME )
  #define APS_DEFAULT_MAXBINDING_TIME 16000
#endif

// 网络设备类型
#if defined ( SOFT_START )
  #define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_SOFT
#elif defined( ZDO_COORDINATOR )
  #define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_COORDINATOR     //协调器
#elif defined (RTR_NWK)
  #define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_ROUTER          //路由器
#else
  #define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_ENDDEVICE       //终端
#endif

// Concentrator values
#if !defined ( CONCENTRATOR_ENABLE )
  #define CONCENTRATOR_ENABLE          0
#endif

#if !defined ( CONCENTRATOR_DISCOVERY_TIME )
  #define CONCENTRATOR_DISCOVERY_TIME  0
#endif

#if !defined ( CONCENTRATOR_RADIUS )
  #define CONCENTRATOR_RADIUS          0x0a
#endif

#if !defined ( MAX_SOURCE_ROUTE )
  #define MAX_SOURCE_ROUTE             0x0c
#endif

#if !defined ( START_DELAY )
  #define START_DELAY                  0x0a
#endif

#if !defined ( SAPI_ENDPOINT )
  #define SAPI_ENDPOINT                0xe0
#endif

#define ZG_STARTUP_CLEAR               0x00
#define ZG_STARTUP_SET                 0xFF

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * NWK GLOBAL VARIABLES
 */

extern uint16 zgPollRate;
extern uint16 zgQueuedPollRate;
extern uint16 zgResponsePollRate;
extern uint16 zgRejoinPollRate;

// Variables for number of transmission retries
extern uint8 zgMaxDataRetries;
extern uint8 zgMaxPollFailureRetries;

extern uint32 zgDefaultChannelList;
extern uint8  zgDefaultStartingScanDuration;

extern uint8 zgStackProfile;

extern uint8 zgIndirectMsgTimeout;
extern uint8 zgSecurityLevel;
extern uint8 zgRouteExpiryTime;

extern uint8 zgExtendedPANID[];

extern uint8 zgMaxBcastRetires;
extern uint8 zgPassiveAckTimeout;
extern uint8 zgBcastDeliveryTime;

extern uint8 zgNwkMode;

extern uint8 zgConcentratorEnable;
extern uint8 zgConcentratorDiscoveryTime;
extern uint8 zgConcentratorRadius;
extern uint8 zgMaxSourceRoute;

/*********************************************************************
 * APS GLOBAL VARIABLES
 */

extern uint8  zgApscMaxFrameRetries;
extern uint16 zgApscAckWaitDurationPolled;
extern uint8  zgApsAckWaitMultiplier;
extern uint16 zgApsDefaultMaxBindingTime;


/*********************************************************************
 * SECURITY GLOBAL VARIABLES
 */

extern uint8 zgPreConfigKey[];
extern uint8 zgPreConfigKeys;


/*********************************************************************
 * ZDO GLOBAL VARIABLES
 */

extern uint16 zgConfigPANID;
extern uint8  zgDeviceLogicalType;


/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialize the Z-Stack Globals.
 */
extern uint8 zgInit( void );


/*
 * Get Startup Options (ZCD_NV_STARTUP_OPTION NV Item)
 */
extern uint8 zgReadStartupOptions( void );

/*
 * Write Startup Options (ZCD_NV_STARTUP_OPTION NV Item)
 *
 *      action - ZG_STARTUP_SET set bit, ZG_STARTUP_CLEAR to clear bit.
 *               The set bit is an OR operation, and the clear bit is an
 *               AND ~(bitOptions) operation.
 *      bitOptions - which bits to perform action on:
 *                      ZCD_STARTOPT_DEFAULT_CONFIG_STATE
 *                      ZDC_STARTOPT_DEFAULT_NETWORK_STATE
 *
 * Returns - ZSUCCESS if successful
 */
extern uint8 zgWriteStartupOptions( uint8 action, uint8 bitOptions );

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* ZGLOBALS_H */
