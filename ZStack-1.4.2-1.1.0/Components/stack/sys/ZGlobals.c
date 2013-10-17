/*********************************************************************
    Filename:       ZGlobals.c
    Revised:        $Date: 2007-01-08 12:56:09 -0800 (Mon, 08 Jan 2007) $
    Revision:       $Revision: 13236 $

    Description:

        User definable Z-Stack parameters.

    Notes:

    Copyright (c) 2007 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "OSAL_Nv.h"
#include "ZDObject.h"
#include "ZGlobals.h"

#include "OnBoard.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

typedef struct zgItem
{
  uint16 id;
  uint16 len;
  void *buf;
} zgItem_t;

/*********************************************************************
 * NWK GLOBAL VARIABLES
 */

// Polling values
uint16 zgPollRate = POLL_RATE;
uint16 zgQueuedPollRate = QUEUED_POLL_RATE;
uint16 zgResponsePollRate = RESPONSE_POLL_RATE;
uint16 zgRejoinPollRate = REJOIN_POLL_RATE;

// Transmission retries numbers
uint8 zgMaxDataRetries = NWK_MAX_DATA_RETRIES;
uint8 zgMaxPollFailureRetries = MAX_POLL_FAILURE_RETRIES;

// Default channel list
uint32 zgDefaultChannelList = DEFAULT_CHANLIST;

// Default starting scan duration
uint8 zgDefaultStartingScanDuration = STARTING_SCAN_DURATION;

// Stack profile Id
uint8 zgStackProfile = STACK_PROFILE_ID;

// Default indirect message holding timeout
uint8 zgIndirectMsgTimeout = NWK_INDIRECT_MSG_TIMEOUT;

// Security level
uint8 zgSecurityLevel = SECURITY_LEVEL;

// Route expiry
uint8 zgRouteExpiryTime = ROUTE_EXPIRY_TIME;

// Extended PAN Id
uint8 zgExtendedPANID[Z_EXTADDR_LEN];

// Broadcast parameters
uint8 zgMaxBcastRetires   = MAX_BCAST_RETRIES;
uint8 zgPassiveAckTimeout = PASSIVE_ACK_TIMEOUT;
uint8 zgBcastDeliveryTime = BCAST_DELIVERY_TIME;

// Network mode
uint8 zgNwkMode = NWK_MODE;

// Many-to-one values
uint8 zgConcentratorEnable = CONCENTRATOR_ENABLE;
uint8 zgConcentratorDiscoveryTime = CONCENTRATOR_DISCOVERY_TIME;
uint8 zgConcentratorRadius = CONCENTRATOR_RADIUS;
uint8 zgMaxSourceRoute = MAX_SOURCE_ROUTE;


/*********************************************************************
 * APS GLOBAL VARIABLES
 */

// The maximum number of retries allowed after a transmission failure
uint8 zgApscMaxFrameRetries = APSC_MAX_FRAME_RETRIES;

// The maximum number of seconds (milliseconds) to wait for an
// acknowledgement to a transmitted frame.

// This number is used by polled devices.
uint16 zgApscAckWaitDurationPolled = APSC_ACK_WAIT_DURATION_POLLED;

// This number is used by non-polled devices in the following formula:
//   (100 mSec) * (_NIB.MaxDepth * zgApsAckWaitMultiplier)
uint8 zgApsAckWaitMultiplier = 2;

// The maximum number of milliseconds for the end device binding
uint16 zgApsDefaultMaxBindingTime = APS_DEFAULT_MAXBINDING_TIME;

/*********************************************************************
 * SECURITY GLOBAL VARIABLES
 */

// This is the pre-configured key in use (from NV memory)
uint8 zgPreConfigKey[SEC_KEY_LEN];

// If true, preConfigKey should be configured on all devices on the network
// If false, it is configured only on the coordinator and sent to other
// devices upon joining.
uint8 zgPreConfigKeys = FALSE; //  TRUE;


/*********************************************************************
 * ZDO GLOBAL VARIABLES
 */

// Configured PAN ID
uint16 zgConfigPANID = ZDAPP_CONFIG_PAN_ID;

// 设备类型
uint8 zgDeviceLogicalType = DEVICE_LOGICAL_TYPE;

// 启动延迟
uint8 zgStartDelay = START_DELAY;

/*********************************************************************
 * NON-STANDARD GLOBAL VARIABLES
 */

// Simple API Endpoint
uint8 zgSapiEndpoint = SAPI_ENDPOINT;

/*********************************************************************
 * LOCAl VARIABLES
 */

/*********************************************************************
 * ZGlobal Item Table
 */
static CONST zgItem_t zgItemTable[] =
{
#if defined ( NV_INIT )
  {
    ZCD_NV_LOGICAL_TYPE, sizeof(zgDeviceLogicalType), &zgDeviceLogicalType
  },
  {
    ZCD_NV_POLL_RATE, sizeof(zgPollRate), &zgPollRate
  },
  {
   ZCD_NV_QUEUED_POLL_RATE, sizeof(zgQueuedPollRate), &zgQueuedPollRate
  },
  {
    ZCD_NV_RESPONSE_POLL_RATE, sizeof(zgResponsePollRate), &zgResponsePollRate
  },
  {
    ZCD_NV_REJOIN_POLL_RATE, sizeof(zgRejoinPollRate), &zgRejoinPollRate
  },
  {
   ZCD_NV_DATA_RETRIES, sizeof(zgMaxDataRetries), &zgMaxDataRetries
  },
  {
     ZCD_NV_POLL_FAILURE_RETRIES, sizeof(zgMaxPollFailureRetries), &zgMaxPollFailureRetries
  },
  {
    ZCD_NV_CHANLIST, sizeof(zgDefaultChannelList), &zgDefaultChannelList
  }
  ,
  {
    ZCD_NV_SCAN_DURATION, sizeof(zgDefaultStartingScanDuration), &zgDefaultStartingScanDuration
  },
  {
    ZCD_NV_STACK_PROFILE, sizeof(zgStackProfile), &zgStackProfile
  },
  {
    ZCD_NV_INDIRECT_MSG_TIMEOUT, sizeof(zgIndirectMsgTimeout), &zgIndirectMsgTimeout
  },
  {
    ZCD_NV_ROUTE_EXPIRY_TIME, sizeof(zgRouteExpiryTime), &zgRouteExpiryTime
  },
  {
    ZCD_NV_EXTENDED_PAN_ID, Z_EXTADDR_LEN, &zgExtendedPANID
  },
  {
    ZCD_NV_BCAST_RETRIES, sizeof(zgMaxBcastRetires), &zgMaxBcastRetires
  },
  {
    ZCD_NV_PASSIVE_ACK_TIMEOUT, sizeof(zgPassiveAckTimeout), &zgPassiveAckTimeout
  },
  {
    ZCD_NV_BCAST_DELIVERY_TIME, sizeof(zgBcastDeliveryTime), &zgBcastDeliveryTime
  },
  {
    ZCD_NV_NWK_MODE, sizeof(zgNwkMode), &zgNwkMode
  },
  {
    ZCD_NV_CONCENTRATOR_ENABLE, sizeof(zgConcentratorEnable), &zgConcentratorEnable
  },
  {
    ZCD_NV_CONCENTRATOR_DISCOVERY, sizeof(zgConcentratorDiscoveryTime), &zgConcentratorDiscoveryTime
  },
  {
    ZCD_NV_CONCENTRATOR_RADIUS, sizeof(zgConcentratorRadius), &zgConcentratorRadius
  },
  {
    ZCD_NV_MAX_SOURCE_ROUTE, sizeof(zgMaxSourceRoute), &zgMaxSourceRoute
  },
#ifndef NONWK
  {
    ZCD_NV_PANID, sizeof(zgConfigPANID), &zgConfigPANID
  },
  {
    ZCD_NV_PRECFGKEY, SEC_KEY_LEN, &zgPreConfigKey
  },
  {
    ZCD_NV_PRECFGKEYS_ENABLE, sizeof(zgPreConfigKeys), &zgPreConfigKeys
  },
  {
    ZCD_NV_SECURITY_LEVEL, sizeof(zgSecurityLevel), &zgSecurityLevel
  },
#endif // NONWK
  {
    ZCD_NV_APS_FRAME_RETRIES, sizeof(zgApscMaxFrameRetries), &zgApscMaxFrameRetries
  },
  {
    ZCD_NV_APS_ACK_WAIT_DURATION, sizeof(zgApscAckWaitDurationPolled), &zgApscAckWaitDurationPolled
  },
  {
    ZCD_NV_APS_ACK_WAIT_MULTIPLIER, sizeof(zgApsAckWaitMultiplier), &zgApsAckWaitMultiplier
  },
  {
    ZCD_NV_BINDING_TIME, sizeof(zgApsDefaultMaxBindingTime), &zgApsDefaultMaxBindingTime
  },
  {
    ZCD_NV_START_DELAY, sizeof(zgStartDelay), &zgStartDelay
  },
  {
    ZCD_NV_SAPI_ENDPOINT, sizeof(zgSapiEndpoint), &zgSapiEndpoint
  },
#endif // NV_INIT
  // Last item -- DO NOT MOVE IT!
  {
    0x00, 0, NULL
  }
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static uint8 zgItemInit( uint16 id, uint16 len, void *buf, uint8 setDefault );


/*********************************************************************
 * @fn       zgItemInit()
 *
 * @brief
 *
 *   Initialize a global item. If the item doesn't exist in NV memory,
 *   write the system default (value passed in) into NV memory. But if
 *   it exists, set the item to the value stored in NV memory.
 *
 *   Also, if setDefault is TRUE and the item exists, we will write
 *   the default value to NV space.
 *
 * @param   id - item id
 * @param   len - item len
 * @param   buf - pointer to the item
 * @param   setDefault - TRUE to set default, not read
 *
 * @return  ZSUCCESS if successful, NV_ITEM_UNINIT if item did not
 *          exist in NV, NV_OPER_FAILED if failure.
 */
static uint8 zgItemInit( uint16 id, uint16 len, void *buf, uint8 setDefault )
{

  uint8 status;

  // If the item doesn't exist in NV memory, create and initialize
  // it with the value passed in.
  status = osal_nv_item_init( id, len, buf );
  if ( status == ZSUCCESS )
  {
    if ( setDefault )
    {
      // Write the default value back to NV
      status =  osal_nv_write( id, 0, len, buf );
    }
    else
    {
      // The item exists in NV memory, read it from NV memory
      status = osal_nv_read( id, 0, len, buf );
    }
  }

  return (status);
}

/*********************************************************************
 * API FUNCTIONS
 */


/*********************************************************************
 * @fn          zgInit
 *
 * @brief
 *
 *   Initialize the Z-Stack Globals. If an item doesn't exist in
 *   NV memory, write the system default into NV memory. But if
 *   it exists, set the item to the value stored in NV memory.
 *
 * NOTE: The Startup Options (ZCD_NV_STARTUP_OPTION) indicate
 *       that the Config state items (zgItemTable) need to be
 *       set to defaults (ZCD_STARTOPT_DEFAULT_CONFIG_STATE). The
 *
 *
 * @param       none
 *
 * @return      ZSUCCESS if successful, NV_ITEM_UNINIT if item did not
 *              exist in NV, NV_OPER_FAILED if failure.
 */
uint8 zgInit( void )
{
  uint8  i = 0;
  uint8  setDefault = FALSE;

  // Do we want to default the Config state values
  if ( zgReadStartupOptions() & ZCD_STARTOPT_DEFAULT_CONFIG_STATE )
  {
    setDefault = TRUE;
  }

#if 0
  // Enable this section if you need to track the number of resets
  // This section is normally disabled to minimize "wear" on NV memory
  uint16 bootCnt = 0;

  // Update the Boot Counter
  if ( osal_nv_item_init( ZCD_NV_BOOTCOUNTER, sizeof(bootCnt), &bootCnt ) == ZSUCCESS )
  {
    // Get the old value from NV memory
    osal_nv_read( ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt );
  }

  // Increment the Boot Counter and store it into NV memory
  if ( setDefault )
    bootCnt = 0;
  else
    bootCnt++;
  osal_nv_write( ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt );
#endif

  // Initialize the Extended PAN ID as my own extended address
  ZMacGetReq( ZMacExtAddr, zgExtendedPANID );

#ifndef NONWK
  // Initialize the Pre-Configured Key to the default key
  osal_memcpy( zgPreConfigKey, defaultKey, SEC_KEY_LEN );  // Do NOT Change!!!
#endif // NONWK

  while ( zgItemTable[i].id != 0x00 )
  {
    // Initialize the item
    zgItemInit( zgItemTable[i].id, zgItemTable[i].len, zgItemTable[i].buf, setDefault  );

    // Move on to the next item
    i++;
  }

  // Clear the Config State default
  if ( setDefault )
  {
    zgWriteStartupOptions( ZG_STARTUP_CLEAR, ZCD_STARTOPT_DEFAULT_CONFIG_STATE );
  }

  return ( ZSUCCESS );
}

/*********************************************************************
 * @fn          zgReadStartupOptions
 *
 * @brief       Reads the ZCD_NV_STARTUP_OPTION NV Item.
 *
 * @param       none
 *
 * @return      the ZCD_NV_STARTUP_OPTION NV item
 */
uint8 zgReadStartupOptions( void )
{
  // Default to Use Config State and Use Network State
  uint8 startupOption = 0;

  // This should have been done in ZMain.c, but just in case.
  if ( osal_nv_item_init( ZCD_NV_STARTUP_OPTION,
                              sizeof(startupOption),
                              &startupOption ) == ZSUCCESS )
  {
    // Read saved startup control
    osal_nv_read( ZCD_NV_STARTUP_OPTION,
                  0,
                  sizeof( startupOption ),
                  &startupOption);
  }
  return ( startupOption );
}

/*********************************************************************
 * @fn          zgWriteStartupOptions
 *
 * @brief       Writes bits into the ZCD_NV_STARTUP_OPTION NV Item.
 *
 * @param       action - ZG_STARTUP_SET set bit, ZG_STARTUP_CLEAR to
 *               clear bit. The set bit is an OR operation, and the
 *               clear bit is an AND ~(bitOptions) operation.
 *
 * @param       bitOptions - which bits to perform action on:
 *                      ZCD_STARTOPT_DEFAULT_CONFIG_STATE
 *                      ZCD_STARTOPT_DEFAULT_NETWORK_STATE
 *
 * @return      ZSUCCESS if successful
 */
uint8 zgWriteStartupOptions( uint8 action, uint8 bitOptions )
{
  uint8 status;
  uint8 startupOptions = 0;

  status = osal_nv_read( ZCD_NV_STARTUP_OPTION,
                0,
                sizeof( startupOptions ),
                &startupOptions );

  if ( status == ZSUCCESS )
  {
    if ( action == ZG_STARTUP_SET )
    {
      // Set bits
      startupOptions |= bitOptions;
    }
    else
    {
      // Clear bits
      startupOptions &= ~(bitOptions);
    }

    // Changed?
    status = osal_nv_write( ZCD_NV_STARTUP_OPTION,
                 0,
                 sizeof( startupOptions ),
                 &startupOptions );
  }

  return ( status );
}

/*********************************************************************
*********************************************************************/
