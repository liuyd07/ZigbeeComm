#ifndef ZDCONFIG_H
#define ZDCONFIG_H

/*********************************************************************
    Filename:       ZDConfig.h
    Revised:        $Date: 2006-11-30 12:05:15 -0800 (Thu, 30 Nov 2006) $
    Revision:       $Revision: 12906 $
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

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "AF.h"

/*********************************************************************
 * Enable Features
 */
#if defined ( MT_ZDO_FUNC )
  // All of the ZDO functions are enabled for ZTool use.
  #define ZDO_NWKADDR_REQUEST
  #define ZDO_IEEEADDR_REQUEST
  #define ZDO_MATCH_REQUEST
  #define ZDO_NODEDESC_REQUEST
  #define ZDO_POWERDESC_REQUEST
  #define ZDO_SIMPLEDESC_REQUEST
  #define ZDO_ACTIVEEP_REQUEST
  
  #define ZDO_COMPLEXDESC_REQUEST
  #define ZDO_USERDESC_REQUEST
  #define ZDO_USERDESCSET_REQUEST
  #define ZDO_ENDDEVICEBIND_REQUEST
  #define ZDO_BIND_UNBIND_REQUEST
  #define ZDO_SERVERDISC_REQUEST
  #define ZDO_NETWORKSTART_REQUEST

  #define ZDO_COMPLEXDESC_RESPONSE
  #define ZDO_USERDESC_RESPONSE
  #define ZDO_USERDESCSET_RESPONSE
  #define ZDO_SERVERDISC_RESPONSE
  

#if defined ( MT_ZDO_MGMT )
  #define ZDO_MGMT_NWKDISC_REQUEST
  #define ZDO_MGMT_LQI_REQUEST
  #define ZDO_MGMT_RTG_REQUEST
  #define ZDO_MGMT_BIND_REQUEST
  #define ZDO_MGMT_LEAVE_REQUEST
  #define ZDO_MGMT_JOINDIRECT_REQUEST
  #define ZDO_MGMT_PERMIT_JOIN_REQUEST
  #define ZDO_ENDDEVICE_ANNCE_REQUEST
  #define ZDO_MGMT_NWKDISC_RESPONSE
  #define ZDO_MGMT_LQI_RESPONSE
  #define ZDO_MGMT_RTG_RESPONSE
  #define ZDO_MGMT_BIND_RESPONSE
  #define ZDO_MGMT_LEAVE_RESPONSE
  #define ZDO_MGMT_JOINDIRECT_RESPONSE
  #define ZDO_MGMT_PERMIT_JOIN_RESPONSE
  #define ZDO_ENDDEVICE_ANNCE
#endif

#else // !MT_ZDO_FUNC

  // Normal operation and sample apps only use End Device Bind
  // and Match Request.

  //#define ZDO_NWKADDR_REQUEST
  //#define ZDO_IEEEADDR_REQUEST
  #define ZDO_MATCH_REQUEST
  //#define ZDO_NODEDESC_REQUEST
  //#define ZDO_POWERDESC_REQUEST
  //#define ZDO_SIMPLEDESC_REQUEST
  //#define ZDO_ACTIVEEP_REQUEST
  //#define ZDO_COMPLEXDESC_REQUEST
  //#define ZDO_USERDESC_REQUEST
  //#define ZDO_USERDESCSET_REQUEST
  #define ZDO_ENDDEVICEBIND_REQUEST
  //#define ZDO_BIND_UNBIND_REQUEST
  //#define ZDO_SERVERDISC_REQUEST

  //#define ZDO_BIND_UNBIND_RESPONSE
  //#define ZDO_COMPLEXDESC_RESPONSE
  //#define ZDO_USERDESC_RESPONSE
  //#define ZDO_USERDESCSET_RESPONSE
  //#define ZDO_SERVERDISC_RESPONSE

  //#define ZDO_MGMT_NWKDISC_REQUEST
  //#define ZDO_MGMT_LQI_REQUEST
  //#define ZDO_MGMT_RTG_REQUEST
  //#define ZDO_MGMT_BIND_REQUEST
  //#define ZDO_MGMT_LEAVE_REQUEST
  //#define ZDO_MGMT_JOINDIRECT_REQUEST
  //#define ZDO_MGMT_PERMIT_JOIN_REQUEST
  //#define ZDO_ENDDEVICE_ANNCE_REQUEST
  //#define ZDO_MGMT_NWKDISC_RESPONSE
  //#define ZDO_MGMT_LQI_RESPONSE
  //#define ZDO_MGMT_RTG_RESPONSE
  //#define ZDO_MGMT_BIND_RESPONSE
  //#define ZDO_MGMT_LEAVE_RESPONSE
  //#define ZDO_MGMT_JOINDIRECT_RESPONSE
  //#define ZDO_MGMT_PERMIT_JOIN_RESPONSE
  //#define ZDO_ENDDEVICE_ANNCE

#if defined ( REFLECTOR  )
  // Binding needs this request to do a 64 to 16 bit conversion
  #define ZDO_NWKADDR_REQUEST
  #define ZDO_IEEEADDR_REQUEST
  #define ZDO_BIND_UNBIND_RESPONSE
#endif

#endif  // !MT_ZDO_FUNC


/*********************************************************************
 * Constants
 */

#define MAX_BUFFER_SIZE		        	80		
#define MAX_TRANSFER_SIZE	        	80

#define MAX_ENDPOINTS	            	240

// Node Description Bitfields
#define ZDOLOGICALTYPE_MASK		    	0x07
#define ZDOAPSFLAGS_MASK		      	0x07
#define ZDOFREQUENCYBANDS_MASK    	0x1F
#define ZDOAPSFLAGS_BITLEN	    		3

#define SIMPLE_DESC_DATA_SIZE				7
#define NODE_DESC_DATA_SIZE					10

// Simple Description Bitfields
#define ZDOENDPOINT_BITLEN		      5
#define ZDOENDPOINT_MASK		        0x1F
#define ZDOINTERFACE_MASK	      		0x07
#define ZDOAPPFLAGS_MASK	      		0x0F
#define ZDOAPPDEVVER_MASK		      	0x0F
#define ZDOAPPDEVVER_BITLEN		    	4

/*********************************************************************
 * Attributes
 */

extern NodeDescriptorFormat_t ZDO_Config_Node_Descriptor;
extern NodePowerDescriptorFormat_t ZDO_Config_Power_Descriptor;

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZDCONFIG_H */
