#ifndef ASSOCLIST_H
#define ASSOCLIST_H
/*********************************************************************
    Filename:       AssocList.h
    Revised:        $Date: 2006-11-14 15:09:20 -0800 (Tue, 14 Nov 2006) $
    Revision:       $Revision: 12708 $

    Description:

        Associated Device List.

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

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define NVINDEX_NOT_FOUND   0xFFFF
#define NVINDEX_THIS_DEVICE 0xFFFE

#define ASSOC_INDEX_NOT_FOUND 0xFFFF

// Bitmap of associated devices status fields
#define DEV_LINK_STATUS     0x01 // link is in-active ?
#define DEV_LINK_REPAIR     0x02 // link repair in progress ?
#define DEV_SEC_INIT_STATUS 0x04 // security init
#define DEV_SEC_AUTH_STATUS 0x08 // security authenticated

// Node Relations
#define PARENT              0
#define CHILD_RFD           1
#define CHILD_RFD_RX_IDLE   2
#define CHILD_FFD           3
#define CHILD_FFD_RX_IDLE   4
#define NEIGHBOR            5
#define OTHER               6
#define NOTUSED             0xFF

/*********************************************************************
 * TYPEDEFS
 */

typedef struct
{
  UINT16 shortAddr;                 // Short address of associated device
  uint16 addrIdx;                   // Index from the address manager
  byte nodeRelation;
  byte devStatus;                   // bitmap of various status values
  byte assocCnt;
  linkInfo_t linkInfo;
} associated_devices_t;

typedef struct
{
  uint16 numRecs;
} nvDeviceListHdr_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
//extern byte _numAssocDev;
#if defined(RTR_NWK)
  extern associated_devices_t AssociatedDevList[];
#endif

/*********************************************************************
 * FUNCTIONS
 */
#if defined(RTR_NWK)

/*
 * Variable initialization
 */
extern void AssocInit( void );

/*
 * Create a new or update a previous association.
 */
extern associated_devices_t *AssocAddNew( uint16 shortAddr, byte *extAddr,
                                                            byte nodeRelation );

/*
 * Count number of devices.
 */
extern uint16 AssocCount( byte startRelation, byte endRelation );

/*
 * Check if the device is a child.
 */
extern byte AssocIsChild( uint16 shortAddr );

/*
 * Check if the device is my parent.
 */
extern byte AssocIsParent( uint16 shortAddr );

/*
 * Search the Device list using shortAddr.
 */
extern associated_devices_t *AssocGetWithShort( uint16 shortAddr );

/*
 * Search the Device list using extended Address.
 */
extern associated_devices_t *AssocGetWithExt( byte *extAddr );

/*
 * Remove a device from the list. Uses the extended address.
 */
extern byte AssocRemove( byte *extAddr );

/*
 * Returns the next inactive child node
 */
extern uint16 AssocGetNextInactiveNode( uint16 shortAddr );

/*
 * Returns the next child node
 */
extern uint16 AssocGetNextChildNode( uint16 shortAddr );

/*
 * Remove all devices from the list and reset it
 */
extern void AssocReset( void );

/*
 * AssocMakeList - Make a list of associate devices
 *  NOTE:  this function returns a dynamically allocated buffer
 *         that MUST be deallocated (osal_mem_free).
 */
extern uint16 *AssocMakeList( byte *pCount );

/*
 * Gets next device that matches the status parameter
 */
extern associated_devices_t *AssocMatchDeviceStatus( uint8 status );

/*
 * Initialize the Assoc Device List NV Item
 */
extern byte AssocInitNV( void );

/*
 * Set Assoc Device list NV Item to defaults
 */
extern void AssocSetDefaultNV( void );

/*
 * Restore the device list (assoc list) from NV
 */
extern byte AssocRestoreFromNV( void );

/*
 * Save the device list (assoc list) to NV
 */
extern void AssocWriteNV( void );

/*
 * Find Nth active device in list
 */
extern associated_devices_t *AssocFindDevice( byte number );

#endif // RTR_NWK

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* ASSOCLIST_H */


