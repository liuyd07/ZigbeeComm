/*********************************************************************
    Filename:       ZDConfig.c
    Revised:        $Date: 2006-08-15 10:35:43 -0700 (Tue, 15 Aug 2006) $
    Revision:       $Revision: 11786 $

    Description:

      This file contains the configuration attributes for the
      Zigbee Device Object.  These are references to Configuration
      items that MUST be defined in ZDApp.c.  The names mustn't
      change.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComdef.h"
#include "AF.h"
#include "ZDObject.h"
#include "ZDConfig.h"
#include "ZDCache.h"

 /*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

NodeDescriptorFormat_t ZDO_Config_Node_Descriptor =
{
#if defined( ZDO_COORDINATOR ) && !defined( SOFT_START )
  NODETYPE_COORDINATOR,
#elif defined (RTR_NWK)
	NODETYPE_ROUTER,
#else
  NODETYPE_DEVICE,          // Logical Type
#endif
  0,                        // User Descriptor Available is set later.
  0,                        // Complex Descriptor Available is set later.
  0,			              		// Reserved
  0,	        	            // NO APS flags
  NODEFREQ_2400,            // Frequency Band
  // MAC Capabilities
#if defined (RTR_NWK)
  (
  #if defined( ZDO_COORDINATOR ) || defined( SOFT_START )
    CAPINFO_ALTPANCOORD |
  #endif
    CAPINFO_DEVICETYPE_FFD |
    CAPINFO_POWER_AC |
    CAPINFO_RCVR_ON_IDLE ),
#else
  CAPINFO_DEVICETYPE_RFD
  #if ( RFD_RCVC_ALWAYS_ON == TRUE)
    | CAPINFO_RCVR_ON_IDLE
  #endif
  ,
#endif
  { 0x00, 0x00 },           // Manfacturer Code - *YOU FILL IN*
  MAX_BUFFER_SIZE,          // Maximum Buffer Size.
  // The maximum transfer size field isn't used and spec says to set to 0.
  {0, 0},
  ( 0
#if defined( ZDO_COORDINATOR ) && ( SECURE != 0 )    
    | PRIM_TRUST_CENTER
#endif      
#if defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 )
    | PRIM_DISC_TABLE
#endif
  )
};

NodePowerDescriptorFormat_t ZDO_Config_Power_Descriptor =
{
#if defined ( RTR_NWK )
  NODECURPWR_RCVR_ALWAYS_ON,
  NODEAVAILPWR_MAINS,       // available power sources
  NODEAVAILPWR_MAINS,       // current power source
  NODEPOWER_LEVEL_100       // Power Level
#else
  // Assume End Device
#if defined ( NWK_AUTO_POLL )
  NODECURPWR_RCVR_AUTO,
#else
  NODECURPWR_RCVR_STIM,
#endif
  NODEAVAILPWR_RECHARGE,    // available power sources
  NODEAVAILPWR_RECHARGE,    // current power source
  NODEPOWER_LEVEL_66        // Power Level
#endif
};

/*********************************************************************
*********************************************************************/


