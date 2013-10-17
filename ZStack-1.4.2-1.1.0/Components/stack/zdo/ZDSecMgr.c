/******************************************************************************
    Filename:       ZDSecMgr.c
    Revised:        $Date: 2007-03-16 17:30:34 -0700 (Fri, 16 Mar 2007) $
    Revision:       $Revision: 13778 $

    Description:

      The ZigBee Device Security Manager.

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
#include "ZComdef.h"
#include "osal.h"
#include "ZGlobals.h"
#include "ssp.h"
#include "nwk_globals.h"
#include "nwk.h"
#include "NLMEDE.h"
#include "AddrMgr.h"
#include "AssocList.h"
#include "APSMEDE.h"
#include "AF.h"
#include "ZDConfig.h"
#include "ZDApp.h"
#include "ZDSecMgr.h"

/******************************************************************************
 * CONSTANTS
 */
// maximum number of devices managed by this Security Manager
#define ZDSECMGR_DEVICE_MAX 3

// total number of preconfigured devices (EXT address, MASTER key)
#define ZDSECMGR_PRECONFIG_MAX ZDSECMGR_DEVICE_MAX

// maximum number of MASTER keys this device may hold
#define ZDSECMGR_MASTERKEY_MAX ZDSECMGR_DEVICE_MAX

// maximum number of LINK keys this device may store
#define ZDSECMGR_ENTRY_MAX ZDSECMGR_DEVICE_MAX

// total number of devices under control - authentication, SKKE, etc.
#define ZDSECMGR_CTRL_MAX ZDSECMGR_DEVICE_MAX

// total number of restricted devices
#define ZDSECMGR_RESTRICTED_DEVICES 2

#define ZDSECMGR_CTRL_NONE       0
#define ZDSECMGR_CTRL_INIT       1
#define ZDSECMGR_CTRL_TK_MASTER  2
#define ZDSECMGR_CTRL_SKKE_INIT  3
#define ZDSECMGR_CTRL_SKKE_WAIT  4
#define ZDSECMGR_CTRL_SKKE_DONE  5
#define ZDSECMGR_CTRL_TK_NWK     6

#define ZDSECMGR_CTRL_BASE_CNTR      1
#define ZDSECMGR_CTRL_SKKE_INIT_CNTR 10
#define ZDSECMGR_CTRL_SKKE_WAIT_CNTR 100
#define ZDSECMGR_CTRL_TK_NWK_CNTR    10

// set SKKE slot maximum
#define ZDSECMGR_SKKE_SLOT_MAX 1

// APSME Stub Implementations
#define ZDSecMgrMasterKeyGet   APSME_MasterKeyGet
#define ZDSecMgrLinkKeySet     APSME_LinkKeySet
#define ZDSecMgrLinkKeyDataGet APSME_LinkKeyDataGet
#define ZDSecMgrKeyFwdToChild  APSME_KeyFwdToChild

/******************************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint8 extAddr[Z_EXTADDR_LEN];
  uint8 key[SEC_KEY_LEN];
} ZDSecMgrPreConfigData_t;

typedef struct
{
  uint16 ami;
  uint8  key[SEC_KEY_LEN];
} ZDSecMgrMasterKeyData_t;

//should match APSME_LinkKeyData_t;
typedef struct
{
  uint8               key[SEC_KEY_LEN];
  APSME_LinkKeyData_t apsmelkd;
} ZDSecMgrLinkKeyData_t;

typedef struct
{
  uint16                ami;
  ZDSecMgrLinkKeyData_t lkd;
} ZDSecMgrEntry_t;

typedef struct
{
  ZDSecMgrEntry_t* entry;
  uint16           parentAddr;
  uint8            secure;
  uint8            state;
  uint8            cntr;
  //uint8          next;
} ZDSecMgrCtrl_t;

typedef struct
{
  uint16          nwkAddr;
  uint8*          extAddr;
  uint16          parentAddr;
  uint8           secure;
  ZDSecMgrCtrl_t* ctrl;
} ZDSecMgrDevice_t;

/******************************************************************************
 * LOCAL VARIABLES
 */
#if defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )
uint8 ZDSecMgrRestrictedDevices[ZDSECMGR_RESTRICTED_DEVICES][Z_EXTADDR_LEN] =
{
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};
#endif // defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )

#if defined ( ZDSECMGR_COMMERCIAL )
#if ( ZDSECMGR_PRECONFIG_MAX != 0 )
const ZDSecMgrPreConfigData_t ZDSecMgrPreConfigData[ZDSECMGR_PRECONFIG_MAX] =
{
  //---------------------------------------------------------------------------
  // DEVICE A
  //---------------------------------------------------------------------------
  {
    // extAddr
    {0x7C,0x01,0x12,0x13,0x14,0x15,0x16,0x17},

    // key
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
  },
  //---------------------------------------------------------------------------
  // DEVICE B
  //---------------------------------------------------------------------------
  {
    // extAddr
    {0x84,0x03,0x00,0x00,0x00,0x4B,0x12,0x00},

    // key
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
  },
  //---------------------------------------------------------------------------
  // DEVICE C
  //---------------------------------------------------------------------------
  {
    // extAddr
    {0x3E,0x01,0x12,0x13,0x14,0x15,0x16,0x17},

    // key
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
  },
};
#endif // ( ZDSECMGR_PRECONFIG_MAX != 0 )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
ZDSecMgrMasterKeyData_t* ZDSecMgrMasterKeyData = NULL;
ZDSecMgrEntry_t*         ZDSecMgrEntries       = NULL;
ZDSecMgrCtrl_t*          ZDSecMgrCtrlData      = NULL;
void ZDSecMgrAddrMgrUpdate( uint16 ami, uint16 nwkAddr );
void ZDSecMgrAddrMgrCB( uint8 update, AddrMgrEntry_t* newEntry, AddrMgrEntry_t* oldEntry );
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )
uint8 ZDSecMgrPermitJoiningEnabled;
uint8 ZDSecMgrPermitJoiningTimed;
#endif // defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )

#if defined ( ZDSECMGR_SECURE )
/******************************************************************************
 * PRIVATE FUNCTIONS
 *
 *   ZDSecMgrMasterKeyInit
 *   ZDSecMgrExtAddrLookup
 *   ZDSecMgrExtAddrStore
 *   ZDSecMgrMasterKeyLookup
 *   ZDSecMgrMasterKeyStore
 *   ZDSecMgrEntryInit
 *   ZDSecMgrEntryLookup
 *   ZDSecMgrEntryLookupAMI
 *   ZDSecMgrEntryLookupExt
 *   ZDSecMgrEntryFree
 *   ZDSecMgrEntryNew
 *   ZDSecMgrCtrlInit
 *   ZDSecMgrCtrlRelease
 *   ZDSecMgrCtrlLookup
 *   ZDSecMgrCtrlSet
 *   ZDSecMgrCtrlAdd
 *   ZDSecMgrCtrlTerm
 *   ZDSecMgrCtrlReset
 *   ZDSecMgrMasterKeyLoad
 *   ZDSecMgrAppKeyGet
 *   ZDSecMgrAppKeyReq
 *   ZDSecMgrEstablishKey
 *   ZDSecMgrSendMasterKey
 *   ZDSecMgrSendNwkKey
 *   ZDSecMgrDeviceEntryRemove
 *   ZDSecMgrDeviceEntryAdd
 *   ZDSecMgrDeviceCtrlHandler
 *   ZDSecMgrDeviceCtrlSetup
 *   ZDSecMgrDeviceCtrlUpdate
 *   ZDSecMgrDeviceRemove
 *   ZDSecMgrDeviceValidateSKKE
 *   ZDSecMgrDeviceValidateRM
 *   ZDSecMgrDeviceValidateCM
 *   ZDSecMgrDeviceValidate
 *   ZDSecMgrDeviceJoin
 *   ZDSecMgrDeviceJoinDirect
 *   ZDSecMgrDeviceJoinFwd
 *   ZDSecMgrDeviceNew
 *   ZDSecMgrAssocDeviceAuth
 */
//-----------------------------------------------------------------------------
// master key data
//-----------------------------------------------------------------------------
void ZDSecMgrMasterKeyInit( void );

//-----------------------------------------------------------------------------
// EXT address management
//-----------------------------------------------------------------------------
ZStatus_t ZDSecMgrExtAddrLookup( uint8* extAddr, uint16* ami );
ZStatus_t ZDSecMgrExtAddrStore( uint8* extAddr, uint16* ami );

//-----------------------------------------------------------------------------
// MASTER key data
//-----------------------------------------------------------------------------
ZStatus_t ZDSecMgrMasterKeyLookup( uint16 ami, uint8** key );
ZStatus_t ZDSecMgrMasterKeyStore( uint16 ami, uint8* key );

//-----------------------------------------------------------------------------
// entry data
//-----------------------------------------------------------------------------
void ZDSecMgrEntryInit( void );
ZStatus_t ZDSecMgrEntryLookup( uint16 nwkAddr, ZDSecMgrEntry_t** entry );
ZStatus_t ZDSecMgrEntryLookupAMI( uint16 ami, ZDSecMgrEntry_t** entry );
ZStatus_t ZDSecMgrEntryLookupExt( uint8* extAddr, ZDSecMgrEntry_t** entry );
void ZDSecMgrEntryFree( ZDSecMgrEntry_t* entry );
ZStatus_t ZDSecMgrEntryNew( ZDSecMgrEntry_t** entry );

//-----------------------------------------------------------------------------
// control data
//-----------------------------------------------------------------------------
void ZDSecMgrCtrlInit( void );
void ZDSecMgrCtrlRelease( ZDSecMgrCtrl_t* ctrl );
void ZDSecMgrCtrlLookup( ZDSecMgrEntry_t* entry, ZDSecMgrCtrl_t** ctrl );
void ZDSecMgrCtrlSet( ZDSecMgrDevice_t* device,
                      ZDSecMgrEntry_t*  entry,
                      ZDSecMgrCtrl_t*   ctrl );
ZStatus_t ZDSecMgrCtrlAdd( ZDSecMgrDevice_t* device, ZDSecMgrEntry_t*  entry );
void ZDSecMgrCtrlTerm( ZDSecMgrEntry_t* entry );
ZStatus_t ZDSecMgrCtrlReset( ZDSecMgrDevice_t* device,
                             ZDSecMgrEntry_t*  entry );

//-----------------------------------------------------------------------------
// key support
//-----------------------------------------------------------------------------
void ZDSecMgrMasterKeyLoad( uint16 nwkAddr, uint8* extAddr, uint8* key );
ZStatus_t ZDSecMgrAppKeyGet( uint16  initNwkAddr,
                             uint8*  initExtAddr,
                             uint16  partNwkAddr,
                             uint8*  partExtAddr,
                             uint8** key,
                             uint8*  keyType );
void ZDSecMgrAppKeyReq( ZDO_RequestKeyInd_t* ind );
ZStatus_t ZDSecMgrEstablishKey( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrSendMasterKey( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrSendNwkKey( ZDSecMgrDevice_t* device );

//-----------------------------------------------------------------------------
// device entry
//-----------------------------------------------------------------------------
void ZDSecMgrDeviceEntryRemove( ZDSecMgrEntry_t* entry );
ZStatus_t ZDSecMgrDeviceEntryAdd( ZDSecMgrDevice_t* device, uint16 ami );

//-----------------------------------------------------------------------------
// device control
//-----------------------------------------------------------------------------
void ZDSecMgrDeviceCtrlHandler( ZDSecMgrDevice_t* device );
void ZDSecMgrDeviceCtrlSetup( ZDSecMgrDevice_t* device );
void ZDSecMgrDeviceCtrlUpdate( uint8* extAddr, uint8 state );

//-----------------------------------------------------------------------------
// device management
//-----------------------------------------------------------------------------
void ZDSecMgrDeviceRemove( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceValidateSKKE( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceValidateRM( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceValidateCM( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceValidate( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceJoin( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceJoinDirect( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceJoinFwd( ZDSecMgrDevice_t* device );
ZStatus_t ZDSecMgrDeviceNew( ZDSecMgrDevice_t* device );

//-----------------------------------------------------------------------------
// association management
//-----------------------------------------------------------------------------
void ZDSecMgrAssocDeviceAuth( associated_devices_t* assoc );

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrMasterKeyInit                     ]
 *
 * @brief       Initialize master key data.
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrMasterKeyInit( void )
{
  uint16         index;
  uint16         size;
  AddrMgrEntry_t entry;


  // allocate MASTER key data
  size = (short)( sizeof(ZDSecMgrMasterKeyData_t) * ZDSECMGR_MASTERKEY_MAX );

  ZDSecMgrMasterKeyData = osal_mem_alloc( size );

  // initialize MASTER key data
  if ( ZDSecMgrMasterKeyData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_MASTERKEY_MAX; index++ )
    {
      ZDSecMgrMasterKeyData[index].ami = INVALID_NODE_ADDR;
    }

    // check if preconfigured keys are enabled
    //-------------------------------------------------------------------------
    #if ( ZDSECMGR_PRECONFIG_MAX != 0 )
    //-------------------------------------------------------------------------
    if ( zgPreConfigKeys == TRUE )
    {
      // sync configured data
      entry.user = ADDRMGR_USER_SECURITY;

      for ( index = 0; index < ZDSECMGR_PRECONFIG_MAX; index++ )
      {
        // check for Address Manager entry
        AddrMgrExtAddrSet( entry.extAddr,
                           (uint8*)ZDSecMgrPreConfigData[index].extAddr );

        if ( AddrMgrEntryLookupExt( &entry ) != TRUE )
        {
          // update Address Manager
          AddrMgrEntryUpdate( &entry );
        }

        if ( entry.index != INVALID_NODE_ADDR )
        {
          // sync MASTER keys with Address Manager index
          ZDSecMgrMasterKeyData[index].ami = entry.index;

          osal_cpyExtAddr( ZDSecMgrMasterKeyData[index].key,
                           (void*)ZDSecMgrPreConfigData[index].key );
        }
      }
    }
    //-------------------------------------------------------------------------
    #endif // ( ZDSECMGR_PRECONFIG_MAX != 0 )
    //-------------------------------------------------------------------------
  }
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrExtAddrLookup
 *
 * @brief       Lookup index for specified EXT address.
 *
 * @param       extAddr - [in] EXT address
 * @param       ami     - [out] Address Manager index
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrExtAddrLookup( uint8* extAddr, uint16* ami )
{
  ZStatus_t      status;
  AddrMgrEntry_t entry;


  // lookup entry
  entry.user = ADDRMGR_USER_SECURITY;
  AddrMgrExtAddrSet( entry.extAddr, extAddr );

  if ( AddrMgrEntryLookupExt( &entry ) == TRUE )
  {
    // return successful results
    *ami   = entry.index;
    status = ZSuccess;
  }
  else
  {
    // return failed results
    *ami   = entry.index;
    status = ZNwkUnknownDevice;
  }

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrExtAddrStore
 *
 * @brief       Store EXT address.
 *
 * @param       extAddr - [in] EXT address
 * @param       ami     - [out] Address Manager index
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrExtAddrStore( uint8* extAddr, uint16* ami )
{
  ZStatus_t      status;
  AddrMgrEntry_t entry;


  // add entry
  entry.user    = ADDRMGR_USER_SECURITY;
  entry.nwkAddr = INVALID_NODE_ADDR;
  AddrMgrExtAddrSet( entry.extAddr, extAddr );

  if ( AddrMgrEntryUpdate( &entry ) == TRUE )
  {
    // return successful results
    *ami   = entry.index;
    status = ZSuccess;
  }
  else
  {
    // return failed results
    *ami   = entry.index;
    status = ZNwkUnknownDevice;
  }

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrMasterKeyLookup
 *
 * @brief       Lookup MASTER key for specified address index.
 *
 * @param       ami - [in] Address Manager index
 * @param       key - [out] valid MASTER key
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrMasterKeyLookup( uint16 ami, uint8** key )
{
  ZStatus_t status;
  uint16    index;


  // initialize results
  *key   = NULL;
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrMasterKeyData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_MASTERKEY_MAX ; index++ )
    {
      if ( ZDSecMgrMasterKeyData[index].ami == ami )
      {
        // return successful results
        *key   = ZDSecMgrMasterKeyData[index].key;
        status = ZSuccess;

        // break from loop
        index  = ZDSECMGR_MASTERKEY_MAX;
      }
    }
  }

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrMasterKeyStore
 *
 * @brief       Store MASTER key for specified address index.
 *
 * @param       ami - [in] Address Manager index
 * @param       key - [in] valid key to store
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrMasterKeyStore( uint16 ami, uint8* key )
{
  ZStatus_t status;
  uint16    index;
  uint8*    entry;


  // initialize results
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrMasterKeyData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_MASTERKEY_MAX ; index++ )
    {
      if ( ZDSecMgrMasterKeyData[index].ami == INVALID_NODE_ADDR )
      {
        // store EXT address index
        ZDSecMgrMasterKeyData[index].ami = ami;

        entry = ZDSecMgrMasterKeyData[index].key;

        if ( key != NULL )
        {
          osal_cpyExtAddr( entry, key );
        }
        else
        {
          osal_memset( entry, 0, SEC_KEY_LEN );
        }

        // return successful results
        status = ZSuccess;

        // break from loop
        index  = ZDSECMGR_MASTERKEY_MAX;
      }
    }
  }

  return status;
}
#endif // !defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEntryInit
 *
 * @brief       Initialize entry sub module
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrEntryInit( void )
{
  uint16 size;
  uint16 index;


  // allocate entry data
  size = (short)( sizeof(ZDSecMgrEntry_t) * ZDSECMGR_ENTRY_MAX );

  ZDSecMgrEntries = osal_mem_alloc( size );

  // initialize data
  if ( ZDSecMgrEntries != NULL )
  {
    for( index = 0; index < ZDSECMGR_ENTRY_MAX; index++ )
    {
      ZDSecMgrEntries[index].ami = INVALID_NODE_ADDR;
    }
  }
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEntryLookup
 *
 * @brief       Lookup entry index using specified NWK address.
 *
 * @param       nwkAddr - [in] NWK address
 * @param       entry   - [out] valid entry
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEntryLookup( uint16 nwkAddr, ZDSecMgrEntry_t** entry )
{
  ZStatus_t      status;
  uint16         index;
  AddrMgrEntry_t addrMgrEntry;


  // initialize results
  *entry = NULL;
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrEntries != NULL )
  {
    addrMgrEntry.user    = ADDRMGR_USER_SECURITY;
    addrMgrEntry.nwkAddr = nwkAddr;

    if ( AddrMgrEntryLookupNwk( &addrMgrEntry ) == TRUE )
    {
      for ( index = 0; index < ZDSECMGR_ENTRY_MAX ; index++ )
      {
        if ( addrMgrEntry.index == ZDSecMgrEntries[index].ami )
        {
          // return successful results
          *entry = &ZDSecMgrEntries[index];
          status = ZSuccess;

          // break from loop
          index = ZDSECMGR_ENTRY_MAX;
        }
      }
    }
  }

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEntryLookupAMI
 *
 * @brief       Lookup entry using specified address index
 *
 * @param       ami   - [in] Address Manager index
 * @param       entry - [out] valid entry
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEntryLookupAMI( uint16 ami, ZDSecMgrEntry_t** entry )
{
  ZStatus_t status;
  uint16    index;


  // initialize results
  *entry = NULL;
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrEntries != NULL )
  {
    for ( index = 0; index < ZDSECMGR_ENTRY_MAX ; index++ )
    {
      if ( ZDSecMgrEntries[index].ami == ami )
      {
        // return successful results
        *entry = &ZDSecMgrEntries[index];
        status = ZSuccess;

        // break from loop
        index = ZDSECMGR_ENTRY_MAX;
      }
    }
  }

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEntryLookupExt
 *
 * @brief       Lookup entry index using specified EXT address.
 *
 * @param       extAddr - [in] EXT address
 * @param       entry   - [out] valid entry
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEntryLookupExt( uint8* extAddr, ZDSecMgrEntry_t** entry )
{
  ZStatus_t status;
  uint16    ami;


  // initialize results
  *entry = NULL;
  status = ZNwkUnknownDevice;

  // lookup address index
  if ( ZDSecMgrExtAddrLookup( extAddr, &ami ) == ZSuccess )
  {
    status = ZDSecMgrEntryLookupAMI( ami, entry );
  }

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEntryFree
 *
 * @brief       Free entry.
 *
 * @param       entry - [in] valid entry
 *
 * @return      ZStatus_t
 */
void ZDSecMgrEntryFree( ZDSecMgrEntry_t* entry )
{
  entry->ami = INVALID_NODE_ADDR;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEntryNew
 *
 * @brief       Get a new entry.
 *
 * @param       entry - [out] valid entry
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEntryNew( ZDSecMgrEntry_t** entry )
{
  ZStatus_t status;
  uint16    index;


  // initialize results
  *entry = NULL;
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrEntries != NULL )
  {
    // find available entry
    for ( index = 0; index < ZDSECMGR_ENTRY_MAX ; index++ )
    {
      if ( ZDSecMgrEntries[index].ami == INVALID_NODE_ADDR )
      {
        // return successful result
        *entry = &ZDSecMgrEntries[index];
        status = ZSuccess;

        // break from loop
        index = ZDSECMGR_ENTRY_MAX;
      }
    }
  }

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrCtrlInit
 *
 * @brief       Initialize control sub module
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrCtrlInit( void )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  uint16 size;
  uint16 index;


  // allocate entry data
  size = (short)( sizeof(ZDSecMgrCtrl_t) * ZDSECMGR_CTRL_MAX );

  ZDSecMgrCtrlData = osal_mem_alloc( size );

  // initialize data
  if ( ZDSecMgrCtrlData != NULL )
  {
    for( index = 0; index < ZDSECMGR_CTRL_MAX; index++ )
    {
      ZDSecMgrCtrlData[index].state = ZDSECMGR_CTRL_NONE;
    }
  }
  //---------------------------------------------------------------------------
  #endif // defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrCtrlRelease
 *
 * @brief       Release control data.
 *
 * @param       ctrl - [in] valid control data
 *
 * @return      none
 */
void ZDSecMgrCtrlRelease( ZDSecMgrCtrl_t* ctrl )
{
  // should always be enough entry control data
  ctrl->state = ZDSECMGR_CTRL_NONE;
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrCtrlLookup
 *
 * @brief       Lookup control data.
 *
 * @param       entry - [in] valid entry data
 * @param       ctrl  - [out] control data - NULL if not found
 *
 * @return      none
 */
void ZDSecMgrCtrlLookup( ZDSecMgrEntry_t* entry, ZDSecMgrCtrl_t** ctrl )
{
  uint16 index;


  // initialize search results
  *ctrl = NULL;

  // verify data is available
  if ( ZDSecMgrCtrlData != NULL )
  {
    for ( index = 0; index < ZDSECMGR_CTRL_MAX; index++ )
    {
      // make sure control data is in use
      if ( ZDSecMgrCtrlData[index].state != ZDSECMGR_CTRL_NONE )
      {
        // check for entry match
        if ( ZDSecMgrCtrlData[index].entry == entry )
        {
          // return this control data
          *ctrl = &ZDSecMgrCtrlData[index];

          // break from loop
          index = ZDSECMGR_CTRL_MAX;
        }
      }
    }
  }
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_COMMERCIAL )


#if defined ( ZDSECMGR_COMMERCIAL )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrCtrlSet
 *
 * @brief       Set control data.
 *
 * @param       device - [in] valid device data
 * @param       entry  - [in] valid entry data
 * @param       ctrl   - [in] valid control data
 *
 * @return      none
 */
void ZDSecMgrCtrlSet( ZDSecMgrDevice_t* device,
                      ZDSecMgrEntry_t*  entry,
                      ZDSecMgrCtrl_t*   ctrl )
{
  // set control date
  ctrl->parentAddr = device->parentAddr;
  ctrl->secure     = device->secure;
  ctrl->entry      = entry;
  ctrl->state      = ZDSECMGR_CTRL_INIT;
  ctrl->cntr       = 0;

  // set device pointer
  device->ctrl = ctrl;
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrCtrlAdd
 *
 * @brief       Add control data.
 *
 * @param       device - [in] valid device data
 * @param       entry  - [in] valid entry data
 *
 * @return      none
 */
ZStatus_t ZDSecMgrCtrlAdd( ZDSecMgrDevice_t* device, ZDSecMgrEntry_t*  entry )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  ZStatus_t status;
  uint16    index;


  // initialize results
  status = ZNwkUnknownDevice;

  // verify data is available
  if ( ZDSecMgrCtrlData != NULL )
  {
    // look for an empty slot
    for ( index = 0; index < ZDSECMGR_CTRL_MAX; index++ )
    {
      if ( ZDSecMgrCtrlData[index].state == ZDSECMGR_CTRL_NONE )
      {
        // return successful results
        ZDSecMgrCtrlSet( device, entry, &ZDSecMgrCtrlData[index] );

        status = ZSuccess;

        // break from loop
        index = ZDSECMGR_CTRL_MAX;
      }
    }
  }

  return status;
  //---------------------------------------------------------------------------
  #else // !defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  return ZSuccess;
  //---------------------------------------------------------------------------
  #endif // !defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrCtrlTerm
 *
 * @brief       Terminate device control.
 *
 * @param       entry - [in] valid entry data
 *
 * @return      none
 */
void ZDSecMgrCtrlTerm( ZDSecMgrEntry_t* entry )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  ZDSecMgrCtrl_t* ctrl;


  // remove device from control data
  ZDSecMgrCtrlLookup ( entry, &ctrl );

  if ( ctrl != NULL )
  {
    ZDSecMgrCtrlRelease ( ctrl );
  }
  //---------------------------------------------------------------------------
  #endif // defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrCtrlReset
 *
 * @brief       Reset control data.
 *
 * @param       device - [in] valid device data
 * @param       entry  - [in] valid entry data
 *
 * @return      none
 */
ZStatus_t ZDSecMgrCtrlReset( ZDSecMgrDevice_t* device, ZDSecMgrEntry_t* entry )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  ZStatus_t       status;
  ZDSecMgrCtrl_t* ctrl;


  // initialize results
  status = ZNwkUnknownDevice;

  // look for a match for the entry
  ZDSecMgrCtrlLookup( entry, &ctrl );

  if ( ctrl != NULL )
  {
    ZDSecMgrCtrlSet( device, entry, ctrl );

    status = ZSuccess;
  }
  else
  {
    status = ZDSecMgrCtrlAdd( device, entry );
  }

  return status;
  //---------------------------------------------------------------------------
  #else // !defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  return ZSuccess;
  //---------------------------------------------------------------------------
  #endif // !defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrMasterKeyLoad
 *
 * @brief       Load the MASTER key for device with specified EXT
 *              address.
 *
 * @param       nwkAddr - [in] NWK address of Trust Center
 * @param       extAddr - [in] EXT address of Trust Center
 * @param       key     - [in] MASTER key shared with Trust Center
 *
 * @return      none
 */
void ZDSecMgrMasterKeyLoad( uint16 nwkAddr, uint8* extAddr, uint8* key )
{
  AddrMgrEntry_t addr;
  uint8*         loaded;


  // check if Trust Center address is configured and correct
  // check if MASTER key has already been sent

  // add address data
  addr.user    = ADDRMGR_USER_SECURITY;
  addr.nwkAddr = nwkAddr;
  AddrMgrExtAddrSet( addr.extAddr, extAddr );

  if ( AddrMgrEntryUpdate( &addr ) == TRUE )
  {
    if ( ZDSecMgrMasterKeyLookup( addr.index, &loaded ) != ZSuccess )
    {
      ZDSecMgrMasterKeyStore( addr.index, key );
    }
  }
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrAppKeyGet
 *
 * @brief       get an APP key - option APP(MASTER or LINK) key
 *
 * @param       initNwkAddr - [in] NWK address of initiator device
 * @param       initExtAddr - [in] EXT address of initiator device
 * @param       partNwkAddr - [in] NWK address of partner device
 * @param       partExtAddr - [in] EXT address of partner device
 * @param       key         - [out] APP(MASTER or LINK) key
 * @param       keyType     - [out] APP(MASTER or LINK) key type
 *
 * @return      pointer to MASTER key
 */
ZStatus_t ZDSecMgrAppKeyGet( uint16  initNwkAddr,
                             uint8*  initExtAddr,
                             uint16  partNwkAddr,
                             uint8*  partExtAddr,
                             uint8** key,
                             uint8*  keyType )
{
  //---------------------------------------------------------------------------
  // note:
  // should use a robust mechanism to generate keys, for example
  // combine EXT addresses and call a hash function
  //---------------------------------------------------------------------------
  osal_memset( *key, 0, SEC_KEY_LEN );

  *keyType = KEY_TYPE_APP_LINK;
  //or       KEY_TYPE_APP_MASTER;

  return ZSuccess;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrAppKeyReq
 *
 * @brief       Process request for APP key between two devices.
 *
 * @param       device - [in] ZDO_RequestKeyInd_t, request info
 *
 * @return      none
 */
void ZDSecMgrAppKeyReq( ZDO_RequestKeyInd_t* ind )
{
  APSME_TransportKeyReq_t req;
  uint8                   initExtAddr[Z_EXTADDR_LEN];
  uint16                  partNwkAddr;
  uint8                   key[SEC_KEY_LEN];


  // validate initiator and partner
  if ( ( APSME_LookupNwkAddr( ind->partExtAddr, &partNwkAddr ) == TRUE ) &&
       ( APSME_LookupExtAddr( ind->srcAddr, initExtAddr ) == TRUE      )   )
  {
    // point the key to some memory
    req.key = key;

    // get an APP key - option APP (MASTER or LINK) key
    if ( ZDSecMgrAppKeyGet( ind->srcAddr,
                            initExtAddr,
                            partNwkAddr,
                            ind->partExtAddr,
                            &req.key,
                            &req.keyType ) == ZSuccess )
    {
      // always secure
      req.secure = TRUE;

      // send key to initiator device
      req.dstAddr   = ind->srcAddr;
      req.extAddr   = ind->partExtAddr;
      req.initiator = TRUE;
      APSME_TransportKeyReq( &req );

      // send key to partner device
      req.dstAddr   = partNwkAddr;
      req.extAddr   = initExtAddr;
      req.initiator = FALSE;
      APSME_TransportKeyReq( &req );
    }
  }
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEstablishKey
 *
 * @brief       Start SKKE with device joining network.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrEstablishKey( ZDSecMgrDevice_t* device )
{
  ZStatus_t               status;
  APSME_EstablishKeyReq_t req;


  req.respExtAddr = device->extAddr;
  req.method      = APSME_SKKE_METHOD;

  if ( device->parentAddr == NLME_GetShortAddr() )
  {
    req.dstAddr = device->nwkAddr;
    req.secure  = FALSE;
  }
  else
  {
    req.dstAddr = device->parentAddr;
    req.secure  = TRUE;
  }

  status = APSME_EstablishKeyReq( &req );

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrSendMasterKey
 *
 * @brief       Send MASTER key to device joining network.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrSendMasterKey( ZDSecMgrDevice_t* device )
{
  ZStatus_t               status;
  APSME_TransportKeyReq_t req;


  req.keyType = KEY_TYPE_TC_MASTER;
  req.extAddr = device->extAddr;

  ZDSecMgrMasterKeyLookup( device->ctrl->entry->ami, &req.key );

  //check if using secure hop to to parent
  if ( device->parentAddr != NLME_GetShortAddr() )
  {
    //send to parent with security
    req.dstAddr = device->parentAddr;
    req.secure  = TRUE;
  }
  else
  {
    //direct with no security
    req.dstAddr = device->nwkAddr;
    req.secure  = FALSE;
  }

  status = APSME_TransportKeyReq( &req );

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

/******************************************************************************
 * @fn          ZDSecMgrSendNwkKey
 *
 * @brief       Send NWK key to device joining network.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrSendNwkKey( ZDSecMgrDevice_t* device )
{
  ZStatus_t               status;
  APSME_TransportKeyReq_t req;


  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  {
    // set values
    req.extAddr   = device->extAddr;
    req.keyType   = KEY_TYPE_NWK;
    req.keySeqNum = _NIB.nwkActiveKey.keySeqNum;
    req.key       = _NIB.nwkActiveKey.key;

    // check if using secure hop to to parent
    if ( device->parentAddr == NLME_GetShortAddr() )
    {
      req.dstAddr = device->nwkAddr;
      req.secure  = FALSE;
    }
    else
    {
      req.dstAddr = device->parentAddr;
      req.secure  = TRUE;
    }
  }
  //---------------------------------------------------------------------------
  #else // defined( ZDSECMGR_RESIDENTIAL )
  //---------------------------------------------------------------------------
  {
    // default values
    req.dstAddr = device->nwkAddr;
    req.secure  = device->secure;
    req.keyType = KEY_TYPE_NWK;
    req.extAddr = device->extAddr;

    // special cases
    if ( device->secure == FALSE )
    {
      req.keySeqNum = _NIB.nwkActiveKey.keySeqNum;
      req.key       = _NIB.nwkActiveKey.key;

      // check if using secure hop to to parent
      if ( device->parentAddr != NLME_GetShortAddr() )
      {
        req.dstAddr = device->parentAddr;
        req.secure  = TRUE;
      }
    }
    else
    {
      req.key       = NULL;
      req.keySeqNum = 0;
    }
  }
  //-------------------------------------------------------------------------
  #endif // defined( ZDSECMGR_RESIDENTIAL )
  //-------------------------------------------------------------------------

  status = APSME_TransportKeyReq( &req );

  return status;
}

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrDeviceEntryRemove
 *
 * @brief       Remove device entry.
 *
 * @param       entry - [in] valid entry
 *
 * @return      none
 */
void ZDSecMgrDeviceEntryRemove( ZDSecMgrEntry_t* entry )
{
  // terminate device control
  ZDSecMgrCtrlTerm( entry );

  // remove device from entry data
  ZDSecMgrEntryFree( entry );

  // remove EXT address
  //ZDSecMgrExtAddrRelease( aiOld );
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrDeviceEntryAdd
 *
 * @brief       Add entry.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 * @param       ami    - [in] Address Manager index
 *
 * @return      ZStatus_t
 */
void ZDSecMgrAddrMgrUpdate( uint16 ami, uint16 nwkAddr )
{
  AddrMgrEntry_t entry;

  // get the ami data
  entry.user  = ADDRMGR_USER_SECURITY;
  entry.index = ami;

  AddrMgrEntryGet( &entry );

  // check if NWK address is same
  if ( entry.nwkAddr != nwkAddr )
  {
    // update NWK address
    entry.nwkAddr = nwkAddr;

    AddrMgrEntryUpdate( &entry );
  }
}

ZStatus_t ZDSecMgrDeviceEntryAdd( ZDSecMgrDevice_t* device, uint16 ami )
{
  ZStatus_t        status;
  ZDSecMgrEntry_t* entry;


  // initialize as unknown until completion
  status = ZNwkUnknownDevice;

  device->ctrl = NULL;

  // make sure not already registered
  if ( ZDSecMgrEntryLookup( device->nwkAddr, &entry ) == ZSuccess )
  {
    // verify that address index is same
    if ( entry->ami != ami )
    {
      // remove conflicting entry
      ZDSecMgrDeviceEntryRemove( entry );

      if ( ZDSecMgrEntryLookupAMI( ami, &entry ) == ZSuccess )
      {
        // update NWK address
        ZDSecMgrAddrMgrUpdate( ami, device->nwkAddr );
      }
    }
  }
  else if ( ZDSecMgrEntryLookupAMI( ami, &entry ) == ZSuccess )
  {
    // update NWK address
    ZDSecMgrAddrMgrUpdate( ami, device->nwkAddr );
  }

  // check if a new entry needs to be created
  if ( entry == NULL )
  {
    // get new entry
    if ( ZDSecMgrEntryNew( &entry ) == ZSuccess )
    {
      // reset entry lkd

      // finish setting up entry
      entry->ami = ami;

      // update NWK address
      ZDSecMgrAddrMgrUpdate( ami, device->nwkAddr );

      // enter new device into device control
      status = ZDSecMgrCtrlAdd( device, entry );
    }
  }
  else
  {
    // reset entry lkd

    // reset entry in entry control
    status = ZDSecMgrCtrlReset( device, entry );
  }

  return status;
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrDeviceCtrlHandler
 *
 * @brief       Device control handler.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      none
 */
void ZDSecMgrDeviceCtrlHandler( ZDSecMgrDevice_t* device )
{
  uint8 state;
  uint8 cntr;


  state = device->ctrl->state;
  cntr  = ZDSECMGR_CTRL_BASE_CNTR;

  switch ( state )
  {
    case ZDSECMGR_CTRL_TK_MASTER:
      if ( ZDSecMgrSendMasterKey( device ) == ZSuccess )
      {
        state = ZDSECMGR_CTRL_SKKE_INIT;
        cntr  = ZDSECMGR_CTRL_SKKE_INIT_CNTR;
      }
      break;

    case ZDSECMGR_CTRL_SKKE_INIT:
      if ( ZDSecMgrEstablishKey( device ) == ZSuccess )
      {
        state = ZDSECMGR_CTRL_SKKE_WAIT;
        cntr  = ZDSECMGR_CTRL_SKKE_WAIT_CNTR;
      }
      break;

    case ZDSECMGR_CTRL_SKKE_WAIT:
      state = ZDSECMGR_CTRL_NONE;
      // timeout error - cleanup SKKE slot and entry
      break;

    case ZDSECMGR_CTRL_TK_NWK:
      if ( ZDSecMgrSendNwkKey( device ) == ZSuccess )
      {
        state = ZDSECMGR_CTRL_NONE;
      }
      break;

    default:
      state = ZDSECMGR_CTRL_NONE;
      break;
  }

  if ( state != ZDSECMGR_CTRL_NONE )
  {
    device->ctrl->state = state;
    device->ctrl->cntr  = cntr;

    osal_start_timer( ZDO_SECMGR_EVENT, 100 );
  }
  else
  {
    ZDSecMgrCtrlRelease( device->ctrl );
  }
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDO_COORDINATOR )
#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrDeviceCtrlSetup
 *
 * @brief       Setup device control.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
void ZDSecMgrDeviceCtrlSetup( ZDSecMgrDevice_t* device )
{
  if ( device->ctrl != NULL )
  {
    if ( device->secure == FALSE )
    {
      // send the master key data to the joining device
      device->ctrl->state = ZDSECMGR_CTRL_TK_MASTER;
    }
    else
    {
      // start SKKE
      device->ctrl->state = ZDSECMGR_CTRL_SKKE_INIT;
    }

    ZDSecMgrDeviceCtrlHandler( device );
  }
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrDeviceCtrlUpdate
 *
 * @brief       Update control data.
 *
 * @param       extAddr - [in] EXT address
 * @param       state   - [in] new control state
 *
 * @return      none
 */
void ZDSecMgrDeviceCtrlUpdate( uint8* extAddr, uint8 state )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  ZDSecMgrEntry_t* entry;
  ZDSecMgrCtrl_t*  ctrl;


  // lookup device entry data
  ZDSecMgrEntryLookupExt( extAddr, &entry );

  if ( entry != NULL )
  {
    // lookup device control data
    ZDSecMgrCtrlLookup( entry, &ctrl );

    // make sure control data is valid
    if ( ctrl != NULL )
    {
      // possible state transitions
      if ( ( state == ZDSECMGR_CTRL_SKKE_DONE       ) &&
           ( ctrl->state == ZDSECMGR_CTRL_SKKE_WAIT )    )
      {
        ctrl->state = ZDSECMGR_CTRL_TK_NWK;
        ctrl->cntr  = ZDSECMGR_CTRL_TK_NWK_CNTR;
      }

      // timer should be active
    }
  }
  //---------------------------------------------------------------------------
  #endif // defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( RTR_NWK )
/******************************************************************************
 * @fn          ZDSecMgrDeviceRemove
 *
 * @brief       Remove device from network.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      none
 */
void ZDSecMgrDeviceRemove( ZDSecMgrDevice_t* device )
{
  APSME_RemoveDeviceReq_t remDevReq;
  NLME_LeaveReq_t         leaveReq;
  associated_devices_t*   assoc;


  // check if parent, remove the device
  if ( device->parentAddr == NLME_GetShortAddr() )
  {
    // this is the parent of the device
    leaveReq.extAddr        = device->extAddr;
    leaveReq.removeChildren = FALSE;
    leaveReq.rejoin         = FALSE;

    // find child association
    assoc = AssocGetWithExt( device->extAddr );

    if ( ( assoc != NULL                            ) &&
         ( assoc->nodeRelation >= CHILD_RFD         ) &&
         ( assoc->nodeRelation <= CHILD_FFD_RX_IDLE )    )
    {
      // check if associated device is authenticated
      if ( assoc->devStatus & DEV_SEC_AUTH_STATUS )
      {
        leaveReq.silent = FALSE;
      }
      else
      {
        leaveReq.silent = TRUE;
      }

      NLME_LeaveReq( &leaveReq );
    }
  }
  else
  {
    // this is not the parent of the device
    remDevReq.parentAddr   = device->parentAddr;
    remDevReq.childExtAddr = device->extAddr;

    APSME_RemoveDeviceReq( &remDevReq );
  }
}
#endif // defined( RTR_NWK )

#if defined ( ZDSECMGR_COMMERCIAL )
#if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
/******************************************************************************
 * @fn          ZDSecMgrDeviceValidateSKKE
 *
 * @brief       Decide whether device is allowed for SKKE.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceValidateSKKE( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;
  uint16    ami;
  uint8*    key;


  // get EXT address
  status = ZDSecMgrExtAddrLookup( device->extAddr, &ami );

  if ( status == ZSuccess )
  {
    // get MASTER key
    status = ZDSecMgrMasterKeyLookup( ami, &key );

    if ( status == ZSuccess )
    {
    //  // check if initiator is Trust Center
    //  if ( device->nwkAddr == APSME_TRUSTCENTER_NWKADDR )
    //  {
    //    // verify NWK key not sent
    //    // devtag.todo
    //    // temporary - add device to internal data
    //    status = ZDSecMgrDeviceEntryAdd( device, ami );
    //  }
    //  else
    //  {
    //    // initiator not Trust Center - End to End SKKE - set policy
    //    // for accepting an SKKE initiation
    //    // temporary - add device to internal data
    //    status = ZDSecMgrDeviceEntryAdd( device, ami );
    //  }
        status = ZDSecMgrDeviceEntryAdd( device, ami );
    }
  }

  return status;
}
#endif // !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_RESIDENTIAL )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrDeviceValidateRM (RESIDENTIAL MODE)
 *
 * @brief       Decide whether device is allowed.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceValidateRM( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;
  uint8     index;
  uint8*    restricted;

  status = ZSuccess;

  // Look through the restricted device list
  for ( index = 0; index < ZDSECMGR_RESTRICTED_DEVICES; index++ )
  {
    restricted = ZDSecMgrRestrictedDevices[index];

    if ( AddrMgrExtAddrEqual( restricted, device->extAddr )  == TRUE )
    {
      // return as unknown device in regards to validation
      status = ZNwkUnknownDevice;

      // break from loop
      index = ZDSECMGR_RESTRICTED_DEVICES;
    }
  }

  return status;
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_RESIDENTIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrDeviceValidateCM (COMMERCIAL MODE)
 *
 * @brief       Decide whether device is allowed.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceValidateCM( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;
  uint16    ami;
  uint8*    key;


  // check for pre configured setting
  if ( device->secure == TRUE )
  {
    // get EXT address and MASTER key
    status = ZDSecMgrExtAddrLookup( device->extAddr, &ami );

    if ( status == ZSuccess )
    {
      status = ZDSecMgrMasterKeyLookup( ami, &key );
    }
  }
  else
  {
    // implement EXT address and MASTER key policy here -- the total number of
    // Security Manager entries should never exceed the number of EXT addresses
    // and MASTER keys available

    // set status based on policy
    status = ZSuccess; // ZNwkUnknownDevice;

    // get the address index
    if ( ZDSecMgrExtAddrLookup( device->extAddr, &ami ) != ZSuccess )
    {
      // if policy, store new EXT address
      status = ZDSecMgrExtAddrStore( device->extAddr, &ami );
    }

    // get the address index
    if ( ZDSecMgrMasterKeyLookup( ami, &key ) != ZSuccess )
    {
      // if policy, store new key -- NULL will zero key
      status = ZDSecMgrMasterKeyStore( ami, NULL );
    }
  }

  // if EXT address and MASTER key available -- add device
  if ( status == ZSuccess )
  {
    // add device to internal data - with control
    status = ZDSecMgrDeviceEntryAdd( device, ami );
  }

  return status;
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrDeviceValidate
 *
 * @brief       Decide whether device is allowed.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceValidate( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;


  if ( ZDSecMgrPermitJoiningEnabled == TRUE )
  {
    // device may be joining with a secure flag but it is ultimately the Trust
    // Center that decides -- check if expected pre configured device --
    // override settings
    if ( zgPreConfigKeys == TRUE )
    {
        device->secure = TRUE;
    }
    else
    {
        device->secure = FALSE;
    }

    //-------------------------------------------------------------------------
    #if defined ( ZDSECMGR_COMMERCIAL )
    //-------------------------------------------------------------------------
    status = ZDSecMgrDeviceValidateCM( device );
    //-------------------------------------------------------------------------
    #else // defined( ZDSECMGR_RESIDENTIAL )
    //-------------------------------------------------------------------------
    status = ZDSecMgrDeviceValidateRM( device );
    //-------------------------------------------------------------------------
    #endif // defined( ZDSECMGR_RESIDENTIAL )
    //-------------------------------------------------------------------------
  }
  else
  {
    status = ZNwkUnknownDevice;
  }

  return status;
}
#endif // defined ( ZDO_COORDINATOR )

#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrDeviceJoin
 *
 * @brief       Try to join this device.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceJoin( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;


  // attempt to validate device
  status = ZDSecMgrDeviceValidate( device );

  if ( status == ZSuccess )
  {
    //-------------------------------------------------------------------------
    #if defined ( ZDSECMGR_COMMERCIAL )
    //-------------------------------------------------------------------------
    ZDSecMgrDeviceCtrlSetup( device );
    //-------------------------------------------------------------------------
    #else // defined( ZDSECMGR_RESIDENTIAL )
    //-------------------------------------------------------------------------
    //send the nwk key data to the joining device
    status = ZDSecMgrSendNwkKey( device );
    //-------------------------------------------------------------------------
    #endif // defined( ZDSECMGR_RESIDENTIAL )
    //-------------------------------------------------------------------------
  }
  else
  {
    // not allowed, remove the device
    ZDSecMgrDeviceRemove( device );
  }

  return status;
}
#endif // defined ( ZDO_COORDINATOR )

#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrDeviceJoinDirect
 *
 * @brief       Try to join this device as a direct child.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceJoinDirect( ZDSecMgrDevice_t* device )
{
  ZStatus_t status;

  status = ZDSecMgrDeviceJoin( device );

  if ( status == ZSuccess )
  {
    // set association status to authenticated
    ZDSecMgrAssocDeviceAuth( AssocGetWithShort( device->nwkAddr ) );
  }

  return status;
}
#endif // defined ( ZDO_COORDINATOR )

#if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
/******************************************************************************
 * @fn          ZDSecMgrDeviceJoinFwd
 *
 * @brief       Forward join to Trust Center.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceJoinFwd( ZDSecMgrDevice_t* device )
{
  ZStatus_t               status;
  APSME_UpdateDeviceReq_t req;


  // forward any joining device to the Trust Center -- the Trust Center will
  // decide if the device is allowed to join
  status = ZSuccess;

  //if ( status == ZSuccess )
  //{
    // forward authorization to the Trust Center
    req.dstAddr    = APSME_TRUSTCENTER_NWKADDR;
    req.devAddr    = device->nwkAddr;
    req.devExtAddr = device->extAddr;

    // set security status, option for router to reject if policy set
    if ( device->secure == TRUE )
    {
        req.status = APSME_UD_SECURED_JOIN;
    }
    else
    {
        req.status = APSME_UD_UNSECURED_JOIN;
    }

    // send and APSME_UPDATE_DEVICE request to the trust center
    status = APSME_UpdateDeviceReq( &req );
  //}
  //else
  //{
  //  // not allowed, remove the device
  //  ZDSecMgrDeviceRemove( device );
  //}

  return status;
}
#endif // !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )

#if defined ( RTR_NWK )
/******************************************************************************
 * @fn          ZDSecMgrDeviceNew
 *
 * @brief       Process a new device.
 *
 * @param       device - [in] ZDSecMgrDevice_t, device info
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrDeviceNew( ZDSecMgrDevice_t* joiner )
{
  ZStatus_t status;

  //---------------------------------------------------------------------------
  #if defined ( ZDO_COORDINATOR ) && !defined ( SOFT_START )
  //---------------------------------------------------------------------------
  // try to join this device
  status = ZDSecMgrDeviceJoinDirect( joiner );
  //---------------------------------------------------------------------------
  #elif defined ( ZDO_COORDINATOR ) && defined ( SOFT_START )
  //---------------------------------------------------------------------------
  // which mode -- COORD or ROUTER
  if ( ZDO_Config_Node_Descriptor.LogicalType == NODETYPE_COORDINATOR )
  {
    // try to join this device
    status = ZDSecMgrDeviceJoinDirect( joiner );
  }
  else
  {
    // forward join to Trust Center
    status = ZDSecMgrDeviceJoinFwd( joiner );
  }
  //---------------------------------------------------------------------------
  #else // !ZDO_COORDINATOR
  //---------------------------------------------------------------------------
  // forward join to Trust Center
  status = ZDSecMgrDeviceJoinFwd( joiner );
  //---------------------------------------------------------------------------
  #endif // !ZDO_COORDINATOR
  //---------------------------------------------------------------------------

  return status;
}
#endif // defined ( RTR_NWK )

#if defined ( RTR_NWK )
/******************************************************************************
 * @fn          ZDSecMgrAssocDeviceAuth
 *
 * @brief       Set associated device status to authenticated
 *
 * @param       assoc - [in, out] associated_devices_t
 *
 * @return      none
 */
void ZDSecMgrAssocDeviceAuth( associated_devices_t* assoc )
{
  if ( assoc != NULL )
  {
    assoc->devStatus |= DEV_SEC_AUTH_STATUS;
  }
}
#endif // defined ( RTR_NWK )
#endif // defined ( ZDSECMGR_SECURE )

/******************************************************************************
 * PUBLIC FUNCTIONS
 */
/******************************************************************************
 * @fn          ZDSecMgrInit
 *
 * @brief       Initialize ZigBee Device Security Manager.
 *
 * @param       none
 *
 * @return      none
 */
#if defined ( ZDSECMGR_COMMERCIAL )
#if ( ADDRMGR_CALLBACK_ENABLED == 1 )
void ZDSecMgrAddrMgrCB( uint8           update,
                        AddrMgrEntry_t* newEntry,
                        AddrMgrEntry_t* oldEntry )
{
  (void)update;
  (void)newEntry;
  (void)oldEntry;
}
#endif // ( ADDRMGR_CALLBACK_ENABLED == 1 )
#endif // defined ( ZDSECMGR_COMMERCIAL )

void ZDSecMgrInit( void )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  // initialize sub modules
  ZDSecMgrMasterKeyInit();
  ZDSecMgrEntryInit();
  ZDSecMgrCtrlInit();

  // configure SKKE slot data
  APSME_SKKE_SlotInit( ZDSECMGR_SKKE_SLOT_MAX );

  // register with Address Manager
  #if ( ADDRMGR_CALLBACK_ENABLED == 1 )
  AddrMgrRegister( ADDRMGR_REG_SECURITY, ZDSecMgrAddrMgrCB );
  #endif
  //---------------------------------------------------------------------------
  #endif // defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  // setup joining permissions
  ZDSecMgrPermitJoiningEnabled = TRUE;
  ZDSecMgrPermitJoiningTimed   = FALSE;
  //---------------------------------------------------------------------------
  #endif // defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------

  // configure security based on security mode and type of device
  ZDSecMgrConfig();
}

/******************************************************************************
 * @fn          ZDSecMgrConfig
 *
 * @brief       Configure ZigBee Device Security Manager.
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrConfig( void )
{
  #if defined ( ZDSECMGR_SECURE )
  SSP_Init();
  #endif

  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  {
    #if defined ( ZDO_COORDINATOR )
    {
      #if defined ( SOFT_START )
      {
        //switch here
        if ( ZDO_Config_Node_Descriptor.LogicalType == NODETYPE_COORDINATOR )
        {
          // COMMERCIAL MODE - COORDINATOR DEVICE
          APSME_SecurityCM_CD();
        }
        else
        {
          // COMMERCIAL MODE - ROUTER DEVICE
          APSME_SecurityCM_RD();
        }
      }
      #else
      {
        // COMMERCIAL MODE - COORDINATOR DEVICE
        APSME_SecurityCM_CD();
      }
      #endif
    }
    #elif defined ( RTR_NWK )
    {
      // COMMERCIAL MODE - ROUTER DEVICE
      APSME_SecurityCM_RD();
    }
    #else
    {
      // COMMERCIAL MODE - END DEVICE
      APSME_SecurityCM_ED();
    }
    #endif
  }
  //---------------------------------------------------------------------------
  #elif defined (ZDSECMGR_RESIDENTIAL )
  //---------------------------------------------------------------------------
  {
    #if defined ( ZDO_COORDINATOR )
    {
      #if defined ( SOFT_START )
      {
        //switch here
        if ( ZDO_Config_Node_Descriptor.LogicalType == NODETYPE_COORDINATOR )
        {
          // RESIDENTIAL MODE - COORDINATOR DEVICE
          APSME_SecurityRM_CD();
        }
        else
        {
          // RESIDENTIAL MODE - ROUTER DEVICE
          APSME_SecurityRM_RD();
        }
      }
      #else
      {
        // RESIDENTIAL MODE - COORDINATOR DEVICE
        APSME_SecurityRM_CD();
      }
      #endif
    }
    #elif defined ( RTR_NWK )
    {
      // RESIDENTIAL MODE - ROUTER DEVICE
      APSME_SecurityRM_RD();
    }
    #else
    {
      // RESIDENTIAL MODE - END DEVICE
      APSME_SecurityRM_ED();
    }
    #endif
  }
  //---------------------------------------------------------------------------
  #else
  //---------------------------------------------------------------------------
  {
    // NO SECURITY
    APSME_SecurityNM();
  }
  //---------------------------------------------------------------------------
  #endif
  //---------------------------------------------------------------------------
}

#if defined( ZDO_MGMT_PERMIT_JOIN_RESPONSE ) && defined( RTR_NWK )
/******************************************************************************
 * @fn          ZDSecMgrPermitJoining
 *
 * @brief       Process request to change joining permissions.
 *
 * @param       duration - [in] timed duration for join in seconds
 *                         - 0x00 not allowed
 *                         - 0xFF allowed without timeout
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
uint8 ZDSecMgrPermitJoining( uint8 duration )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  uint8 accept;


  ZDSecMgrPermitJoiningTimed = FALSE;

  if ( duration > 0 )
  {
    ZDSecMgrPermitJoiningEnabled = TRUE;

    if ( duration != 0xFF )
    {
      ZDSecMgrPermitJoiningTimed = TRUE;
    }
  }
  else
  {
    ZDSecMgrPermitJoiningEnabled = FALSE;
  }

  accept = TRUE;

  return accept;
  //---------------------------------------------------------------------------
  #else // !defined ( ZDSECMGR_SECURE ) || !defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  return FALSE;
  //---------------------------------------------------------------------------
  #endif // !defined ( ZDSECMGR_SECURE ) || !defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
}
#endif // defined( ZDO_MGMT_PERMIT_JOIN_RESPONSE ) && defined( RTR_NWK )

#if defined( ZDO_MGMT_PERMIT_JOIN_RESPONSE ) && defined( RTR_NWK )
/******************************************************************************
 * @fn          ZDSecMgrPermitJoiningTimeout
 *
 * @brief       Process permit joining timeout
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrPermitJoiningTimeout( void )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  if ( ZDSecMgrPermitJoiningTimed == TRUE )
  {
    ZDSecMgrPermitJoiningEnabled = FALSE;
    ZDSecMgrPermitJoiningTimed   = FALSE;
  }
  //---------------------------------------------------------------------------
  #endif // defined ( ZDSECMGR_SECURE ) && defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
}
#endif // defined( ZDO_MGMT_PERMIT_JOIN_RESPONSE ) && defined( RTR_NWK )

#if defined ( ZDSECMGR_SECURE )
#if defined ( RTR_NWK )
/******************************************************************************
 * @fn          ZDSecMgrNewDeviceEvent
 *
 * @brief       Process a the new device event, if found reset new device
 *              event/timer.
 *
 * @param       none
 *
 * @return      uint8 - found(TRUE:FALSE)
 */
uint8 ZDSecMgrNewDeviceEvent( void )
{
  uint8                 found;
  ZDSecMgrDevice_t      device;
  AddrMgrEntry_t        addrEntry;
  associated_devices_t* assoc;
  ZStatus_t             status;

  // initialize return results
  found = FALSE;

  // look for device in the security init state
  assoc = AssocMatchDeviceStatus( DEV_SEC_INIT_STATUS );

  if ( assoc != NULL )
  {
    // device found
    found = TRUE;

    // check for preconfigured security
    if ( zgPreConfigKeys == TRUE )
    {
      // set association status to authenticated
      ZDSecMgrAssocDeviceAuth( assoc );
    }

    // set up device info
    addrEntry.user  = ADDRMGR_USER_DEFAULT;
    addrEntry.index = assoc->addrIdx;
    AddrMgrEntryGet( &addrEntry );

    device.nwkAddr    = assoc->shortAddr;
    device.extAddr    = addrEntry.extAddr;
    device.parentAddr = NLME_GetShortAddr();
    device.secure     = FALSE;

    // process new device
    status = ZDSecMgrDeviceNew( &device );

    if ( status == ZSuccess )
    {
      assoc->devStatus &= ~DEV_SEC_INIT_STATUS;
    }
    else if ( status == ZNwkUnknownDevice )
    {
      AssocRemove( addrEntry.extAddr );
    }
  }

  return found;
}
#endif // defined ( RTR_NWK )
#endif // defined ( ZDSECMGR_SECURE )

#if defined( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEvent
 *
 * @brief       Handle ZDO Security Manager event/timer(ZDO_SECMGR_EVENT).
 *
 * @param       none
 *
 * @return      none
 */
void ZDSecMgrEvent( void )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
  uint8            action;
  uint8            restart;
  uint16           index;
  AddrMgrEntry_t   entry;
  ZDSecMgrDevice_t device;


  // verify data is available
  if ( ZDSecMgrCtrlData != NULL )
  {
    action  = FALSE;
    restart = FALSE;

    // update all the counters
    for ( index = 0; index < ZDSECMGR_ENTRY_MAX; index++ )
    {
      if ( ZDSecMgrCtrlData[index].state !=  ZDSECMGR_CTRL_NONE )
      {
        if ( ZDSecMgrCtrlData[index].cntr != 0 )
        {
          ZDSecMgrCtrlData[index].cntr--;
        }

        if ( ( action == FALSE ) && ( ZDSecMgrCtrlData[index].cntr == 0 ) )
        {
          action = TRUE;

          // update from control data
          device.parentAddr = ZDSecMgrCtrlData[index].parentAddr;
          device.secure     = ZDSecMgrCtrlData[index].secure;
          device.ctrl       = &ZDSecMgrCtrlData[index];

          // set the user and address index
          entry.user  = ADDRMGR_USER_SECURITY;
          entry.index = ZDSecMgrCtrlData[index].entry->ami;

          // get the address data
          AddrMgrEntryGet( &entry );

          // set device address data
          device.nwkAddr = entry.nwkAddr;
          device.extAddr = entry.extAddr;

          // update from entry data
          ZDSecMgrDeviceCtrlHandler( &device );
        }
        else
        {
          restart = TRUE;
        }
      }
    }

    // check for timer restart
    if ( restart == TRUE )
    {
      osal_start_timer( ZDO_SECMGR_EVENT, 100 );
    }
  }
  //---------------------------------------------------------------------------
  #endif // defined ( ZDO_COORDINATOR )
  //---------------------------------------------------------------------------
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
/******************************************************************************
 * @fn          ZDSecMgrEstablishKeyCfm
 *
 * @brief       Process the ZDO_EstablishKeyCfm_t message.
 *
 * @param       cfm - [in] ZDO_EstablishKeyCfm_t confirmation
 *
 * @return      none
 */
void ZDSecMgrEstablishKeyCfm( ZDO_EstablishKeyCfm_t* cfm )
{
  // send the NWK key
  if ( ZDO_Config_Node_Descriptor.LogicalType == NODETYPE_COORDINATOR )
  {
    // update control for specified EXT address
    ZDSecMgrDeviceCtrlUpdate( cfm->partExtAddr, ZDSECMGR_CTRL_SKKE_DONE );
  }
  else
  {
    // this should be done when receiving the NWK key
    // if devState ==
    //if ( devState == DEV_END_DEVICE_UNAUTH )
        //osal_set_event( ZDAppTaskID, ZDO_DEVICE_AUTH );

    // if not in joining state -- this should trigger an event for an
    // end point that requested SKKE
    // if ( devState == DEV_END_DEVICE )
   //       devState == DEV_ROUTER;

  }
}
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_COMMERCIAL )
#if !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
/******************************************************************************
 * @fn          ZDSecMgrEstablishKeyInd
 *
 * @brief       Process the ZDO_EstablishKeyInd_t message.
 *
 * @param       ind - [in] ZDO_EstablishKeyInd_t indication
 *
 * @return      none
 */
void ZDSecMgrEstablishKeyInd( ZDO_EstablishKeyInd_t* ind )
{
  ZDSecMgrDevice_t        device;
  APSME_EstablishKeyRsp_t rsp;


  device.extAddr = ind->initExtAddr;
  device.secure  = ind->secure;

  if ( ind->secure == FALSE )
  {
    // SKKE from Trust Center is not secured between child and parent
    device.nwkAddr    = APSME_TRUSTCENTER_NWKADDR;
    device.parentAddr = ind->srcAddr;
  }
  else
  {
    // SKKE from initiator should be secured
    device.nwkAddr    = ind->srcAddr;
    device.parentAddr = INVALID_NODE_ADDR;
  }

  rsp.dstAddr     = ind->srcAddr;
  rsp.initExtAddr = &ind->initExtAddr[0];
  rsp.secure      = ind->secure;

  // validate device for SKKE
  if ( ZDSecMgrDeviceValidateSKKE( &device ) == ZSuccess )
  {
    rsp.accept = TRUE;
  }
  else
  {
    rsp.accept = FALSE;
  }

  APSME_EstablishKeyRsp( &rsp );
}
#endif // !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_SECURE )
#if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
/******************************************************************************
 * @fn          ZDSecMgrTransportKeyInd
 *
 * @brief       Process the ZDO_TransportKeyInd_t message.
 *
 * @param       ind - [in] ZDO_TransportKeyInd_t indication
 *
 * @return      none
 */
void ZDSecMgrTransportKeyInd( ZDO_TransportKeyInd_t* ind )
{
  uint8 index;


  if ( ind->keyType == KEY_TYPE_TC_MASTER )
  {
    #if defined ( ZDSECMGR_COMMERCIAL )
    {
      if ( zgPreConfigKeys != TRUE )
      {
        ZDSecMgrMasterKeyLoad( ind->srcAddr, ind->srcExtAddr, ind->key );
      }
      else
      {
        // error condition - reject key
      }
    }
    #endif // defined ( ZDSECMGR_COMMERCIAL )
  }
  else if ( ind->keyType == KEY_TYPE_NWK )
  {
    // check for dummy NWK key (all zeros)
    for ( index = 0;
          ( (index < SEC_KEY_LEN) && (ind->key[index] == 0) );
          index++ );

    if ( index == SEC_KEY_LEN )
    {
      // load preconfigured key
      SSP_UpdateNwkKey( (byte*)zgPreConfigKey, 0 );
      SSP_SwitchNwkKey( 0 );
    }
    else
    {
      SSP_UpdateNwkKey( ind->key, ind->keySeqNum );
      if ( !_NIB.nwkKeyLoaded )
      {
        SSP_SwitchNwkKey( ind->keySeqNum );
      }
    }

    // inform ZDO that NWK key was received
    osal_set_event ( ZDAppTaskID, ZDO_DEVICE_AUTH );
  }
  else if ( ind->keyType == KEY_TYPE_APP_MASTER )
  {
    #if defined ( ZDSECMGR_COMMERCIAL )
    #endif
  }
  else // (ind->keyType==KEY_TYPE_APP_LINK)
  {
    #if defined ( ZDSECMGR_COMMERCIAL )
      ZDSecMgrLinkKeySet( ind->srcExtAddr, ind->key );
    #endif
  }
}
#endif // !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
#endif // defined ( ZDSECMGR_SECURE )

#if defined ( ZDSECMGR_SECURE )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrUpdateDeviceInd
 *
 * @brief       Process the ZDO_UpdateDeviceInd_t message.
 *
 * @param       ind - [in] ZDO_UpdateDeviceInd_t indication
 *
 * @return      none
 */
void ZDSecMgrUpdateDeviceInd( ZDO_UpdateDeviceInd_t* ind )
{
  ZDSecMgrDevice_t device;


  device.nwkAddr    = ind->devAddr;
  device.extAddr    = ind->devExtAddr;
  device.parentAddr = ind->srcAddr;

  if ( ( ind->status == APSME_UD_SECURED_JOIN   ) ||
       ( ind->status == APSME_UD_UNSECURED_JOIN )   )
  {
    if ( ind->status == APSME_UD_SECURED_JOIN )
    {
      device.secure = TRUE;
    }
    else
    {
      device.secure = FALSE;
    }

    // try to join this device
    ZDSecMgrDeviceJoin( &device );
  }
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_SECURE )

#if defined ( ZDSECMGR_SECURE )
#if defined ( RTR_NWK )
#if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
/******************************************************************************
 * @fn          ZDSecMgrRemoveDeviceInd
 *
 * @brief       Process the ZDO_RemoveDeviceInd_t message.
 *
 * @param       ind - [in] ZDO_RemoveDeviceInd_t indication
 *
 * @return      none
 */
void ZDSecMgrRemoveDeviceInd( ZDO_RemoveDeviceInd_t* ind )
{
  ZDSecMgrDevice_t device;


  // only accept from Trust Center
  if ( ind->srcAddr == APSME_TRUSTCENTER_NWKADDR )
  {
    // look up NWK address
    if ( APSME_LookupNwkAddr( ind->childExtAddr, &device.nwkAddr ) == TRUE )
    {
      device.parentAddr = NLME_GetShortAddr();
      device.extAddr    = ind->childExtAddr;

      // remove device
      ZDSecMgrDeviceRemove( &device );
    }
  }
}
#endif // !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
#endif // defined( RTR_NWK )
#endif // defined ( ZDSECMGR_SECURE )

#if defined ( ZDSECMGR_COMMERCIAL )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrRequestKeyInd
 *
 * @brief       Process the ZDO_RequestKeyInd_t message.
 *
 * @param       ind - [in] ZDO_RequestKeyInd_t indication
 *
 * @return      none
 */
void ZDSecMgrRequestKeyInd( ZDO_RequestKeyInd_t* ind )
{
  if ( ind->keyType == KEY_TYPE_NWK )
  {
  }
  else if ( ind->keyType == KEY_TYPE_APP_MASTER )
  {
    ZDSecMgrAppKeyReq( ind );
  }
  //else ignore
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_COMMERCIAL )

#if defined ( ZDSECMGR_SECURE )
#if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
/******************************************************************************
 * @fn          ZDSecMgrSwitchKeyInd
 *
 * @brief       Process the ZDO_SwitchKeyInd_t message.
 *
 * @param       ind - [in] ZDO_SwitchKeyInd_t indication
 *
 * @return      none
 */
void ZDSecMgrSwitchKeyInd( ZDO_SwitchKeyInd_t* ind )
{
  SSP_SwitchNwkKey( ind->keySeqNum );

  // Save if nv
  osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );
}
#endif // !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
#endif // defined ( ZDSECMGR_SECURE )

#if defined ( ZDSECMGR_SECURE )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrUpdateNwkKey
 *
 * @brief       Load a new NWK key and trigger a network wide update.
 *
 * @param       key       - [in] new NWK key
 * @param       keySeqNum - [in] new NWK key sequence number
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrUpdateNwkKey( uint8* key, uint8 keySeqNum )
{
  ZStatus_t               status;
  APSME_TransportKeyReq_t req;


  req.keyType   = KEY_TYPE_NWK;
  req.keySeqNum = keySeqNum;
  req.key       = key;
  req.dstAddr   = NWK_BROADCAST_SHORTADDR;
  req.extAddr   = NULL;
  req.secure    = TRUE;

  status = APSME_TransportKeyReq( &req );

  SSP_UpdateNwkKey( key, keySeqNum );

  // Save if nv
  osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );

  return status;
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_SECURE )

#if defined ( ZDSECMGR_SECURE )
#if defined ( ZDO_COORDINATOR )
/******************************************************************************
 * @fn          ZDSecMgrSwitchNwkKey
 *
 * @brief       Causes the NWK key to switch via a network wide command.
 *
 * @param       keySeqNum - [in] new NWK key sequence number
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrSwitchNwkKey( uint8 keySeqNum )
{
  ZStatus_t            status;
  APSME_SwitchKeyReq_t req;


  req.dstAddr   = NWK_BROADCAST_SHORTADDR;
  req.keySeqNum = keySeqNum;

  status = APSME_SwitchKeyReq( &req );

  SSP_SwitchNwkKey( keySeqNum );

  // Save if nv
  osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );

  return status;
}
#endif // defined ( ZDO_COORDINATOR )
#endif // defined ( ZDSECMGR_SECURE )

/******************************************************************************
 * ZigBee Device Security Manager - Stub Implementations
 */
/******************************************************************************
 * @fn          ZDSecMgrMasterKeyGet (stubs APSME_MasterKeyGet)
 *
 * @brief       Get MASTER key for specified EXT address.
 *
 * @param       extAddr - [in] EXT address
 * @param       key     - [out] MASTER key
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrMasterKeyGet( uint8* extAddr, uint8** key )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  {
    ZStatus_t        status;
    ZDSecMgrEntry_t* entry;


    // lookup entry for specified EXT address
    status = ZDSecMgrEntryLookupExt( extAddr, &entry );

    if ( status == ZSuccess )
    {
      ZDSecMgrMasterKeyLookup( entry->ami, key );
    }
    else
    {
      *key = NULL;
    }

    return status;
  }
  //---------------------------------------------------------------------------
  #else // !defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  {
    return ZNwkUnknownDevice;
  }
  //---------------------------------------------------------------------------
  #endif
  //---------------------------------------------------------------------------
}

/******************************************************************************
 * @fn          ZDSecMgrLinkKeySet (stubs APSME_LinkKeyExtSet)
 *
 * @brief       Set <APSME_LinkKeyData_t> for specified NWK address.
 *
 * @param       extAddr - [in] EXT address
 * @param       data    - [in] APSME_LinkKeyData_t
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrLinkKeySet( uint8* extAddr, uint8* key )
{
  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  {
    ZStatus_t        status;
    ZDSecMgrEntry_t* entry;


    // lookup entry index for specified EXT address
    status = ZDSecMgrEntryLookupExt( extAddr, &entry );

    if ( status == ZSuccess )
    {
      // setup the link key data reference
      osal_memcpy( entry->lkd.key, key, SEC_KEY_LEN );

      entry->lkd.apsmelkd.rxFrmCntr = 0;
      entry->lkd.apsmelkd.txFrmCntr = 0;
    }

    return status;
  }
  //---------------------------------------------------------------------------
  #else // !defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  {
    return ZNwkUnknownDevice;
  }
  //---------------------------------------------------------------------------
  #endif
  //---------------------------------------------------------------------------
}

/******************************************************************************
 * @fn          ZDSecMgrLinkKeyDataGet (stubs APSME_LinkKeyDataGet)
 *
 * @brief       Get <APSME_LinkKeyData_t> for specified NWK address.
 *
 * @param       nwkAddr - [in] NWK address
 * @param       data    - [out] APSME_LinkKeyData_t
 *
 * @return      ZStatus_t
 */
ZStatus_t ZDSecMgrLinkKeyDataGet(uint16 nwkAddr, APSME_LinkKeyData_t** data)
{
  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  {
    ZStatus_t        status;
    ZDSecMgrEntry_t* entry;


    // lookup entry index for specified NWK address
    status = ZDSecMgrEntryLookup( nwkAddr, &entry );

    if ( status == ZSuccess )
    {
      // setup the link key data reference
      (*data) = &entry->lkd.apsmelkd;
      (*data)->key = entry->lkd.key;
    }
    else
    {
      *data = NULL;
    }

    return status;
  }
  //---------------------------------------------------------------------------
  #else // !defined ( ZDSECMGR_COMMERCIAL )
  //---------------------------------------------------------------------------
  {
    return ZNwkUnknownDevice;
  }
  //---------------------------------------------------------------------------
  #endif
  //---------------------------------------------------------------------------
}

/******************************************************************************
 * @fn          ZDSecMgrKeyFwdToChild (stubs APSME_KeyFwdToChild)
 *
 * @brief       Verify and process key transportation to child.
 *
 * @param       ind - [in] APSME_TransportKeyInd_t
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
uint8 ZDSecMgrKeyFwdToChild( APSME_TransportKeyInd_t* ind )
{
  uint8 success;

  success = FALSE;

  //---------------------------------------------------------------------------
  #if defined ( ZDSECMGR_SECURE ) && defined ( RTR_NWK )
  //---------------------------------------------------------------------------
  // verify from Trust Center
  if ( ind->srcAddr == APSME_TRUSTCENTER_NWKADDR )
  {
    success = TRUE;

    // check for initial NWK key
    if ( ind->keyType == KEY_TYPE_NWK )
    {
      // set association status to authenticated
      ZDSecMgrAssocDeviceAuth( AssocGetWithExt( ind->dstExtAddr ) );
    }
  }
  //---------------------------------------------------------------------------
  #endif // defined ( ZDSECMGR_SECURE ) && defined ( RTR_NWK )
  //---------------------------------------------------------------------------

  return success;
}

/******************************************************************************
******************************************************************************/

