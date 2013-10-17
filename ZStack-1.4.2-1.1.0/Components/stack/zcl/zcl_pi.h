#ifndef ZCL_PI_H
#define ZCL_PI_H

/*********************************************************************
    Filename:       zcl_pi.h
    Revised:        $Date: 2006-03-28 16:28:48 -0800 (Tue, 28 Mar 2006) $
    Revision:       $Revision: 10266 $

    Description:

       This file contains the ZCL Protocol Interface Definitions

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved. Permission to use, reproduce, copy, prepare
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
#include "zcl.h"

/*********************************************************************
 * CONSTANTS
 */

/*************************************************/
/***   Protocol Interface Cluster Attributes   ***/
/*************************************************/
  
/*** The Server and Client clusters have no attributes ***/

/*** Commands ***/
#define COMMAND_PI_RAW_BACNET_DATA                                       0x00


/*****************************************************************************/
/***            Logical Cluster ID - for mapping only                      ***/
/***            These are not to be used over-the-air                      ***/
/*****************************************************************************/
#define ZCL_PI_LOGICAL_CLUSTER_ID_BACNET_INTERFACE                       0x0060


/************************************************************************************
 * MACROS
 */


/****************************************************************************
 * TYPEDEFS
 */

/*** ZCL Protocol Interface Cluster: Raw BACnet Data Cmd payload ***/
typedef struct
{
  uint8 *BACnet;
} zclCmdPIRawBACnetDataPayload_t;

// This callback is called to process a Raw BACnet Data command
// len - length of BACnet data
//  BACnet -  BACnet data
typedef void (*zclPI_BACnet_t)( uint8 len, uint8 *BACnet );

// Register Callbacks table entry - enter function pointers for callbacks that
// the application would like to receive
typedef struct			
{
  zclPI_BACnet_t                     pfnPI_BACnet; // Raw BACnet Data command

} zclPI_AppCallbacks_t;


/****************************************************************************
 * VARIABLES
 */


/****************************************************************************
 * FUNCTIONS
 */

 /*
  * Register for callbacks from this cluster library
  */
extern ZStatus_t zclPI_RegisterCmdCallbacks( uint8 endpoint, zclPI_AppCallbacks_t *callbacks );

extern ZStatus_t zclPI_Send_RawBACnetDataCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                              uint8 len, uint8 *BACnet, uint8 seqNum );

#ifdef __cplusplus
}
#endif

#endif /* ZCL_PI_H */
