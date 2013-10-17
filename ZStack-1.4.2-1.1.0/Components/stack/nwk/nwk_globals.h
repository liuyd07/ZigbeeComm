#ifndef NWK_GLOBALS_H
#define NWK_GLOBALS_H
/*********************************************************************
    Filename:       nwk_globals.h
    Revised:        $Date: 2007-05-14 17:34:18 -0700 (Mon, 14 May 2007) $
    Revision:       $Revision: 14296 $

    Description:

        User definable Network Parameters.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
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

#include "ZComDef.h"
#include "nwk_bufs.h"
#include "AssocList.h"
#include "BindingTable.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Controls the operational mode of network
#define NWK_MODE_STAR         0         //ÐÇ×´ÍøÂç
#define NWK_MODE_TREE         1         //Ê÷×´ÍøÂç
#define NWK_MODE_MESH         2         //Íø×´ÍøÂç

// Controls the security mode of network
#define SECURITY_RESIDENTIAL  0
#define SECURITY_COMMERCIAL   1

// Controls various stack parameter settings
#define NETWORK_SPECIFIC      0
#define HOME_CONTROLS         1
#define BUILDING_AUTOMATION   2
#define GENERIC_STAR          3
#define GENERIC_TREE          4

#define STACK_PROFILE_ID      HOME_CONTROLS                                        //¶¨ÒåÎªÍø×´ÍøÂç

#if ( STACK_PROFILE_ID == HOME_CONTROLS )
    #define MAX_NODE_DEPTH      5
    #define NWK_MODE            NWK_MODE_MESH             //Íø×´ÍøÂç
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif

#elif ( STACK_PROFILE_ID == GENERIC_STAR )            //ÐÇ×´ÍøÂçÖÐ
    #define MAX_NODE_DEPTH      1
    #define NWK_MODE            NWK_MODE_STAR          //ÐÇ×´ÍøÂç
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif

#elif ( STACK_PROFILE_ID == NETWORK_SPECIFIC )
// define your own stack profile settings
    #define MAX_NODE_DEPTH          5               //ÍøÂçÉî¶È
    #define NWK_MODE            NWK_MODE_MESH      //Íø×´ÍøÂçÍ¨Ñ¶
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif
#endif

// no. of entries in the regular routing table plus
#define MAX_UNRESERVED_RTG_ENTRIES      (MAX_RTG_ENTRIES - 4)

// Status and error codes for extra information
#define NWK_STATUS_PING_RCVD            0x0001
#define NWK_STATUS_ASSOC_CNF            0x0002
#define NWK_STATUS_ED_ADDR              0x0003
#define NWK_STATUS_PARENT_ADDR          0x0004
#define NWK_STATUS_COORD_ADDR           0x0005
#define NWK_STATUS_ROUTER_ADDR          0x0006
#define NWK_STATUS_ORPHAN_RSP           0x0007

#define NWK_ERROR_ASSOC_RSP             0x1001
#define NWK_ERROR_ASSOC_RSP_MF          0x1002
#define NWK_ERROR_ASSOC_CNF_DENIED      0x1003
#define NWK_ERROR_ENERGY_SCAN_FAILED    0x1004

// Maximum number in tables
#if !defined( NWK_MAX_DEVICE_LIST )
  #define NWK_MAX_DEVICE_LIST     20  // ÍøÂçÖÐÉè±¸µÄ×î¶à¸öÊý
                                    // Assoc/Device list.
#endif

// Don't change this value to set the number of devices.  Change
//  NWK_MAX_DEVICE_LIST above
#define NWK_MAX_DEVICES      NWK_MAX_DEVICE_LIST + 1    // One extra space for parent

#if defined (RTR_NWK)
    #define MAX_NEIGHBOR_ENTRIES    8
#else
    #define MAX_NEIGHBOR_ENTRIES    4
#endif  // RTR

#if !defined ( APS_MAX_GROUPS )
  #define APS_MAX_GROUPS  10
#endif

#if !defined ( APSF_DEFAULT_WINDOW_SIZE )
  #define APS_DEFAULT_WINDOW_SIZE        5
#endif

#if !defined ( APSF_DEFAULT_INTERFRAME_DELAY )
  #define APS_DEFAULT_INTERFRAME_DELAY   50
#endif

// Maxiumum number of REFLECTOR address entries
#if defined ( REFLECTOR )
  #define NWK_MAX_REFLECTOR_ENTRIES ( NWK_MAX_BINDING_ENTRIES )
#else
  #define NWK_MAX_REFLECTOR_ENTRIES 0
#endif

#if !defined( MAX_BCAST )
  #define MAX_BCAST 4
#endif

// Maxiumum number of secure partners(Commercial mode only).
// Add 1 for the Trust Center(Coordinator) if it is not the parent.
#define NWK_MAX_SECURE_PARTNERS 1

// Maximum number of addresses managed by the Address Manager
#define NWK_MAX_ADDRESSES (uint16)                          \
                          ( ( NWK_MAX_DEVICES           ) +   \
                            ( NWK_MAX_REFLECTOR_ENTRIES ) +   \
                            ( NWK_MAX_SECURE_PARTNERS   )   )

// Network PAN Coordinator Address
#define NWK_PAN_COORD_ADDR 0x0000

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * NWK GLOBAL VARIABLES
 */

// Variables for MAX data buffer levels
extern CONST byte gNWK_MAX_DATABUFS_WAITING;
extern CONST byte gNWK_MAX_DATABUFS_SCHEDULED;
extern CONST byte gNWK_MAX_DATABUFS_CONFIRMED;
extern CONST byte gNWK_MAX_DATABUFS_TOTAL;

extern CONST byte gNWK_INDIRECT_CNT_RTG_TMR;
extern CONST byte gNWK_INDIRECT_MSG_MAX_PER;
extern CONST byte gNWK_INDIRECT_MSG_MAX_ALL;

extern CONST byte gMAX_NEIGHBOR_ENTRIES;

extern CONST byte gMAX_RTG_ENTRIES;
extern CONST byte gMAX_UNRESERVED_RTG_ENTRIES;
extern CONST byte gMAX_RREQ_ENTRIES;

// Variables for MAX list size
extern CONST uint16 gNWK_MAX_DEVICE_LIST;

extern uint16 *Cskip;
extern byte CskipRtrs[];
extern byte CskipChldrn[];

extern byte gMIN_TREE_LINK_COST;

extern CONST byte defaultKey[];

extern CONST byte gMAX_BCAST;


/*********************************************************************
 * APS GLOBAL VARIABLES
 */

// Variables for Binding Table
extern CONST uint16 gNWK_MAX_BINDING_ENTRIES;
extern CONST byte gMAX_BINDING_CLUSTER_IDS;
extern CONST uint16 gBIND_REC_SIZE;

extern CONST uint8 gAPS_MAX_GROUPS;

extern CONST uint8 apscMaxWindowSize;
extern CONST uint16 gAPS_INTERFRAME_DELAY;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Init Global Variables
 */
extern void nwk_globals_init( void );
extern void NIB_init( void );

extern void nwk_Status( uint16 statusCode, uint16 statusValue );

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* NWK_GLOBALS_H */


