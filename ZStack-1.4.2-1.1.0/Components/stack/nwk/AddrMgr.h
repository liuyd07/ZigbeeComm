#ifndef ADDRMGR_H
#define ADDRMGR_H

/******************************************************************************
    Filename:       AddrMgr.h
    Revised:        $Date: 2007-03-16 17:30:34 -0700 (Fri, 16 Mar 2007) $
    Revision:       $Revision: 13778 $

    Description:

      This file contains the interface to the Address Manager.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/******************************************************************************
 * CONSTANTS
 */
// registration IDs - use with <AddrMgrRegister>
#define ADDRMGR_REG_ASSOC    0x00
#define ADDRMGR_REG_SECURITY 0x01
#define ADDRMGR_REG_BINDING  0x02
#define ADDRMGR_REG_PRIVATE1 0x03

// user IDs - use with <AddrMgrEntry_t>
#define ADDRMGR_USER_DEFAULT  0x00
#define ADDRMGR_USER_ASSOC    0x01
#define ADDRMGR_USER_SECURITY 0x02
#define ADDRMGR_USER_BINDING  0x04
#define ADDRMGR_USER_PRIVATE1 0x08

// update types - use with registered callback <AddrMgrUserCB_t>
#define ADDRMGR_ENTRY_NWKADDR_SET 1
#define ADDRMGR_ENTRY_NWKADDR_DUP 2
#define ADDRMGR_ENTRY_EXTADDR_SET 3

// address manager callback feature enable/disable
#define ADDRMGR_CALLBACK_ENABLED 0

/******************************************************************************
 * TYPEDEFS
 */
// entry data
typedef struct
{
  uint8  user;
  uint16 nwkAddr;
  uint8  extAddr[Z_EXTADDR_LEN];
  uint16 index;
} AddrMgrEntry_t;

// user callback set during registration
typedef void (*AddrMgrUserCB_t)( uint8           update,
                                 AddrMgrEntry_t* newEntry,
                                 AddrMgrEntry_t* oldEntry );

/******************************************************************************
 * PUBLIC FUNCTIONS
 */
/******************************************************************************
 * @fn          AddrMgrInit
 *
 * @brief       Initialize Address Manager.
 *
 * @param       entryTotal - [in] total number of address entries
 *
 * @return      none
 */
extern void AddrMgrInit( uint16 entryTotal );

/******************************************************************************
 * @fn          AddrMgrInitNV
 *
 * @brief       Initialize the address entry data in NV.
 *
 * @param       none
 *
 * @return      uint8 - <osal_nv_item_init> return codes
 */
extern uint8 AddrMgrInitNV( void );

/******************************************************************************
 * @fn          AddrMgrSetDefaultNV
 *
 * @brief       Set default address entry data in NV.
 *
 * @param       none
 *
 * @return      none
 */
extern void AddrMgrSetDefaultNV( void );

/******************************************************************************
 * @fn          AddrMgrRestoreFromNV
 *
 * @brief       Restore the address entry data from NV.
 *
 * @param       none
 *
 * @return      none
 */
extern void AddrMgrRestoreFromNV( void );

/******************************************************************************
 * @fn          AddrMgrWriteNV
 *
 * @brief       Save the address entry data to NV.
 *
 * @param       none
 *
 * @return      none
 */
extern void AddrMgrWriteNV( void );

/******************************************************************************
 * @fn          AddrMgrWriteNVRequest
 *
 * @brief       Stub routine implemented by NHLE. NHLE should call 
 *              <AddrMgrWriteNV> when appropriate. 
 *
 * @param       none
 *
 * @return      none
 */
extern void AddrMgrWriteNVRequest( void );

#if ( ADDRMGR_CALLBACK_ENABLED == 1 )
/******************************************************************************
 * @fn          AddrMgrRegister
 *
 * @brief       Register as a user of the Address Manager.
 *
 * @param       reg - [in] register ID
 * @param       cb  - [in] user callback
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrRegister( uint8 reg, AddrMgrUserCB_t cb );

#endif //ADDRMGR_CALLBACK_ENABLED

/******************************************************************************
 * @fn          AddrMgrExtAddrSet
 *
 * @brief       Set destination address to source address or empty{0xFF}.
 *
 * @param       dstExtAddr - [in] destination EXT address
 *              srcExtAddr - [in] source EXT address
 *
 * @return      none
 */
extern void AddrMgrExtAddrSet( uint8* dstExtAddr, uint8* srcExtAddr );

/******************************************************************************
 * @fn          AddrMgrExtAddrValid
 *
 * @brief       Check if EXT address is valid - not NULL, not empty{0xFF}.
 *
 * @param       extAddr - [in] EXT address
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrExtAddrValid( uint8* extAddr );

/******************************************************************************
 * @fn          AddrMgrExtAddrEqual
 *
 * @brief       Compare two EXT addresses.
 *
 * @param       extAddr1 - [in] EXT address 1
 *              extAddr2 - [in] EXT address 2
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrExtAddrEqual( uint8* extAddr1, uint8* extAddr2 );

/******************************************************************************
 * @fn          AddrMgrExtAddrLookup
 *
 * @brief       Lookup EXT address using the NWK address.
 *
 * @param       nwkAddr - [in] NWK address
 *              extAddr - [out] EXT address
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrExtAddrLookup( uint16 nwkAddr, uint8* extAddr );

/******************************************************************************
 * @fn          AddrMgrNwkAddrLookup
 *
 * @brief       Lookup NWK address using the EXT address.
 *
 * @param       extAddr - [in] EXT address
 *              nwkAddr - [out] NWK address
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrNwkAddrLookup( uint8* extAddr, uint16* nwkAddr );

/******************************************************************************
 * @fn          AddrMgrEntryRelease
 *
 * @brief       Release a user reference from an entry in the Address Manager.
 *
 * @param       entry
 *                ::user  - [in] user ID
 *                ::index - [in] index of data
 *                ::nwkAddr - not used
 *                ::extAddr - not used
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrEntryRelease( AddrMgrEntry_t* entry );

/******************************************************************************
 * @fn          AddrMgrEntryAddRef
 *
 * @brief       Add a user reference to an entry in the Address Manager.
 *
 * @param       entry
 *                ::user  - [in] user ID
 *                ::index - [in] index of data
 *                ::nwkAddr - not used
 *                ::extAddr - not used
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrEntryAddRef( AddrMgrEntry_t* entry );

/******************************************************************************
 * @fn          AddrMgrEntryLookupNwk
 *
 * @brief       Lookup entry based on NWK address.
 *
 * @param       entry
 *                ::user    - [in] user ID
 *                ::nwkAddr - [in] NWK address
 *                ::extAddr - [out] EXT address
 *                ::index   - [out] index of data
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrEntryLookupNwk( AddrMgrEntry_t* entry );

/******************************************************************************
 * @fn          AddrMgrEntryLookupExt
 *
 * @brief       Lookup entry based on EXT address.
 *
 * @param       entry
 *                ::user    - [in] user ID
 *                ::extAddr - [in] EXT address
 *                ::nwkAddr - [out] NWK address
 *                ::index   - [out] index of data
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrEntryLookupExt( AddrMgrEntry_t* entry );

/******************************************************************************
 * @fn          AddrMgrEntryGet
 *
 * @brief       Get NWK address and EXT address based on index.
 *
 * @param       entry
 *                ::user    - [in] user ID
 *                ::index   - [in] index of data
 *                ::nwkAddr - [out] NWK address
 *                ::extAddr - [out] EXT address
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 AddrMgrEntryGet( AddrMgrEntry_t* entry );

/******************************************************************************
 * @fn          AddrMgrEntryUpdate
 *
 * @brief       Update an entry into the Address Manager.
 *
 * @param       entry
 *                ::user    - [in] user ID
 *                ::nwkAddr - [in] NWK address
 *                ::extAddr - [in] EXT address
 *                ::index   - [out] index of data
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
uint8 AddrMgrEntryUpdate( AddrMgrEntry_t* entry );

/******************************************************************************
******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ADDRMGR_H */