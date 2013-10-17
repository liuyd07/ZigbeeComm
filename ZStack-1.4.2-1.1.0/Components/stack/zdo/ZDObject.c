/*********************************************************************
    Filename:       ZDObject.c
    Revised:        $Date: 2007-05-14 17:34:18 -0700 (Mon, 14 May 2007) $
    Revision:       $Revision: 14296 $

    Description:

      This Zigbee Device Object.

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
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "rtg.h"
#include "NLMEDE.h"
#include "nwk_globals.h"
#include "APS.h"
#include "APSMEDE.h"
#include "AssocList.h"
#include "BindingTable.h"
#include "AddrMgr.h"
#include "AF.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "ZDConfig.h"
#include "ZDCache.h"
#include "ZDSecMgr.h"
#include "ZDApp.h"
#include "nwk_util.h"   // NLME_IsAddressBroadcast()
#include "ZGlobals.h"

#if defined( LCD_SUPPORTED )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
// NLME Stub Implementations
#define ZDO_ProcessMgmtPermitJoinTimeout NLME_PermitJoiningTimeout

// Status fields used by ZDO_ProcessMgmtRtgReq
#define ZDO_MGMT_RTG_ENTRY_ACTIVE             0x00
#define ZDO_MGMT_RTG_ENTRY_DISCOVERY_UNDERWAY 0x01
#define ZDO_MGMT_RTG_ENTRY_DISCOVERY_FAILED   0x02
#define ZDO_MGMT_RTG_ENTRY_INACTIVE           0x03

/*********************************************************************
 * TYPEDEFS
 */
#if defined ( REFLECTOR )
typedef struct
{
  byte SrcTransSeq;
  zAddrType_t SrcAddr;
  uint16 LocalCoordinator;
  byte epIntf;
  uint16 ProfileID;
  byte numInClusters;
  uint16 *inClusters;
  byte numOutClusters;
  uint16 *outClusters;
  byte SecurityUse;
  byte status;
} ZDO_EDBind_t;
#endif // defined ( REFLECTOR )

#if defined ( ZDO_COORDINATOR )
enum
{
  ZDMATCH_INIT,           // Initialized
  ZDMATCH_WAIT_REQ,       // Received first request, waiting for second
  ZDMATCH_SENDING_BINDS   // Received both requests, sending unbind/binds
};

enum
{
  ZDMATCH_REASON_START,
  ZDMATCH_REASON_TIMEOUT,
  ZDMATCH_REASON_UNBIND_RSP,
  ZDMATCH_REASON_BIND_RSP
};

enum
{
  ZDMATCH_SENDING_NOT,
  ZDMATCH_SENDING_UNBIND,
  ZDMATCH_SENDING_BIND
};

typedef struct
{
  ZDEndDeviceBind_t ed1;
  ZDEndDeviceBind_t ed2;
  uint8  state;            // One of the above states
  uint8  sending;         // 0 - not sent, 1 - unbind, 2 bind - expecting response
  uint8  transSeq;
  uint8  ed1numMatched;
  uint16 *ed1Matched;
  uint8  ed2numMatched;
  uint16 *ed2Matched;
} ZDMatchEndDeviceBind_t;
#endif

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
 * LOCAL VARIABLES
 */
static uint16 ZDOBuildBuf[26];  // temp area to build data without allocation

#if defined ( REFLECTOR )
static ZDO_EDBind_t *ZDO_EDBind;     // Null when not used
#endif

#if defined ( MANAGED_SCAN )
  uint32 managedScanNextChannel = 0;
  uint32 managedScanChannelMask = 0;
  uint8  managedScanTimesPerChannel = 0;
#endif

#if defined ( ZDO_COORDINATOR )
  ZDMatchEndDeviceBind_t *matchED = (ZDMatchEndDeviceBind_t *)NULL;
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void ZDODeviceSetup( void );
static uint16 *ZDO_CreateAlignedUINT16List(uint8 num, uint8 *buf);
#if defined ( MANAGED_SCAN )
  static void ZDOManagedScan_Next( void );
#endif
#if defined ( REFLECTOR )
  static void ZDO_RemoveEndDeviceBind( void );
  static void ZDO_SendEDBindRsp( byte TransSeq, zAddrType_t *dstAddr, byte Status, byte secUse );
#endif
#if defined ( REFLECTOR ) || defined( ZDO_COORDINATOR )
  static byte ZDO_CompareClusterLists( byte numList1, uint16 *list1,
                                byte numList2, uint16 *list2, uint16 *pMatches );
#endif
#if defined ( ZDO_COORDINATOR )
  static void ZDO_RemoveMatchMemory( void );
  static uint8 ZDO_CopyMatchInfo( ZDEndDeviceBind_t *destReq, ZDEndDeviceBind_t *srcReq );
  static uint8 ZDMatchSendState( uint8 reason, uint8 status, uint8 TransSeq );
  static void ZDO_EndDeviceBindMatchTimeoutCB( void );
#endif

/*********************************************************************
 * @fn          ZDO_Init
 *
 * @brief       ZDObject and ZDProfile initialization.
 *
 * @param       none
 *
 * @return      none
 */
void ZDO_Init( void )
{
  // Initialize ZD items
  #if defined ( REFLECTOR )
  ZDO_EDBind = NULL;
  #endif

  // Setup the device - type of device to create.
  ZDODeviceSetup();

  // Initialize ZigBee Device Security Manager
  ZDSecMgrInit();
}

#if defined ( MANAGED_SCAN )
/*********************************************************************
 * @fn      ZDOManagedScan_Next()
 *
 * @brief   Setup a managed scan.
 *
 * @param   none
 *
 * @return  none
 */
static void ZDOManagedScan_Next( void )
{
  // Is it the first time
  if ( managedScanNextChannel == 0 && managedScanTimesPerChannel == 0 )
  {
    // Setup the defaults
    managedScanNextChannel  = 1;

    while( managedScanNextChannel && (zgDefaultChannelList & managedScanNextChannel) == 0 )
      managedScanNextChannel <<= 1;

    managedScanChannelMask = managedScanNextChannel;
    managedScanTimesPerChannel = MANAGEDSCAN_TIMES_PRE_CHANNEL;
  }
  else
  {
    // Do we need to go to the next channel
    if ( managedScanTimesPerChannel == 0 )
    {
      // Find next active channel
      managedScanChannelMask  = managedScanNextChannel;
      managedScanTimesPerChannel = MANAGEDSCAN_TIMES_PRE_CHANNEL;
    }
    else
    {
      managedScanTimesPerChannel--;

      if ( managedScanTimesPerChannel == 0 )
      {
        managedScanNextChannel  <<= 1;
        while( managedScanNextChannel && (zgDefaultChannelList & managedScanNextChannel) == 0 )
          managedScanNextChannel <<= 1;

        if ( managedScanNextChannel == 0 )
          zdoDiscCounter  = NUM_DISC_ATTEMPTS + 1; // Stop
      }
    }
  }
}
#endif // MANAGED_SCAN

/*********************************************************************
 * @fn      ZDODeviceSetup()
 *
 * @brief   Call set functions depending on the type of device compiled.
 *
 * @param   none
 *
 * @return  none
 */
static void ZDODeviceSetup( void )
{
#if defined( ZDO_COORDINATOR )
  NLME_CoordinatorInit();
#endif

#if defined ( REFLECTOR )
  #if defined ( ZDO_COORDINATOR )
    APS_ReflectorInit( APS_REFLECTOR_PUBLIC );
  #else
    APS_ReflectorInit( APS_REFLECTOR_PRIVATE );
  #endif
#endif

#if !defined( ZDO_COORDINATOR ) || defined( SOFT_START )
  NLME_DeviceJoiningInit();
#endif
}

/*********************************************************************
 * @fn          ZDO_StartDevice
 *
 * @brief       This function starts a device in a network.
 *
 * @param       logicalType     - Device type to start
 *              startMode       - indicates mode of device startup
 *              beaconOrder     - indicates time betwen beacons
 *              superframeOrder - indicates length of active superframe
 *
 * @return      none
 */
void ZDO_StartDevice( byte logicalType, devStartModes_t startMode, byte beaconOrder, byte superframeOrder )
{
  ZStatus_t ret;

  ret = ZUnsupportedMode;

#if defined(ZDO_COORDINATOR)
  if ( logicalType == NODETYPE_COORDINATOR )
  {
    if ( startMode == MODE_HARD )
    {
      devState = DEV_COORD_STARTING;
      ret = NLME_NetworkFormationRequest( zgConfigPANID, zgDefaultChannelList,
                                          zgDefaultStartingScanDuration, beaconOrder,
                                          superframeOrder, false );
    }
    else if ( startMode == MODE_RESUME )
    {
      // Just start the coordinator
      devState = DEV_COORD_STARTING;
      ret = NLME_StartRouterRequest( beaconOrder, beaconOrder, false );
    }
    else
    {
    }
  }
#endif  // !ZDO_COORDINATOR

#if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
  if ( logicalType == NODETYPE_ROUTER || logicalType == NODETYPE_DEVICE )
  {
    if ( (startMode == MODE_JOIN) || (startMode == MODE_REJOIN) )
    {
      devState = DEV_NWK_DISC;

  #if defined( MANAGED_SCAN )
      ZDOManagedScan_Next();
      ret = NLME_NetworkDiscoveryRequest( managedScanChannelMask, BEACON_ORDER_15_MSEC );
  #else
      ret = NLME_NetworkDiscoveryRequest( zgDefaultChannelList, zgDefaultStartingScanDuration );
  #endif
    }
    else if ( startMode == MODE_RESUME )
    {
      if ( logicalType == NODETYPE_ROUTER )
      {
        ZMacScanCnf_t scanCnf;
        devState = DEV_NWK_ORPHAN;

        /* if router and nvram is available, fake successful orphan scan */
        scanCnf.hdr.Status = ZSUCCESS;
        scanCnf.ScanType = ZMAC_ORPHAN_SCAN;
        scanCnf.UnscannedChannels = 0;
        scanCnf.ResultListSize = 0;
        nwk_ScanJoiningOrphan(&scanCnf);

        ret = ZSuccess;
      }
      else
      {
        devState = DEV_NWK_ORPHAN;
        ret = NLME_OrphanJoinRequest( zgDefaultChannelList,
                                      zgDefaultStartingScanDuration );
      }
    }
    else
    {
    }
  }
#endif  //!ZDO COORDINATOR || SOFT_START

  // configure the Security Manager for type of device
  ZDSecMgrConfig();

  if ( ret != ZSuccess )
    osal_start_timer( ZDO_NETWORK_INIT, NWK_RETRY_DELAY );
}

/*********************************************************************
 * @fn      ZDO_UpdateNwkStatus()
 *
 * @brief
 *
 *   This function will send an update message to each registered
 *   application endpoint/interface about a network status change.
 *
 * @param   none
 *
 * @return  none
 */
void ZDO_UpdateNwkStatus( devStates_t state )
{
  // Endpoint/Interface descriptor list.
  epList_t *epDesc = epList;
  byte bufLen = sizeof(osal_event_hdr_t);
  osal_event_hdr_t *msgPtr;

  ZDAppNwkAddr.addr.shortAddr = NLME_GetShortAddr();
  (void)NLME_GetExtAddr();  // Load the saveExtAddr pointer.

  while ( epDesc )
  {
    if ( epDesc->epDesc->endPoint != ZDO_EP )
    {
      msgPtr = (osal_event_hdr_t *)osal_msg_allocate( bufLen );
      if ( msgPtr )
      {
        msgPtr->event = ZDO_STATE_CHANGE; // Command ID
        msgPtr->status = (byte)state;

        osal_msg_send( *(epDesc->epDesc->task_id), (byte *)msgPtr );
      }
    }
    epDesc = epDesc->nextDesc;
  }
}

#if defined ( REFLECTOR )
/*********************************************************************
 * @fn          ZDO_RemoveEndDeviceBind
 *
 * @brief       Remove the end device bind
 *
 * @param  none
 *
 * @return      none
 */
static void ZDO_RemoveEndDeviceBind( void )
{
  if ( ZDO_EDBind )
  {
    // Free the RAM
    if ( ZDO_EDBind->inClusters )
      osal_mem_free( ZDO_EDBind->inClusters );
    if ( ZDO_EDBind->outClusters )
      osal_mem_free( ZDO_EDBind->outClusters );
    osal_mem_free( ZDO_EDBind );
    ZDO_EDBind = NULL;
  }
}
#endif // REFLECTOR

#if defined ( REFLECTOR )
/*********************************************************************
 * @fn          ZDO_RemoveEndDeviceBind
 *
 * @brief       Remove the end device bind
 *
 * @param  none
 *
 * @return      none
 */
static void ZDO_SendEDBindRsp( byte TransSeq, zAddrType_t *dstAddr, byte Status, byte secUse )
{
  ZDP_EndDeviceBindRsp( TransSeq, dstAddr, Status, secUse );
}
#endif // REFLECTOR

#if defined ( REFLECTOR ) || defined ( ZDO_COORDINATOR )
/*********************************************************************
 * @fn          ZDO_CompareClusterLists
 *
 * @brief       Compare one list to another list
 *
 * @param       numList1 - number of items in list 1
 * @param       list1 - first list of cluster IDs
 * @param       numList2 - number of items in list 2
 * @param       list2 - second list of cluster IDs
 * @param       pMatches - buffer to put matches
 *
 * @return      number of matches
 */
static byte ZDO_CompareClusterLists( byte numList1, uint16 *list1,
                          byte numList2, uint16 *list2, uint16 *pMatches )
{
  byte x, y;
  uint16 z;
  byte numMatches = 0;

  // Check the first in against the seconds out
  for ( x = 0; x < numList1; x++ )
  {
    for ( y = 0; y < numList2; y++ )
    {
      z = list2[y];
      if ( list1[x] == z )
        pMatches[numMatches++] = z;
    }
  }

  return ( numMatches );
}
#endif // REFLECTOR || ZDO_COORDINATOR

#if defined ( REFLECTOR )
/*********************************************************************
 * @fn          ZDO_DoEndDeviceBind
 *
 * @brief       Process the End Device Bind Req from ZDApp
 *
 * @param  bindReq  - Bind Request Information
 * @param  SecurityUse - Security enable/disable
 *
 * @return      none
 */
void ZDO_DoEndDeviceBind( ZDEndDeviceBind_t *bindReq )
{
  uint8 numMatches;
  uint8 Status;
  BindingEntry_t *pBind;
  AddrMgrEntry_t addrEntry;
  zAddrType_t SrcAddr;

  SrcAddr.addrMode = Addr16Bit;
  SrcAddr.addr.shortAddr = bindReq->srcAddr;

  // Ask for IEEE address
  if ( (bindReq->srcAddr != ZDAppNwkAddr.addr.shortAddr) )
  {
    addrEntry.user = ADDRMGR_USER_BINDING;
    addrEntry.nwkAddr = bindReq->srcAddr;
    Status = AddrMgrEntryLookupNwk( &addrEntry );
    if ( Status == TRUE)
    {
      // Add a reference to entry
      AddrMgrEntryAddRef( &addrEntry );
    }
    else
    {
      // If we have the extended address
      if ( NLME_GetProtocolVersion() != ZB_PROT_V1_0 )
      {
        osal_cpyExtAddr( addrEntry.extAddr, bindReq->ieeeAddr );
      }

      // Not in address manager?
      AddrMgrEntryUpdate( &addrEntry );   // Add it
    }

    if ( AddrMgrExtAddrValid( addrEntry.extAddr ) == FALSE )
    {
      ZDP_IEEEAddrReq( bindReq->srcAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, false );
    }
  }

  if ( ZDO_EDBind )   // End Device Bind in progress
  {
    Status = ZDP_NO_MATCH;

    if ( bindReq->profileID == ZDO_EDBind->ProfileID )
    {
      // Check the first in against the seconds out
      numMatches = ZDO_CompareClusterLists(
                  ZDO_EDBind->numOutClusters, ZDO_EDBind->outClusters,
                  bindReq->numInClusters, bindReq->inClusters, ZDOBuildBuf );

      if ( numMatches )
      {
        // if existing bind exists, remove it
        pBind = bindFindExisting( &(ZDO_EDBind->SrcAddr), ZDO_EDBind->epIntf,
                      &SrcAddr, bindReq->endpoint );
        if ( pBind )
        {
          bindRemoveEntry( pBind );
          Status = ZDP_SUCCESS;
        }
        // else add new binding table entry
        else if ( bindAddEntry( &(ZDO_EDBind->SrcAddr), ZDO_EDBind->epIntf,
                      &SrcAddr, bindReq->endpoint, numMatches, ZDOBuildBuf ) )
          Status = ZDP_SUCCESS;
        else
          Status = ZDP_TABLE_FULL;
      }

      // Check the second in against the first out
      numMatches = ZDO_CompareClusterLists( bindReq->numOutClusters, bindReq->outClusters,
                      ZDO_EDBind->numInClusters, ZDO_EDBind->inClusters,
                      ZDOBuildBuf );

      if ( numMatches )
      {
        // if existing bind exists, remove it
        pBind = bindFindExisting( &SrcAddr, bindReq->endpoint, &(ZDO_EDBind->SrcAddr),
                      ZDO_EDBind->epIntf );
        if ( pBind )
        {
          bindRemoveEntry( pBind );
          Status = ZDP_SUCCESS;
        }
        // else add new binding table entry
        else if ( bindAddEntry( &SrcAddr, bindReq->endpoint, &(ZDO_EDBind->SrcAddr),
                      ZDO_EDBind->epIntf, numMatches, ZDOBuildBuf ) )
          Status = ZDP_SUCCESS;
        else
          Status = ZDP_TABLE_FULL;
      }
    }

    if ( Status == ZDP_SUCCESS )
    {
      // We've found a match, so we don't have to wait for the timeout
      APS_SetEndDeviceBindTimeout( 10, ZDO_EndDeviceTimeoutCB );  // psuedo stop end device timeout

        // Notify to save info into NV
      osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );
    }

    ZDO_EDBind->status = Status;

    // Send the response message to the device sending this message
    ZDO_SendEDBindRsp( bindReq->TransSeq, &SrcAddr, Status, bindReq->SecurityUse );
  }
  else  // Start a new End Device Bind
  {
    // Copy the info
    ZDO_EDBind = osal_mem_alloc( sizeof( ZDO_EDBind_t ) );
    if ( ZDO_EDBind )
    {
      osal_memcpy( &(ZDO_EDBind->SrcAddr), &SrcAddr, sizeof( zAddrType_t ) );
      ZDO_EDBind->LocalCoordinator = bindReq->localCoordinator;
      ZDO_EDBind->epIntf = bindReq->endpoint;
      ZDO_EDBind->ProfileID = bindReq->profileID;
      ZDO_EDBind->SrcTransSeq = bindReq->TransSeq;

      ZDO_EDBind->numInClusters = bindReq->numInClusters;
      if ( bindReq->numInClusters )
      {
        ZDO_EDBind->inClusters = osal_mem_alloc( (short)(bindReq->numInClusters * sizeof(uint16)) );
        if ( ZDO_EDBind->inClusters )
        {
          osal_memcpy( ZDO_EDBind->inClusters, bindReq->inClusters, (bindReq->numInClusters * sizeof( uint16 )) );
        }
        else
        {
          // Force no clusters
          ZDO_EDBind->numInClusters = 0;
        }
      }
      else
        ZDO_EDBind->inClusters = NULL;

      ZDO_EDBind->numOutClusters = bindReq->numOutClusters;
      if ( bindReq->numOutClusters )
      {
        ZDO_EDBind->outClusters = osal_mem_alloc( (short)(bindReq->numOutClusters*sizeof(uint16)) );
        if ( ZDO_EDBind->outClusters )
        {
          osal_memcpy( ZDO_EDBind->outClusters, bindReq->outClusters, (bindReq->numOutClusters * sizeof( uint16 )) );
        }
        else
        {
          ZDO_EDBind->numOutClusters = 0;
        }
      }
      else
        ZDO_EDBind->outClusters = NULL;

      ZDO_EDBind->SecurityUse = bindReq->SecurityUse;
      ZDO_EDBind->status = ZDP_TIMEOUT;

      // Setup the timer
      APS_SetEndDeviceBindTimeout( AIB_MaxBindingTime, ZDO_EndDeviceTimeoutCB );
    }
  }
}
#endif // REFLECTOR

/*********************************************************************
 * Utility functions
 */

/*********************************************************************
 * @fn          ZDO_CreateAlignedUINT16List
 *
 * @brief       Creates a list of cluster IDs that is guaranteed to be aligned.
 *              according to the needs of the target. If thre device is running
 *              Protocol version 1.0 the incoming buffer will have only a single
 *              byte for the cluster ID.
 *
 *              Depends on the malloc taking care of alignment.
 *
 *              When cluster ID went to 16 bits alignment for cluster IDs became
 *              an issue.
 *
 * @param       num  - number of entries in list
 * @param       buf  - pointer to list
 *
 * @return      pointer to aligned list. Null if can't allocate memory.
 *              Caller's responsibility to free memory.
 */
static uint16 *ZDO_CreateAlignedUINT16List(uint8 num, uint8 *buf)
{
  uint16 *ptr;

  if ((ptr=osal_mem_alloc((short)(num*sizeof(uint16)))))  {
    uint8 i, ubyte, inc;

    inc = (ZB_PROT_V1_1 == NLME_GetProtocolVersion()) ? 2 : 1;

    for (i=0; i<num; ++i)  {
      // set upper byte to 0 if we're talking Version 1.0. otherwise
      // the buffer contains 16 bit cluster IDs.
      ubyte  = (2 == inc) ? buf[1] : 0;
      ptr[i] = BUILD_UINT16(buf[0], ubyte);
      buf    += inc;
    }
  }

  return ptr;
}

/*********************************************************************
 * @fn          ZDO_CompareByteLists
 *
 * @brief       Compares two lists for matches.
 *
 * @param       ACnt  - number of entries in list A
 * @param       AList  - List A
 * @param       BCnt  - number of entries in list B
 * @param       BList  - List B
 *
 * @return      true if a match is found
 */
byte ZDO_AnyClusterMatches( byte ACnt, uint16 *AList, byte BCnt, uint16 *BList )
{
  byte x, y;

  for ( x = 0; x < ACnt; x++ )
  {
    for ( y = 0; y < BCnt; y++ )
    {
      if ( AList[x] == BList[y] )
      {
        return true;
      }
    }
  }

  return false;
}

/*********************************************************************
 * Callback functions from ZDProfile
 */

/*********************************************************************
 * @fn          ZDO_ProcessNodeDescReq
 *
 * @brief       This function processes and responds to the
 *              Node_Desc_req message.
 *
 * @param       src  - Source address
 * @param       msg - NWKAddrOfInterest
 * @param       sty - Security enable/disable
 *
 * @return      none
 */
void ZDO_ProcessNodeDescReq( byte seq, zAddrType_t *src, byte *msg, byte sty )
{
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );
  NodeDescriptorFormat_t *desc = NULL;
  byte stat = ZDP_INVALID_REQTYPE;

  if ( aoi == ZDAppNwkAddr.addr.shortAddr )
  {
    desc = &ZDO_Config_Node_Descriptor;
  }
#if defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 )
  else if ( CACHE_SERVER )
  {
    desc = (NodeDescriptorFormat_t *)ZDCacheGetDesc( aoi, eNodeDesc, &stat );
  }
#endif

  if ( desc != NULL )
  {
    ZDP_NodeDescMsg( seq, src, aoi, desc, sty );
  }
  else
  {
    ZDP_GenericRsp( seq, src, stat, aoi, Node_Desc_rsp, sty );
  }
}

/*********************************************************************
 * @fn          ZDO_ProcessPowerDescReq
 *
 * @brief       This function processes and responds to the
 *              Node_Power_req message.
 *
 * @param       src  - Source address
 * @param       msg - NWKAddrOfInterest
 * @param       sty - Security enable/disable
 *
 * @return      none
 */
void ZDO_ProcessPowerDescReq( byte seq, zAddrType_t *src, byte *msg, byte sty )
{
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );
  NodePowerDescriptorFormat_t *desc = NULL;
  byte stat = ZDP_INVALID_REQTYPE;

  if ( aoi == ZDAppNwkAddr.addr.shortAddr )
  {
    desc = &ZDO_Config_Power_Descriptor;
  }
#if defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 )
  else if ( CACHE_SERVER )
  {
    desc = (NodePowerDescriptorFormat_t *)ZDCacheGetDesc(aoi,ePowerDesc,&stat);
  }
#endif

  if ( desc != NULL )
  {
    ZDP_PowerDescMsg( seq, src, aoi, desc, sty );
  }
  else
  {
    ZDP_GenericRsp( seq, src, stat, aoi, Power_Desc_rsp, sty );
  }
}

/*********************************************************************
 * @fn          ZDO_ProcessSimpleDescReq
 *
 * @brief       This function processes and responds to the
 *              Simple_Desc_req message.
 *
 * @param       src - Source address
 * @param       msg - message data
 * @param       sty - Security enable/disable
 *
 * @return      none
 */
void ZDO_ProcessSimpleDescReq( byte seq, zAddrType_t *src, byte *msg, byte sty )
{
  SimpleDescriptionFormat_t *sDesc = NULL;
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );
  byte endPoint = msg[2];
  byte free = false;
  byte stat = ZDP_SUCCESS;

  if ( (endPoint == ZDO_EP) || (endPoint > MAX_ENDPOINTS) )
  {
    stat = ZDP_INVALID_EP;
  }
  else if ( aoi == ZDAppNwkAddr.addr.shortAddr )
  {
    free = afFindSimpleDesc( &sDesc, endPoint );
    if ( sDesc == NULL )
    {
      stat = ZDP_NOT_ACTIVE;
    }
  }
#if defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 )
  else if ( CACHE_SERVER )
  {
    stat = endPoint;
    sDesc = (SimpleDescriptionFormat_t *)ZDCacheGetDesc(aoi, eSimpDesc, &stat);
  }
#endif
  else
  {
#if defined ( RTR_NWK )
    stat = ZDP_DEVICE_NOT_FOUND;
#else
    stat = ZDP_INVALID_REQTYPE;
#endif
  }

  ZDP_SimpleDescMsg( seq, src, stat, sDesc, Simple_Desc_rsp, sty );

  if ( free )
  {
    osal_mem_free( sDesc );
  }
}

/*********************************************************************
 * @fn          ZDO_ProcessActiveEPReq
 *
 * @brief       This function processes and responds to the
 *              Active_EP_req message.
 *
 * @param       src  - Source address
 * @param       sty - Security enable/disable
 *
 * @return      none
 */
void ZDO_ProcessActiveEPReq( byte seq, zAddrType_t *src, byte *msg, byte sty )
{
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );
  byte cnt = CACHE_EP_MAX;
  byte stat = ZDP_SUCCESS;

  if ( aoi == ZDAppNwkAddr.addr.shortAddr )
  {
    cnt = afNumEndPoints() - 1;  // -1 for ZDO endpoint descriptor
    afEndPoints( (uint8 *)ZDOBuildBuf, true );
  }
#if defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 )
  else if ( CACHE_SERVER )
  {
    cnt = *((byte *)ZDCacheGetDesc(aoi, eActEPDesc, (uint8 *)ZDOBuildBuf));
    // If cnt = 0, err code in 1st byte of buf, otherwise EP list is in the buf.
    if ( cnt == 0 )
    {
      stat = ZDOBuildBuf[0];
    }
  }
#endif
  else
  {
    stat = ZDP_INVALID_REQTYPE;
  }

  if ( cnt != CACHE_EP_MAX )
  {
    ZDP_ActiveEPRsp( seq, src, stat, aoi, cnt, (uint8 *)ZDOBuildBuf, sty );
  }
  else
  {
    ZDP_GenericRsp( seq, src, ZDP_NOT_SUPPORTED, aoi, Active_EP_rsp, sty );
  }
}

/*********************************************************************
 * @fn          ZDO_ProcessMatchDescReq
 *
 * @brief       This function processes and responds to the
 *              Match_Desc_req message.
 *
 * @param       src  - Source address
 * @param       msg - input message containing search material
 * @param       sty - Security enable/disable
 *
 * @return      none
 */
void ZDO_ProcessMatchDescReq( byte seq, zAddrType_t *src, byte *msg, byte sty )
{
  byte epCnt = 0;
  byte numInClusters;
  uint16 *inClusters;
  byte numOutClusters;
  uint16 *outClusters;
  epList_t *epDesc;
  SimpleDescriptionFormat_t *sDesc = NULL;
  uint8 allocated;

  // Parse the incoming message
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );
  uint16 profileID = BUILD_UINT16( msg[2], msg[3] );
  msg += 4;
  numInClusters = *msg++;
  inClusters = NULL;
  if (numInClusters)  {
    if (!(inClusters=ZDO_CreateAlignedUINT16List(numInClusters, msg)))  {
      // can't allocate memory. drop message
      return;
    }
  }
  msg += numInClusters*sizeof(uint16);

  numOutClusters = *msg++;
  outClusters = NULL;
  if (numOutClusters)  {
    if (!(outClusters=ZDO_CreateAlignedUINT16List(numOutClusters, msg)))  {
      // can't allocate memory. drop message
      if (inClusters) {
        osal_mem_free(inClusters);
      }
      return;
    }
  }
  msg += numOutClusters*sizeof(uint16);

  if ( NWK_BROADCAST_SHORTADDR_DEVALL == aoi )
  {
#if defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 )
    if ( CACHE_SERVER )
    {
      ZDCacheProcessMatchDescReq( seq, src, numInClusters, inClusters,
                            numOutClusters, outClusters, profileID, aoi, sty );
    }
#endif
  }
  else if ( ADDR_BCAST_NOT_ME == NLME_IsAddressBroadcast(aoi) )
  {
    ZDP_MatchDescRsp( seq, src, ZDP_INVALID_REQTYPE,
                                   ZDAppNwkAddr.addr.shortAddr, 0, NULL, sty );
    if (inClusters)  {
      osal_mem_free(inClusters);
    }
    if (outClusters)  {
      osal_mem_free(outClusters);
    }
    return;
  }
  else if ( (ADDR_NOT_BCAST == NLME_IsAddressBroadcast(aoi)) && (aoi != ZDAppNwkAddr.addr.shortAddr) )
  {
#if defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 )
    if ( CACHE_SERVER )
    {
      ZDCacheProcessMatchDescReq( seq, src, numInClusters, inClusters,
                            numOutClusters, outClusters, profileID, aoi, sty );
    }
#else
    ZDP_MatchDescRsp( seq, src, ZDP_INVALID_REQTYPE,
                                   ZDAppNwkAddr.addr.shortAddr, 0, NULL, sty );
#endif
    if (inClusters)  {
      osal_mem_free(inClusters);
    }
    if (outClusters)  {
      osal_mem_free(outClusters);
    }
    return;
  }

  // First count the number of endpoints that match.
  epDesc = epList;
  while ( epDesc )
  {
    // Don't search endpoint 0 and check if response is allowed
    if ( epDesc->epDesc->endPoint != ZDO_EP && (epDesc->flags&eEP_AllowMatch) )
    {
      if ( epDesc->pfnDescCB )
      {
        sDesc = (SimpleDescriptionFormat_t *)epDesc->pfnDescCB( AF_DESCRIPTOR_SIMPLE, epDesc->epDesc->endPoint );
        allocated = TRUE;
      }
      else
      {
        sDesc = epDesc->epDesc->simpleDesc;
        allocated = FALSE;
      }

      if ( sDesc && sDesc->AppProfId == profileID )
      {
        uint8 *uint8Buf = (uint8 *)ZDOBuildBuf;

        // If there are no search input/ouput clusters - respond
        if ( ((numInClusters == 0) && (numOutClusters == 0))
            // Are there matching input clusters?
             || (ZDO_AnyClusterMatches( numInClusters, inClusters,
                  sDesc->AppNumInClusters, sDesc->pAppInClusterList ))
            // Are there matching output clusters?
             || (ZDO_AnyClusterMatches( numOutClusters, outClusters,
                  sDesc->AppNumOutClusters, sDesc->pAppOutClusterList ))     )
        {
          // Notify the endpoint of the match.
          uint8 bufLen = sizeof( ZDO_MatchDescRspSent_t ) + (numOutClusters + numInClusters) * sizeof(uint16);
          ZDO_MatchDescRspSent_t *pRspSent = (ZDO_MatchDescRspSent_t *) osal_msg_allocate( bufLen );

          if (pRspSent)
          {
            pRspSent->hdr.event = ZDO_MATCH_DESC_RSP_SENT;
            pRspSent->nwkAddr = src->addr.shortAddr;
            pRspSent->numInClusters = numInClusters;
            pRspSent->numOutClusters = numOutClusters;

            if (numInClusters)
            {
              pRspSent->pInClusters = (uint16*) (pRspSent + 1);
              osal_memcpy(pRspSent->pInClusters, inClusters, numInClusters * sizeof(uint16));
            }
            else
            {
              pRspSent->pInClusters = NULL;
            }

            if (numOutClusters)
            {
              pRspSent->pOutClusters = (uint16*)(pRspSent + 1) + numInClusters;
              osal_memcpy(pRspSent->pOutClusters, outClusters, numOutClusters * sizeof(uint16));
            }
            else
            {
              pRspSent->pOutClusters = NULL;
            }

            osal_msg_send( *epDesc->epDesc->task_id, (uint8 *)pRspSent );
          }

          uint8Buf[epCnt++] = sDesc->EndPoint;
        }
      }

      if ( allocated )
        osal_mem_free( sDesc );
    }
    epDesc = epDesc->nextDesc;
  }

  // Send the message only if at least one match found.
  if ( epCnt )
  {
    if ( ZSuccess == ZDP_MatchDescRsp( seq, src, ZDP_SUCCESS,
                       ZDAppNwkAddr.addr.shortAddr, epCnt, (uint8 *)ZDOBuildBuf, sty ) )
    {
    }
  }
  else
  {
  }
  if (inClusters)  {
    osal_mem_free(inClusters);
  }
  if (outClusters)  {
    osal_mem_free(outClusters);
  }
}

#if defined ( ZDO_COORDINATOR )
/*********************************************************************
 * @fn          ZDO_ProcessEndDeviceBindReq
 *
 * @brief       This function processes and responds to the
 *              End_Device_Bind_req message.
 *
 * @param       SrcAddr  - Source address
 * @param       msg - input message containing search material
 * @param       SecurityUse - Security enable/disable
 *
 * @return      none
 */
void ZDO_ProcessEndDeviceBindReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                                  byte SecurityUse )
{
  ZDEndDeviceBind_t bindReq;
  uint8  protoVer;

  protoVer = NLME_GetProtocolVersion();

  // Parse the message
  bindReq.TransSeq = TransSeq;
  bindReq.srcAddr = SrcAddr->addr.shortAddr;
  bindReq.SecurityUse = SecurityUse;

  bindReq.localCoordinator = BUILD_UINT16( msg[0], msg[1] );
  msg += 2;

  if ( protoVer != ZB_PROT_V1_0 )
  {
    osal_cpyExtAddr( &(bindReq.ieeeAddr), msg );
    msg += Z_EXTADDR_LEN;
  }

  bindReq.endpoint = *msg++;
  bindReq.profileID = BUILD_UINT16( msg[0], msg[1] );
  msg += 2;

  bindReq.numInClusters = *msg++;
  bindReq.inClusters = NULL;
  if ( bindReq.numInClusters )
  {
    if ( !(bindReq.inClusters = ZDO_CreateAlignedUINT16List( bindReq.numInClusters, msg )) )
    {
      // can't allocate memory. drop message
      return;
    }
  }
  msg += (bindReq.numInClusters * ((protoVer != ZB_PROT_V1_0) ? sizeof ( uint16 ) : sizeof( uint8 )));

  bindReq.numOutClusters = *msg++;
  bindReq.outClusters = NULL;
  if ( bindReq.numOutClusters )
  {
    if ( !(bindReq.outClusters=ZDO_CreateAlignedUINT16List( bindReq.numOutClusters, msg )) )
    {
      // can't allocate memory. drop message
      if ( bindReq.inClusters )
      {
        osal_mem_free( bindReq.inClusters );
      }
      return;
    }
  }

  ZDApp_EndDeviceBindReqCB( &bindReq );

  if ( bindReq.inClusters )
  {
    osal_mem_free( bindReq.inClusters );
  }
  if ( bindReq.outClusters )
  {
    osal_mem_free( bindReq.outClusters );
  }
}
#endif // ZDO_COORDINATOR

#if defined ( REFLECTOR ) || defined ( ZDO_BIND_UNBIND_RESPONSE )

/*********************************************************************
 * @fn          ZDO_ProcessBindUnbindReq
 *
 * @brief       This function processes and responds to the
 *              Bind_req or Unbind_req message.
 *
 * @param       SrcAddr  - Source address
 * @param       msgClusterID - message cluster ID
 * @param       msg - input message containing search material
 * @param       SecurityUse - Security enable/disable
 *
 * @return      none
 */
void ZDO_ProcessBindUnbindReq( byte TransSeq, zAddrType_t *SrcAddr, uint16 msgClusterID,
                              byte *msg, byte SecurityUse )
{
  byte *SrcAddress;
  byte SrcEpIntf;
  uint16 ClusterID;
  zAddrType_t DstAddress;
  byte DstEpIntf;
  uint8 protoVer;

  protoVer = NLME_GetProtocolVersion();

  SrcAddress = msg;
  msg += Z_EXTADDR_LEN;
  SrcEpIntf = *msg++;

  if ( protoVer != ZB_PROT_V1_0 )
  {
    ClusterID = BUILD_UINT16( msg[0], msg[1] );
    msg += 2;
  }
  else
  {
    ClusterID = *msg++;
  }

  if ( protoVer != ZB_PROT_V1_0 )
  {
    DstAddress.addrMode = *msg++;
    if ( DstAddress.addrMode == Addr64Bit )
    {
      osal_cpyExtAddr( DstAddress.addr.extAddr, msg );
      msg += Z_EXTADDR_LEN;
      DstEpIntf = *msg;
    }
    else
    {
      DstAddress.addr.shortAddr = BUILD_UINT16( msg[0], msg[1] );
      msg += sizeof ( uint16 );
    }
  }
  else
  {
    DstAddress.addrMode = Addr64Bit;
    osal_cpyExtAddr( DstAddress.addr.extAddr, msg );
    msg += Z_EXTADDR_LEN;
    DstEpIntf = *msg;
  }


  if ( msgClusterID == Bind_req )
  {
    ZDApp_BindReqCB( TransSeq, SrcAddr, SrcAddress, SrcEpIntf,
                    ClusterID, &DstAddress, DstEpIntf, SecurityUse );
  }
  else
  {
    ZDApp_UnbindReqCB( TransSeq, SrcAddr, SrcAddress, SrcEpIntf,
                    ClusterID, &DstAddress, DstEpIntf, SecurityUse );
  }
}
#endif // REFLECTOR || ZDO_BIND_UNBIND_RESPONSE

#if defined ( ZDO_NWKADDR_REQUEST ) || defined ( ZDO_IEEEADDR_REQUEST ) || defined ( REFLECTOR )
/*********************************************************************
 * @fn      ZDO_ProcessAddrRsp
 *
 * @brief   Process an incoming NWK_addr_rsp or IEEE_addr_rsp message and then
 *          invoke the corresponding CB function.
 *
 * @param   src - Source address of the request.
 * @param   cId - Cluster ID of the request.
 * @param   msg - Incoming request message.
 *
 * @return  none
 */
void ZDO_ProcessAddrRsp( zAddrType_t *src, uint16 cId, byte *msg, byte msgLen )
{
#if defined ( REFLECTOR )
  AddrMgrEntry_t addrEntry;
#endif
  uint16 aoi;
  uint16 *list = NULL;
  byte idx = 0;
  byte cnt = 0;

  byte stat = *msg++;
  byte *ieee = msg;
  msg += Z_EXTADDR_LEN;
  aoi = BUILD_UINT16( msg[0], msg[1] );

#if defined ( REFLECTOR )
  // Add this to the address manager
  addrEntry.user = ADDRMGR_USER_DEFAULT;
  addrEntry.nwkAddr = aoi;
  AddrMgrExtAddrSet( addrEntry.extAddr, ieee );
  AddrMgrEntryUpdate( &addrEntry );
#endif

  // NumAssocDev field is only present on success.
  if ( stat == ZDO_SUCCESS )
  {
    msg += 2;
    cnt = ( msgLen > 1 + Z_EXTADDR_LEN + 2 ) ? *msg++ : 0;   // Single req: msgLen = status + IEEEAddr + NWKAddr

    // StartIndex field is only present if NumAssocDev field is non-zero.
    if ( cnt != 0 )
    {
      idx = *msg++;

      if ( cnt > idx )
      {
        list = osal_mem_alloc( (short)(cnt * sizeof( uint16 )) );

        if ( list )
        {
          uint16 *pList = list;
          byte n = cnt - idx;

          while ( n != 0 )
          {
            *pList++ = BUILD_UINT16( msg[0], msg[1] );
            msg += sizeof( uint16 );
            n--;
          }
        }
      }
    }
  }

#if defined ( ZDO_NWKADDR_REQUEST )
  if ( cId == NWK_addr_rsp )
  {
    ZDApp_NwkAddrRspCB( src, stat, ieee, aoi, cnt, idx, list );
  }
#endif

#if defined ( ZDO_IEEEADDR_REQUEST )
  if ( cId == IEEE_addr_rsp )
  {
    ZDApp_IEEEAddrRspCB( src, stat, ieee, aoi, cnt, idx, list );
  }
#endif

  if ( list )
  {
    osal_mem_free( list );
  }
}
#endif // ZDO_NWKADDR_REQUEST ZDO_IEEEADDR_REQUEST

#if defined ( ZDO_NODEDESC_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessNodeDescRsp
 *
 * @brief       This function processes and responds to the
 *              Node_Desc_rsp message.
 *
 * @param       SrcAddr  - Source address
 * @param       msg - input message containing search material
 *
 * @return      none
 */
void ZDO_ProcessNodeDescRsp( zAddrType_t *SrcAddr, byte *msg )
{
  byte proVer = NLME_GetProtocolVersion();
  NodeDescriptorFormat_t nodeDesc;
  NodeDescriptorFormat_t *pNodeDesc = NULL;
  byte Status = *msg++;
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );

  if ( Status == ZDP_SUCCESS )
  {
    msg += 2;
    nodeDesc.LogicalType = *msg & 0x07;
    if ( proVer == ZB_PROT_V1_0 )
    {
      nodeDesc.UserDescAvail = 0;
      nodeDesc.ComplexDescAvail = 0;
    }
    else
    {
      nodeDesc.ComplexDescAvail = ( *msg & 0x08 ) >> 3;
      nodeDesc.UserDescAvail = ( *msg & 0x10 ) >> 4;
    }
    msg++;  // Reserved bits.
    nodeDesc.FrequencyBand = (*msg >> 3) & 0x1f;
    nodeDesc.APSFlags = *msg++ & 0x07;
    nodeDesc.CapabilityFlags = *msg++;
    nodeDesc.ManufacturerCode[0] = *msg++;
    nodeDesc.ManufacturerCode[1] = *msg++;
    nodeDesc.MaxBufferSize = *msg++;
    nodeDesc.MaxTransferSize[0] = *msg++;
    nodeDesc.MaxTransferSize[1] = *msg++;

    if ( proVer == ZB_PROT_V1_0)
    {
      nodeDesc.ServerMask = 0;
    }
    else
    {
      nodeDesc.ServerMask = BUILD_UINT16( msg[0], msg[1] );
    }

    pNodeDesc = &nodeDesc;
  }

  ZDApp_NodeDescRspCB( SrcAddr, Status, aoi, pNodeDesc );
}
#endif // ZDO_NODEDESC_REQUEST

#if defined ( ZDO_POWERDESC_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessPowerDescRsp
 *
 * @brief       This function processes and responds to the
 *              Power_Desc_rsp message.
 *
 * @param       SrcAddr  - Source address
 * @param       msg - input message containing search material
 *
 * @return      none
 */
void ZDO_ProcessPowerDescRsp( zAddrType_t *SrcAddr, byte *msg )
{
  NodePowerDescriptorFormat_t pwrDesc;
  NodePowerDescriptorFormat_t *pPwrDesc = NULL;
  byte Status = *msg++;
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );

  if ( Status == ZDP_SUCCESS )
  {
    msg += 2;
    pwrDesc.AvailablePowerSources = *msg >> 4;
    pwrDesc.PowerMode = *msg++ & 0x0F;
    pwrDesc.CurrentPowerSourceLevel = *msg >> 4;
    pwrDesc.CurrentPowerSource = *msg++ & 0x0F;
    pPwrDesc = &pwrDesc;
  }

  ZDApp_PowerDescRspCB( SrcAddr, Status, aoi, pPwrDesc );
}
#endif // ZDO_POWERDESC_REQUEST

#if defined ( ZDO_SIMPLEDESC_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessSimpleDescRsp
 *
 * @brief       This function processes and responds to the
 *              Simple_Desc_rsp message.
 *
 * @param       SrcAddr  - Source address
 * @param       msg - input message containing search material
 *
 * @return      none
 */
void ZDO_ProcessSimpleDescRsp( zAddrType_t *SrcAddr, byte *msg )
{
  byte epIntf = 0;
  SimpleDescriptionFormat_t simpleDesc;
  SimpleDescriptionFormat_t *pSimpleDesc = NULL;
  byte Status = *msg++;
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );

  if ( Status == ZDP_SUCCESS )
  {
    msg += 3;
    epIntf = *msg;
    pSimpleDesc = &simpleDesc;
    ZDO_ParseSimpleDescBuf( msg, pSimpleDesc );
  }

  ZDApp_SimpleDescRspCB( SrcAddr, Status, aoi, epIntf, pSimpleDesc );
}
#endif // ZDO_SIMPLEDESC_REQUEST

#if defined ( ZDO_ACTIVEEP_REQUEST ) || defined ( ZDO_MATCH_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessEPListRsp
 *
 * @brief       This function processes and responds to the
 *              Active_EP_rsp or Match_Desc_rsp message.
 *
 * @param       src  - Source address
 * @param       ClusterID - Active_EP_rsp or Match_Desc_rsp
 * @param       msg - input message containing search material
 *
 * @return      none
 */
void ZDO_ProcessEPListRsp( zAddrType_t *src, uint16 ClusterID, byte *msg )
{
  byte Status = *msg++;
  byte cnt = msg[2];
  byte *list = msg+3;

  src->addr.shortAddr = BUILD_UINT16( msg[0], msg[1] );

#if defined ( ZDO_ACTIVEEP_REQUEST )
  if ( ClusterID == Active_EP_rsp )
    ZDApp_ActiveEPRspCB( src, Status, cnt, list );
#endif

#if defined ( ZDO_MATCH_REQUEST )
  if ( ClusterID == Match_Desc_rsp )
    ZDApp_MatchDescRspCB( src, Status, cnt, list );
#endif
}
#endif  // ZDO_ACTIVEEP_REQUEST ZDO_MATCH_REQUEST

#if defined ( ZDO_BIND_UNBIND_REQUEST ) || defined ( ZDO_ENDDEVICEBIND_REQUEST ) || defined ( ZDO_COORDINATOR )
/*********************************************************************
 * @fn          ZDO_ProcessBindUnBindRsp
 *
 * @brief       This function processes and responds to the
 *              End_Device_Bind_rsp message.
 *
 * @param       SrcAddr  - Source address
 * @param       ClusterID - Active_EP_rsp or Match_Desc_rsp
 * @param       msg - input message containing search material
 *
 * @return      none
 */
void ZDO_ProcessBindUnbindRsp( zAddrType_t *SrcAddr, uint16 ClusterID, byte Status, uint8 TransSeq )
{
#if defined ( ZDO_COORDINATOR )
  uint8 used = FALSE;
#endif

#if defined ( ZDO_ENDDEVICEBIND_REQUEST )
  if ( ClusterID == End_Device_Bind_rsp )
    ZDApp_EndDeviceBindRsp( SrcAddr, Status );
#endif

#if defined ( ZDO_COORDINATOR )
  if ( matchED )
  {
    used = ZDMatchSendState(
           (uint8)((ClusterID == Bind_rsp) ? ZDMATCH_REASON_BIND_RSP : ZDMATCH_REASON_UNBIND_RSP),
           Status, TransSeq );
  }

  if ( !used )
#endif
  {
  #if defined ( ZDO_BIND_UNBIND_REQUEST )
    if ( ClusterID == Bind_rsp )
      ZDApp_BindRsp( SrcAddr, Status );
    else
      ZDApp_UnbindRsp( SrcAddr, Status );
  #endif
  }
}
#endif // ZDO_BIND_UNBIND_REQUEST ZDO_ENDDEVICEBIND_REQUEST

#if defined ( ZDO_SERVERDISC_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessServerDiscRsp
 *
 * @brief       Process the Server_Discovery_rsp message.
 *
 * @param       srcAddr - Source address.
 * @param       msg - Byte array containing the Server_Discovery_rsp command frame.
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessServerDiscRsp(zAddrType_t *srcAddr, byte *msg, byte SecurityUse)
{
  byte status = *msg++;
  uint16 serverMask = BUILD_UINT16( msg[0], msg[1] );

  ZDApp_ServerDiscRspCB( srcAddr->addr.shortAddr, status, serverMask,
                         SecurityUse );
}
#endif

#if defined ( ZDO_SERVERDISC_RESPONSE )
/*********************************************************************
 * @fn          ZDO_ProcessServerDiscReq
 *
 * @brief       Process the Server_Discovery_req message.
 *
 * @param       transID - Transaction sequence number of request.
 * @param       srcAddr  - Source address
 * @param       msg - Byte array containing the Server_Discovery_req command frame.
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessServerDiscReq( byte transID, zAddrType_t *srcAddr, byte *msg,
                               byte SecurityUse )
{
  uint16 serverMask = BUILD_UINT16( msg[0], msg[1] );
  uint16 matchMask = serverMask & ZDO_Config_Node_Descriptor.ServerMask;

  if ( matchMask )
  {
    ZDP_ServerDiscRsp( transID, srcAddr, ZSUCCESS, ZDAppNwkAddr.addr.shortAddr,
                       matchMask, SecurityUse );
  }
}
#endif

/*********************************************************************
 * Call Back Functions from APS  - API
 */

/*********************************************************************
 * @fn          ZDO_EndDeviceTimeoutCB
 *
 * @brief       This function handles the binding timer for the End
 *              Device Bind command.
 *
 * @param       none
 *
 * @return      none
 */
void ZDO_EndDeviceTimeoutCB( void )
{
#if defined ( REFLECTOR )
  byte stat;
  if ( ZDO_EDBind )
  {
    stat = ZDO_EDBind->status;

    // Send the response message to the first sent
    ZDO_SendEDBindRsp( ZDO_EDBind->SrcTransSeq, &(ZDO_EDBind->SrcAddr),
                        stat, ZDO_EDBind->SecurityUse );

    ZDO_RemoveEndDeviceBind();
  }
#endif  // REFLECTOR
}

/*********************************************************************
 * Optional Management Messages
 */

#if defined( ZDO_MGMT_LQI_RESPONSE ) && defined ( RTR_NWK )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtLqiReq
 *
 * @brief       This function handles parsing the incoming Management
 *              LQI request and generate the response.
 *
 *   Note:      This function will limit the number of items returned
 *              to ZDO_MAX_LQI_ITEMS items.
 *
 * @param       SrcAddr - source of the request
 * @param       StartIndex - where to start the return list
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtLqiReq( byte TransSeq, zAddrType_t *SrcAddr, byte StartIndex, byte SecurityUse )
{
  byte x;
  byte index;
  byte numItems;
  byte maxItems;
  ZDP_MgmtLqiItem_t* table;
  ZDP_MgmtLqiItem_t* item;
  neighborEntry_t    entry;
  byte aItems;
  associated_devices_t *aDevice;

  // Get the number of neighbor items
  NLME_GetRequest( nwkNumNeighborTableEntries, 0, &maxItems );

  // Get the number of associated items
  aItems = (uint8)AssocCount( PARENT, CHILD_FFD_RX_IDLE );
  // Total number of items
  maxItems += aItems;

  // Start with the supplied index
  numItems = maxItems - StartIndex;

  // limit the size of the list
  if ( numItems > ZDO_MAX_LQI_ITEMS )
    numItems = ZDO_MAX_LQI_ITEMS;

  // Allocate the memory to build the table
  table = (ZDP_MgmtLqiItem_t*)osal_mem_alloc( (short)
            ( numItems * sizeof( ZDP_MgmtLqiItem_t ) ) );

  if ( table != NULL )
  {
    x = 0;
    item = table;
    index = StartIndex;

    // Loop through associated items and build list
    for ( ; x < numItems; x++ )
    {
      if ( index < aItems )
      {
        // get next associated device
        aDevice = AssocFindDevice( index++ );

        // set basic fields
        item->panID   = _NIB.nwkPanId;
        osal_cpyExtAddr( item->extPanID, _NIB.extendedPANID );
        item->nwkAddr = aDevice->shortAddr;
        item->permit  = ZDP_MGMT_BOOL_UNKNOWN;
        item->depth   = 0xFF;
        item->lqi     = aDevice->linkInfo.rxCost;

        osal_memset( item->extAddr, 0x00, Z_EXTADDR_LEN );

        // use association info to set other fields
        if ( aDevice->nodeRelation == PARENT )
        {
          if (  aDevice->shortAddr == 0 )
          {
            item->devType = ZDP_MGMT_DT_COORD;
          }
          else
          {
            item->devType = ZDP_MGMT_DT_ROUTER;
          }

          item->rxOnIdle = ZDP_MGMT_BOOL_UNKNOWN;
          item->relation = ZDP_MGMT_REL_PARENT;
        }
        else
        {
          if ( aDevice->nodeRelation < CHILD_FFD )
          {
            item->devType = ZDP_MGMT_DT_ENDDEV;

            if ( aDevice->nodeRelation == CHILD_RFD )
            {
              item->rxOnIdle = FALSE;
            }
            else
            {
              item->rxOnIdle = TRUE;
            }
          }
          else
          {
            item->devType = ZDP_MGMT_DT_ROUTER;

            if ( aDevice->nodeRelation == CHILD_FFD )
            {
              item->rxOnIdle = FALSE;
            }
            else
            {
              item->rxOnIdle = TRUE;
            }
          }

          item->relation = ZDP_MGMT_REL_CHILD;
        }

        item++;
      }
      else
      {
        if ( StartIndex <= aItems )
          // Start with 1st neighbor
          index = 0;
        else
          // Start with >1st neighbor
          index = StartIndex - aItems;
        break;
      }
    }

    // Loop through neighbor items and finish list
    for ( ; x < numItems; x++ )
    {
      // Add next neighbor table item
      NLME_GetRequest( nwkNeighborTable, index++, &entry );

      // set ZDP_MgmtLqiItem_t fields
      item->panID    = entry.panId;
      osal_memset( item->extPanID, 0x00, Z_EXTADDR_LEN);
      osal_memset( item->extAddr, 0x00, Z_EXTADDR_LEN );
      item->nwkAddr  = entry.neighborAddress;
      item->rxOnIdle = ZDP_MGMT_BOOL_UNKNOWN;
      item->relation = ZDP_MGMT_REL_UNKNOWN;
      item->permit   = ZDP_MGMT_BOOL_UNKNOWN;
      item->depth    = 0xFF;
      item->lqi      = entry.linkInfo.rxCost;

      if ( item->nwkAddr == 0 )
      {
        item->devType = ZDP_MGMT_DT_COORD;
      }
      else
      {
        item->devType = ZDP_MGMT_DT_ROUTER;
      }

      item++;
    }

    // Send response
    ZDP_MgmtLqiRsp( TransSeq, SrcAddr, ZSuccess, maxItems,
                    StartIndex, numItems, table, false );

    osal_mem_free( table );
  }
}
#endif // ZDO_MGMT_LQI_RESPONSE && RTR_NWK

#if defined ( ZDO_MGMT_LQI_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtLqiRsp
 *
 * @brief       This function handles parsing the incoming Management
 *              LQI response and then generates a callback to the ZD
 *              application.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - buffer holding incoming message to parse
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtLqiRsp( zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  byte x;
  byte status;
  byte startIndex = 0;
  byte neighborLqiCount = 0;
  byte neighborLqiEntries = 0;
  neighborLqiItem_t *list = NULL;
  byte proVer = NLME_GetProtocolVersion();

  status = *msg++;
  if ( status == ZSuccess )
  {
    neighborLqiEntries = *msg++;
    startIndex = *msg++;
    neighborLqiCount = *msg++;

    // Allocate a buffer big enough to handle the list.
    list = (neighborLqiItem_t *)osal_mem_alloc( neighborLqiCount *
                                        sizeof( neighborLqiItem_t ) );
    if ( list )
    {
      neighborLqiItem_t *pList = list;

      for ( x = 0; x < neighborLqiCount; x++ )
      {
        if ( proVer == ZB_PROT_V1_0 )
        {
          pList->PANId = BUILD_UINT16( msg[0], msg[1] );
          msg += 2;
        }
        else
        {
          osal_cpyExtAddr(pList->extPANId, msg);   //Copy extended PAN ID
          msg += Z_EXTADDR_LEN;
        }

        msg += Z_EXTADDR_LEN;  // Throwing away IEEE.
        pList->nwkAddr = BUILD_UINT16( msg[0], msg[1] );
        if ( proVer == ZB_PROT_V1_0 )
          msg += 2 + 1 + 1;          // Skip DeviceType, RxOnIdle, Relationship, PermitJoinging and Depth
        else
          msg += 2 + 1 + 1 + 1;      // Skip DeviceType, RxOnIdle, Rlationship, PermitJoining and Depth

        pList->rxLqi = *msg++;
        pList->txQuality = 0;  // This is not specified OTA by ZigBee 1.1.
        pList++;
      }
    }
  }

  // Call the callback to the application.
  ZDApp_MgmtLqiRspCB( SrcAddr->addr.shortAddr, status, neighborLqiEntries,
                      startIndex, neighborLqiCount, list );

  if ( list )
  {
    osal_mem_free( list );
  }
}
#endif // ZDO_MGMT_LQI_REQUEST

#if defined( ZDO_MGMT_NWKDISC_RESPONSE )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtNwkDiscReq
 *
 * @brief       This function handles parsing the incoming Management
 *              Network Discover request and starts the request.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtNwkDiscReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  NLME_ScanFields_t scan;
  uint8             index;

  scan.channels = BUILD_UINT32( msg[0], msg[1], msg[2], msg[3] );
  msg += 4;
  scan.duration = *msg++;
  index         = *msg;

  // Save off the information to be used for the response
  zdappMgmtNwkDiscReqInProgress          = true;
  zdappMgmtNwkDiscRspAddr.addrMode       = Addr16Bit;
  zdappMgmtNwkDiscRspAddr.addr.shortAddr = SrcAddr->addr.shortAddr;
  zdappMgmtNwkDiscStartIndex             = index;
  zdappMgmtNwkDiscRspTransSeq            = TransSeq;

  if ( NLME_NwkDiscReq2( &scan ) != ZSuccess )
  {
    NLME_NwkDiscTerm();

    // zdappMgmtNwkDiscReqInProgress will be reset in the confirm callback
  }
}
#endif // ZDO_MGMT_NWKDISC_RESPONSE

#if defined ( ZDO_MGMT_NWKDISC_RESPONSE )
/*********************************************************************
 * @fn          ZDO_FinishProcessingMgmtNwkDiscReq
 *
 * @brief       This function finishes the processing of the Management
 *              Network Discover Request and generates the response.
 *
 *   Note:      This function will limit the number of items returned
 *              to ZDO_MAX_NWKDISC_ITEMS items.
 *
 * @param       ResultCountSrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_FinishProcessingMgmtNwkDiscReq( byte ResultCount,
                                         networkDesc_t *NetworkList )
{
  byte count;

#if defined ( RTR_NWK )
  networkDesc_t *newDesc, *pList = NetworkList;

  // Look for my PanID.
  while ( pList )
  {
    if ( pList->panId == _NIB.nwkPanId )
    {
      break;
    }

    if ( !pList->nextDesc )
    {
      break;
    }
    pList = pList->nextDesc;
  }

  // If my Pan not present (query to a star network ZC or an isolated ZR?),
  // prepend it.
  if ( !pList || (pList->panId != _NIB.nwkPanId) )
  {
    newDesc = (networkDesc_t *)osal_mem_alloc( sizeof( networkDesc_t ) );
    if ( newDesc )
    {
      byte pJoin;

      newDesc->panId = _NIB.nwkPanId;
      newDesc->logicalChannel = _NIB.nwkLogicalChannel;
      newDesc->beaconOrder = _NIB.beaconOrder;
      newDesc->superFrameOrder = _NIB.superFrameOrder;
      newDesc->version = NLME_GetProtocolVersion();
      newDesc->stackProfile = zgStackProfile;
      //Extended PanID
      osal_cpyExtAddr( newDesc->extendedPANID, _NIB.extendedPANID);

      ZMacGetReq( ZMacAssociationPermit, &pJoin );
      newDesc->chosenRouter = ((pJoin) ? ZDAppNwkAddr.addr.shortAddr :
                                         INVALID_NODE_ADDR);

      newDesc->nextDesc = NetworkList;
      NetworkList = newDesc;
      ResultCount++;
    }
  }
#endif

  // Calc the count and apply a max count.
  if ( zdappMgmtNwkDiscStartIndex > ResultCount )
  {
    count = 0;
  }
  else
  {
    count = ResultCount - zdappMgmtNwkDiscStartIndex;
    if ( count > ZDO_MAX_NWKDISC_ITEMS )
    {
      count = ZDO_MAX_NWKDISC_ITEMS;
    }

    // Move the list pointer up to the start index.
    NetworkList += zdappMgmtNwkDiscStartIndex;
  }

  ZDP_MgmtNwkDiscRsp( zdappMgmtNwkDiscRspTransSeq,
                     &zdappMgmtNwkDiscRspAddr, ZSuccess, ResultCount,
                      zdappMgmtNwkDiscStartIndex,
                      count,
                      NetworkList,
                      false );

#if defined ( RTR_NWK )
  if ( newDesc )
  {
    osal_mem_free( newDesc );
  }
#endif

  NLME_NwkDiscTerm();
}
#endif

#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessMgmNwkDiscRsp
 *
 * @brief       This function handles parsing the incoming Management
 *              Network Discover response and then generates a callback
 *              to the ZD application.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - buffer holding incoming message to parse
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmNwkDiscRsp( zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  byte x;
  byte status;
  byte networkCount = 0;
  byte startIndex = 0;
  byte networkListCount = 0;
  mgmtNwkDiscItem_t *list = NULL;

  byte proVer = NLME_GetProtocolVersion();

  status = *msg++;
  if ( status == ZSuccess )
  {
    networkCount = *msg++;
    startIndex = *msg++;
    networkListCount = *msg++;

    // Allocate a buffer big enough to handle the list.
    list = (mgmtNwkDiscItem_t *)osal_mem_alloc( networkListCount *
                                        sizeof( mgmtNwkDiscItem_t ) );
    if ( list )
    {
      mgmtNwkDiscItem_t *pList = list;
      for ( x = 0; x < networkListCount; x++ )
      {
        if ( proVer == ZB_PROT_V1_0 )  //Version 1.0
        {
          pList->PANId = BUILD_UINT16( msg[0], msg[1] );
          msg += 2;
        }
        else
        {
          osal_cpyExtAddr(pList->extendedPANID, msg);   //Copy extended PAN ID
          pList->PANId = BUILD_UINT16( msg[0], msg[1] );
          msg += Z_EXTADDR_LEN;

        }
        pList->logicalChannel = *msg++;
        pList->stackProfile = (*msg) & 0x0F;
        pList->version = (*msg++ >> 4) & 0x0F;
        pList->beaconOrder = (*msg) & 0x0F;
        pList->superFrameOrder = (*msg++ >> 4) & 0x0F;
        pList->permitJoining = *msg++;
        pList++;
      }
    }
  }

  // Call the callback to the application.
  ZDApp_MgmtNwkDiscRspCB( SrcAddr->addr.shortAddr, status, networkCount,
                          startIndex, networkListCount, list );

  if ( list )
  {
    osal_mem_free( list );
  }
}
#endif // ZDO_MGMT_NWKDISC_REQUEST

#if defined ( ZDO_MGMT_RTG_RESPONSE ) && defined ( RTR_NWK )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtRtgReq
 *
 * @brief       This function finishes the processing of the Management
 *              Routing Request and generates the response.
 *
 *   Note:      This function will limit the number of items returned
 *              to ZDO_MAX_RTG_ITEMS items.
 *
 * @param       ResultCountSrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtRtgReq( byte TransSeq, zAddrType_t *SrcAddr, byte StartIndex, byte SecurityUse )
{
  byte x;
  byte maxNumItems;
  byte numItems;
  byte *pBuf;
  rtgItem_t *pList;

  // Get the number of table items
  NLME_GetRequest( nwkNumRoutingTableEntries, 0, &maxNumItems );

  numItems = maxNumItems - StartIndex;    // Start at the passed in index

  // limit the size of the list
  if ( numItems > ZDO_MAX_RTG_ITEMS )
    numItems = ZDO_MAX_RTG_ITEMS;

  // Allocate the memory to build the table
  pBuf = osal_mem_alloc( (short)(sizeof( rtgItem_t ) * numItems) );

  if ( pBuf )
  {
    // Convert buffer to list
    pList = (rtgItem_t *)pBuf;

    // Loop through items and build list
    for ( x = 0; x < numItems; x++ )
    {
      NLME_GetRequest( nwkRoutingTable, (uint16)(x + StartIndex), (void*)pList );

      // Remap the status to the RoutingTableList Record Format defined in the ZigBee spec
      switch( pList->status )
      {
        case RT_ACTIVE:
          pList->status = ZDO_MGMT_RTG_ENTRY_ACTIVE;
          break;

        case RT_DISC:
          pList->status = ZDO_MGMT_RTG_ENTRY_DISCOVERY_UNDERWAY;
          break;

        case RT_LINK_FAIL:
          pList->status = ZDO_MGMT_RTG_ENTRY_DISCOVERY_FAILED;

        case RT_INIT:
        case RT_REPAIR:
        default:
          pList->status = ZDO_MGMT_RTG_ENTRY_INACTIVE;
          break;
      }

      // Increment pointer to next record
      pList++;
    }

    // Send response
    ZDP_MgmtRtgRsp( TransSeq, SrcAddr, ZSuccess, maxNumItems, StartIndex, numItems,
                          (rtgItem_t *)pBuf, false );

    osal_mem_free( pBuf );
  }
}
#endif // defined(ZDO_MGMT_RTG_RESPONSE)  && defined(RTR_NWK)

#if defined ( ZDO_MGMT_RTG_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtRtgRsp
 *
 * @brief       This function handles parsing the incoming Management
 *              Routing response and then generates a callback
 *              to the ZD application.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - buffer holding incoming message to parse
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtRtgRsp( zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  byte x;
  byte status;
  byte rtgCount = 0;
  byte startIndex = 0;
  byte rtgListCount = 0;
  byte *pBuf = NULL;
  rtgItem_t *pList = NULL;

  status = *msg++;
  if ( status == ZSuccess )
  {
    rtgCount = *msg++;
    startIndex = *msg++;
    rtgListCount = *msg++;

    // Allocate a buffer big enough to handle the list
    pBuf = osal_mem_alloc( rtgListCount * sizeof( rtgItem_t ) );
    if ( pBuf )
    {
      pList = (rtgItem_t *)pBuf;
      for ( x = 0; x < rtgListCount; x++ )
      {
        pList->dstAddress = BUILD_UINT16( msg[0], msg[1] );
        msg += 2;
        pList->status = *msg++;
        pList->nextHopAddress = BUILD_UINT16( msg[0], msg[1] );
        msg += 2;
        pList++;
      }
    }
  }

  // Call the callback to the application.
  ZDApp_MgmtRtgRspCB( SrcAddr->addr.shortAddr, status, rtgCount,
                                 startIndex, rtgListCount, (rtgItem_t *)pBuf );

  if ( pBuf )
  {
    osal_mem_free( pBuf );
  }
}
#endif // ZDO_MGMT_RTG_REQUEST

#if defined ( ZDO_MGMT_BIND_RESPONSE )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtBindReq
 *
 * @brief       This function finishes the processing of the Management
 *              Bind Request and generates the response.
 *
 *   Note:      This function will limit the number of items returned
 *              to ZDO_MAX_BIND_ITEMS items.
 *
 * @param       ResultCountSrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtBindReq( byte TransSeq, zAddrType_t *SrcAddr, byte StartIndex, byte SecurityUse )
{
#if defined ( REFLECTOR )
  byte x;
  uint16 maxNumItems;
  uint16 numItems;
  byte *pBuf = NULL;
  apsBindingItem_t *pList;

  // Get the number of table items
  APSME_GetRequest( apsNumBindingTableEntries, 0, (byte*)(&maxNumItems) );

  if ( maxNumItems > StartIndex )
    numItems = maxNumItems - StartIndex;    // Start at the passed in index
  else
    numItems = 0;

  // limit the size of the list
  if ( numItems > ZDO_MAX_BIND_ITEMS )
    numItems = ZDO_MAX_BIND_ITEMS;

  // Allocate the memory to build the table
  if ( numItems )
    pBuf = osal_mem_alloc( sizeof( apsBindingItem_t ) * numItems );

  if ( pBuf )
  {
    // Convert buffer to list
    pList = (apsBindingItem_t *)pBuf;

    // Loop through items and build list
    for ( x = 0; x < numItems; x++ )
    {
      APSME_GetRequest( apsBindingTable, (x + StartIndex), (void*)pList );
      pList++;
    }
  }

  // Send response
  ZDP_MgmtBindRsp( TransSeq, SrcAddr, ZSuccess, (byte)maxNumItems, StartIndex, (byte)numItems,
                        (apsBindingItem_t *)pBuf, false );

  if ( pBuf )
  {
    osal_mem_free( pBuf );
  }
#else  // See if app support is needed

  ZDApp_MgmtBindReqCB( TransSeq, SrcAddr, StartIndex, SecurityUse );

#endif
}
#endif // ZDO_MGMT_BIND_RESPONSE

#if defined ( ZDO_MGMT_BIND_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtBindRsp
 *
 * @brief       This function handles parsing the incoming Management
 *              Binding response and then generates a callback
 *              to the ZD application.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - buffer holding incoming message to parse
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtBindRsp( zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  byte x;
  byte status;
  byte bindingCount = 0;
  byte startIndex = 0;
  byte bindingListCount = 0;
  byte *pBuf = NULL;
  apsBindingItem_t *pList = NULL;

  status = *msg++;
  if ( status == ZSuccess )
  {
    bindingCount = *msg++;
    startIndex = *msg++;
    bindingListCount = *msg++;

    // Allocate a buffer big enough to handle the list
    if ( bindingListCount )
      pBuf = osal_mem_alloc( (short)(bindingListCount * sizeof( apsBindingItem_t )) );
    if ( pBuf )
    {
      pList = (apsBindingItem_t *)pBuf;
      for ( x = 0; x < bindingListCount; x++ )
      {
        osal_cpyExtAddr( pList->srcAddr, msg );
        msg += Z_EXTADDR_LEN;
        pList->srcEP = *msg++;

        // Get the Cluster ID
        if ( NLME_GetProtocolVersion() != ZB_PROT_V1_0 )
        {
          pList->clusterID = BUILD_UINT16( msg[0], msg[1] );
          msg += 2;
          pList->dstAddr.addrMode = *msg++;
          if ( pList->dstAddr.addrMode == Addr64Bit )
          {
            osal_cpyExtAddr( pList->dstAddr.addr.extAddr, msg );
            msg += Z_EXTADDR_LEN;
            pList->dstEP = *msg++;
          }
          else
          {
            pList->dstAddr.addr.shortAddr = BUILD_UINT16( msg[0], msg[1] );
            msg += 2;
          }
        }
        else
        {
          pList->clusterID = *msg++;

          osal_cpyExtAddr( pList->dstAddr.addr.extAddr, msg );
          msg += Z_EXTADDR_LEN;
          pList->dstEP = *msg++;
        }

        pList++;
      }
    }
  }

  // Call the callback to the application
  ZDApp_MgmtBindRspCB( SrcAddr->addr.shortAddr, status, bindingCount,
                    startIndex, bindingListCount, (apsBindingItem_t *)pBuf );

  if ( pBuf )
      osal_mem_free( pBuf );
}
#endif // ZDO_MGMT_BIND_REQUEST

#if defined ( ZDO_MGMT_JOINDIRECT_RESPONSE ) && defined ( RTR_NWK )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtDirectJoinReq
 *
 * @brief       This function finishes the processing of the Management
 *              Direct Join Request and generates the response.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtDirectJoinReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  byte *deviceAddr;
  byte capInfo;
  byte stat;

  // Parse the message
  deviceAddr = msg;
  capInfo = msg[Z_EXTADDR_LEN];

  stat = (byte) NLME_DirectJoinRequest( deviceAddr, capInfo );

  ZDP_MgmtDirectJoinRsp( TransSeq, SrcAddr, stat, false );
}
#endif // ZDO_MGMT_JOINDIRECT_RESPONSE && RTR_NWK

#if defined ( ZDO_MGMT_JOINDIRECT_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtDirectJoinRsp
 *
 * @brief       This function handles parsing the incoming Management
 *              Direct Join response and then generates a callback
 *              to the ZD application.
 *
 * @param       SrcAddr - source of the request
 * @param       Status - ZSuccess or other for failure
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtDirectJoinRsp( zAddrType_t *SrcAddr, byte Status, byte SecurityUse )
{
  // Call the callback to the application
  ZDApp_MgmtDirectJoinRspCB( SrcAddr->addr.shortAddr, Status, SecurityUse );
}
#endif // ZDO_MGMT_JOINDIRECT_REQUEST

#if defined ( ZDO_MGMT_LEAVE_RESPONSE )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtLeaveReq
 *
 * @brief       This function processes a Management Leave Request
 *              and generates the response.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtLeaveReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  NLME_LeaveReq_t req;
  ZStatus_t       status;


  if ( ( AddrMgrExtAddrValid( msg ) == FALSE                 ) ||
       ( osal_ExtAddrEqual( msg, NLME_GetExtAddr() ) == TRUE )    )
  {
    // Remove this device
    req.extAddr = NULL;
  }
  else
  {
    // Remove child device
    req.extAddr = msg;
  }

  req.removeChildren = FALSE;
  req.rejoin         = FALSE;
  req.silent         = FALSE;

  status = NLME_LeaveReq( &req );

  ZDP_MgmtLeaveRsp( TransSeq, SrcAddr, status, FALSE );
}
#endif // ZDO_MGMT_LEAVE_RESPONSE

#if defined ( ZDO_MGMT_LEAVE_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtLeaveRsp
 *
 * @brief       This function handles a Management Leave Response
 *              and generates a callback to the ZD application.
 *
 * @param       SrcAddr - source of the request
 * @param       Status - ZSuccess or other for failure
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtLeaveRsp( zAddrType_t *SrcAddr, byte Status, byte SecurityUse )
{
  // Call the callback to the application
  ZDApp_MgmtLeaveRspCB( SrcAddr->addr.shortAddr, Status, SecurityUse );
}
#endif // ZDO_MGMT_LEAVE_REQUEST

#if defined( ZDO_MGMT_PERMIT_JOIN_RESPONSE ) && defined( RTR_NWK )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtPermitJoinReq
 *
 * @brief       This function processes a Management Permit Join Request
 *              and generates the response.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtPermitJoinReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                                   byte SecurityUse )
{
  uint8 stat;
  uint8 duration;
  uint8 tcsig;


  duration = msg[ZDP_MGMT_PERMIT_JOIN_REQ_DURATION];
  tcsig    = msg[ZDP_MGMT_PERMIT_JOIN_REQ_TC_SIG];

  // Set the network layer permit join duration
  stat = (byte) NLME_PermitJoiningRequest( duration );

  // Handle the Trust Center Significance
  if ( tcsig == TRUE )
  {
    ZDSecMgrPermitJoining( duration );
  }

  // Send a response if unicast
  if (SrcAddr->addr.shortAddr != NWK_BROADCAST_SHORTADDR)
  {
    ZDP_MgmtPermitJoinRsp( TransSeq, SrcAddr, stat, false );
  }
}
#endif // ZDO_MGMT_PERMIT_JOIN_RESPONSE && defined( RTR_NWK )

/*
 * This function stub allows the next higher layer to be notified of
 * a permit joining timeout.
 */
#if defined( RTR_NWK )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtPermitJoinTimeout
 *
 * @brief       This function stub allows the next higher layer to be
 *              notified of a permit joining timeout. Currently, this
 *              directly bypasses the APS layer.
 *
 * @param       none
 *
 * @return      none
 */
void ZDO_ProcessMgmtPermitJoinTimeout( void )
{
  #if defined( ZDO_MGMT_PERMIT_JOIN_RESPONSE )
  {
    // Currently, only the ZDSecMgr needs to be notified
    ZDSecMgrPermitJoiningTimeout();
  }
  #endif
}
#endif // defined( RTR_NWK )

#if defined ( ZDO_MGMT_PERMIT_JOIN_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessMgmtPermitJoinRsp
 *
 * @brief       This function handles a Management Permit Join Response
 *              and generates a callback to the ZD application.
 *
 * @param       SrcAddr - source of the request
 * @param       Status - ZSuccess or other for failure
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessMgmtPermitJoinRsp( zAddrType_t *SrcAddr, byte Status, byte SecurityUse )
{
  // Call the callback to the application
  ZDApp_MgmtPermitJoinRspCB( SrcAddr->addr.shortAddr, Status, SecurityUse );
}
#endif // ZDO_MGMT_PERMIT_JOIN_REQUEST

#if defined ( ZDO_USERDESC_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessUserDescRsp
 *
 * @brief       This function handles parsing the incoming User
 *              Descriptor Response and then generates a callback
 *              to the ZD application.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - incoming response message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessUserDescRsp( zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  ZDApp_UserDescRspCB( SrcAddr->addr.shortAddr,
                      msg[0],                           // Status
                      BUILD_UINT16( msg[1], msg[2] ),   // NWKAddrOfInterest
                      msg[3],                           // Length
                      &msg[4],                          // User Descriptor
                      SecurityUse );
}
#endif // ZDO_USERDESC_REQUEST

#if defined ( ZDO_USERDESC_RESPONSE )
/*********************************************************************
 * @fn          ZDO_ProcessUserDescReq
 *
 * @brief       This function finishes the processing of the User
 *              Descriptor Request and generates the response.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessUserDescReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );
  UserDescriptorFormat_t userDesc;

  if ( (aoi == ZDAppNwkAddr.addr.shortAddr) && (ZSUCCESS == osal_nv_read(
             ZCD_NV_USERDESC, 0, sizeof(UserDescriptorFormat_t), &userDesc )) )
  {
    ZDP_UserDescRsp( TransSeq, SrcAddr, aoi, &userDesc, false );
  }
  else
  {
#if defined( ZDO_CACHE )
    (void)aoi;
#else
    ZDP_GenericRsp(
       TransSeq, SrcAddr, ZDP_NOT_SUPPORTED, aoi, User_Desc_rsp, SecurityUse );
#endif
  }
}
#endif // ZDO_USERDESC_RESPONSE

#if defined ( ZDO_USERDESCSET_REQUEST )
/*********************************************************************
 * @fn          ZDO_ProcessUserDescConf
 *
 * @brief       This function handles parsing the incoming User
 *              Descriptor Confirm and then generates a callback
 *              to the ZD application.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - incoming response message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessUserDescConf( zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  ZDApp_UserDescConfCB( SrcAddr->addr.shortAddr,
                        msg[0],                           // Status
                        SecurityUse );
}
#endif // ZDO_USERDESCSET_REQUEST


#if defined ( ZDO_USERDESCSET_RESPONSE )
/*********************************************************************
 * @fn          ZDO_ProcessUserDescSet
 *
 * @brief       This function finishes the processing of the User
 *              Descriptor Set and generates the response.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessUserDescSet( byte TransSeq, zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  uint16 aoi = BUILD_UINT16( msg[0], msg[1] );
  UserDescriptorFormat_t userDesc;
  uint8 outMsg[3];
  uint8 status;
  uint16 nai;

  nai = BUILD_UINT16( msg[0], msg[1] );

  if ( aoi == ZDAppNwkAddr.addr.shortAddr )
  {
    if ( NLME_GetProtocolVersion() == ZB_PROT_V1_0 )
      userDesc.len = AF_MAX_USER_DESCRIPTOR_LEN;
    else
    {
      userDesc.len = (msg[2] < AF_MAX_USER_DESCRIPTOR_LEN) ? msg[2] : AF_MAX_USER_DESCRIPTOR_LEN;
      msg ++;  // increment one for the length field
    }
    osal_memcpy( userDesc.desc, &msg[2], userDesc.len );
    osal_nv_write( ZCD_NV_USERDESC, 0, sizeof(UserDescriptorFormat_t), &userDesc );
    if ( userDesc.len != 0 )
    {
      ZDO_Config_Node_Descriptor.UserDescAvail = TRUE;
    }
    else
    {
      ZDO_Config_Node_Descriptor.UserDescAvail = FALSE;
    }

    status = ZDP_SUCCESS;
  }
  else
  {
    status =  ZDP_NOT_SUPPORTED;
  }

  outMsg[0] = status;
  outMsg[1] = LO_UINT16( nai );
  outMsg[2] = LO_UINT16( nai );

  ZDP_SendData( &TransSeq, SrcAddr, User_Desc_conf, 3, outMsg, SecurityUse );
}
#endif // ZDO_USERDESCSET_RESPONSE

#if defined ( ZDO_ENDDEVICE_ANNCE ) && defined(RTR_NWK)
/*********************************************************************
 * @fn          ZDO_ProcessEndDeviceAnnce
 *
 * @brief       This function processes an end device annouce message.
 *
 * @param       SrcAddr - source of the request
 * @param       msg - pointer to incoming message
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDO_ProcessEndDeviceAnnce( byte TransSeq, zAddrType_t *SrcAddr, byte *msg, byte SecurityUse )
{
  uint16 nwkAddr;
  associated_devices_t *dev;
  AddrMgrEntry_t addrEntry;
  uint8 *ieeeAddr;
  uint8 capabilities;

  // Parse incoming message
  nwkAddr = BUILD_UINT16( msg[0], msg[1] );
  msg += 2;
  ieeeAddr = msg;
  msg += Z_EXTADDR_LEN;
  if ( NLME_GetProtocolVersion() != ZB_PROT_V1_0 )
  {
    capabilities = *msg;
  }
  else
  {
    capabilities = 0;
  }

  addrEntry.user = ADDRMGR_USER_DEFAULT;
  addrEntry.nwkAddr = nwkAddr;
  AddrMgrExtAddrSet( addrEntry.extAddr, ieeeAddr );
  AddrMgrEntryUpdate( &addrEntry );

  // find device in device list
  dev = AssocGetWithExt( ieeeAddr );
  if ( dev != NULL )
  {
    // if found and address is different
    if ( dev->shortAddr != nwkAddr )
    {
      // update device list if device is (was) not our child
      if ( dev->nodeRelation == NEIGHBOR || dev->nodeRelation == OTHER )
      {
        dev->shortAddr = nwkAddr;
      }
    }
  }

  // Application notification
  ZDApp_EndDeviceAnnounceCB( SrcAddr->addr.shortAddr, nwkAddr, ieeeAddr, capabilities );
}
#endif // ZDO_ENDDEVICE_ANNCE

#if defined( ZDO_SIMPLEDESC_REQUEST ) || ( defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 ) )
/*********************************************************************
 * @fn          ZDO_BuildSimpleDescBuf
 *
 * @brief       Build a byte sequence representation of a Simple Descriptor.
 *
 * @param       buf  - pointer to a byte array big enough for data.
 * @param       desc - SimpleDescriptionFormat_t *
 *
 * @return      none
 */
void ZDO_BuildSimpleDescBuf( byte *buf, SimpleDescriptionFormat_t *desc )
{
  byte proVer = NLME_GetProtocolVersion();
  byte cnt;
  uint16 *ptr;

  *buf++ = desc->EndPoint;
  *buf++ = HI_UINT16( desc->AppProfId );
  *buf++ = LO_UINT16( desc->AppProfId );
  *buf++ = HI_UINT16( desc->AppDeviceId );
  *buf++ = LO_UINT16( desc->AppDeviceId );

  if ( proVer == ZB_PROT_V1_0 )
  {
    *buf++ = (byte)((desc->AppDevVer << 4) | (desc->Reserved));
  }
  else
  {
    *buf++ = (byte)(desc->AppDevVer << 4);
  }

  *buf++ = desc->AppNumInClusters;
  ptr = desc->pAppInClusterList;
  for ( cnt = 0; cnt < desc->AppNumInClusters; ptr++, cnt++ )
  {
    *buf++ = HI_UINT16( *ptr );
    *buf++ = LO_UINT16( *ptr );
  }

  *buf++ = desc->AppNumOutClusters;
  ptr = desc->pAppOutClusterList;
  for ( cnt = 0; cnt < desc->AppNumOutClusters; ptr++, cnt++ )
  {
    *buf++ = HI_UINT16( *ptr );
    *buf++ = LO_UINT16( *ptr );
  }
}

/*********************************************************************
 * @fn          ZDO_ParseSimpleDescBuf
 *
 * @brief       Parse a byte sequence representation of a Simple Descriptor.
 *
 * @param       buf  - pointer to a byte array representing a Simple Desc.
 * @param       desc - SimpleDescriptionFormat_t *
 *
 *              This routine allocates storage for the cluster IDs because
 *              they are 16-bit and need to be aligned to be properly processed.
 *              This routine returns non-zero if an allocation fails.
 *
 *              NOTE: This means that the caller or user of the input structure
 *                    is responsible for freeing the memory
 *
 * @return      0: success
 *              1: failure due to malloc failure.
 */
uint8 ZDO_ParseSimpleDescBuf( byte *buf, SimpleDescriptionFormat_t *desc )
{
  byte proVer = NLME_GetProtocolVersion();
  uint8 num, i;

  desc->EndPoint = *buf++;
  desc->AppProfId = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;
  desc->AppDeviceId = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;
  desc->AppDevVer = *buf >> 4;

  if ( proVer == ZB_PROT_V1_0 )
  {
    desc->Reserved = *buf++ &0x0F;
  }
  else
  {
    desc->Reserved = 0;
    buf++;
  }

  // move in input cluster list (if any). allocate aligned memory.
  num = desc->AppNumInClusters = *buf++;
  if (num)  {
    if (!(desc->pAppInClusterList = (uint16 *)osal_mem_alloc(num*sizeof(uint16))))  {
      // malloc failed. we're done.
      return 1;
    }
    for (i=0; i<num; ++i)  {
      desc->pAppInClusterList[i] = BUILD_UINT16( buf[0], buf[1] );
      buf += 2;
    }
  }

  // move in output cluster list (if any). allocate aligned memory.
  num = desc->AppNumOutClusters = *buf++;
  if (num)  {
    if (!(desc->pAppOutClusterList = (uint16 *)osal_mem_alloc(num*sizeof(uint16))))  {
      // malloc failed. free input cluster list memory if there is any
      if (desc->pAppInClusterList)  {
        osal_mem_free(desc->pAppInClusterList);
      }
      return 1;
    }
    for (i=0; i<num; ++i)  {
      desc->pAppOutClusterList[i] = BUILD_UINT16( buf[0], buf[1] );
      buf += 2;
    }
  }
  return 0;
}
#endif

#if defined ( ZDO_COORDINATOR )
/*********************************************************************
 * @fn      ZDO_MatchEndDeviceBind()
 *
 * @brief
 *
 *   Called to match end device binding requests
 *
 * @param  bindReq  - binding request information
 * @param  SecurityUse - Security enable/disable
 *
 * @return  none
 */
void ZDO_MatchEndDeviceBind( ZDEndDeviceBind_t *bindReq )
{
  zAddrType_t dstAddr;
  uint8 sendRsp = FALSE;
  uint8 status;

  // Is this the first request?
  if ( matchED == NULL )
  {
    // Create match info structure
    matchED = (ZDMatchEndDeviceBind_t *)osal_mem_alloc( sizeof ( ZDMatchEndDeviceBind_t ) );
    if ( matchED )
    {
      // Clear the structure
      osal_memset( (uint8 *)matchED, 0, sizeof ( ZDMatchEndDeviceBind_t ) );

      // Copy the first request's information
      if ( !ZDO_CopyMatchInfo( &(matchED->ed1), bindReq ) )
      {

        status = ZDP_NO_ENTRY;
        sendRsp = TRUE;
      }
    }
    else
    {
      status = ZDP_NO_ENTRY;
      sendRsp = TRUE;
    }

    if ( !sendRsp )
    {
      // Set into the correct state
      matchED->state = ZDMATCH_WAIT_REQ;

      // Setup the timeout
      APS_SetEndDeviceBindTimeout( AIB_MaxBindingTime, ZDO_EndDeviceBindMatchTimeoutCB );
    }
  }
  else
  {
      matchED->state = ZDMATCH_SENDING_BINDS;

      // Copy the 2nd request's information
      if ( !ZDO_CopyMatchInfo( &(matchED->ed2), bindReq ) )
      {
        status = ZDP_NO_ENTRY;
        sendRsp = TRUE;
      }

      // Make a source match for ed1
      matchED->ed1numMatched = ZDO_CompareClusterLists(
                  matchED->ed1.numOutClusters, matchED->ed1.outClusters,
                  matchED->ed2.numInClusters, matchED->ed2.inClusters, ZDOBuildBuf );
      if ( matchED->ed1numMatched )
      {
        // Save the match list
        matchED->ed1Matched = osal_mem_alloc( (short)(matchED->ed1numMatched * sizeof ( uint16 )) );
        if ( matchED->ed1Matched )
        {
          osal_memcpy( matchED->ed1Matched, ZDOBuildBuf, (matchED->ed1numMatched * sizeof ( uint16 )) );
        }
        else
        {
          // Allocation error, stop
          status = ZDP_NO_ENTRY;
          sendRsp = TRUE;
        }
      }

      // Make a source match for ed2
      matchED->ed2numMatched = ZDO_CompareClusterLists(
                  matchED->ed2.numOutClusters, matchED->ed2.outClusters,
                  matchED->ed1.numInClusters, matchED->ed1.inClusters, ZDOBuildBuf );
      if ( matchED->ed2numMatched )
      {
        // Save the match list
        matchED->ed2Matched = osal_mem_alloc( (short)(matchED->ed2numMatched * sizeof ( uint16 )) );
        if ( matchED->ed2Matched )
        {
          osal_memcpy( matchED->ed2Matched, ZDOBuildBuf, (matchED->ed2numMatched * sizeof ( uint16 )) );
        }
        else
        {
          // Allocation error, stop
          status = ZDP_NO_ENTRY;
          sendRsp = TRUE;
        }
      }

      if ( (sendRsp == FALSE) && (matchED->ed1numMatched || matchED->ed2numMatched) )
      {
        // Do the first unbind/bind state
        ZDMatchSendState( ZDMATCH_REASON_START, ZDP_SUCCESS, 0 );
      }
      else
      {
        status = ZDP_NO_MATCH;
        sendRsp = TRUE;
      }
  }

  if ( sendRsp )
  {
    // send response to this requester
    dstAddr.addrMode = Addr16Bit;
    dstAddr.addr.shortAddr = bindReq->srcAddr;
    ZDP_EndDeviceBindRsp( bindReq->TransSeq, &dstAddr, status, bindReq->SecurityUse );

    if ( matchED->state == ZDMATCH_SENDING_BINDS )
    {
      // send response to first requester
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = matchED->ed1.srcAddr;
      ZDP_EndDeviceBindRsp( matchED->ed1.TransSeq, &dstAddr, status, matchED->ed1.SecurityUse );
    }

    // Process ended - release memory used
    ZDO_RemoveMatchMemory();
  }
}

static void ZDO_RemoveMatchMemory( void )
{
  if ( matchED )
  {
    if ( matchED->ed2Matched )
      osal_mem_free( matchED->ed2Matched );
    if ( matchED->ed1Matched )
      osal_mem_free( matchED->ed1Matched );

    if ( matchED->ed1.inClusters )
      osal_mem_free( matchED->ed1.inClusters );

    if ( matchED->ed1.outClusters )
      osal_mem_free( matchED->ed1.outClusters );

    if ( matchED->ed2.inClusters )
      osal_mem_free( matchED->ed2.inClusters );

    if ( matchED->ed2.outClusters )
      osal_mem_free( matchED->ed2.outClusters );

    osal_mem_free( matchED );

    matchED = (ZDMatchEndDeviceBind_t *)NULL;
  }
}

static uint8 ZDO_CopyMatchInfo( ZDEndDeviceBind_t *destReq, ZDEndDeviceBind_t *srcReq )
{
  uint8 allOK = TRUE;

  // Copy bind information into the match info structure
  osal_memcpy( (uint8 *)destReq, srcReq, sizeof ( ZDEndDeviceBind_t ) );

  // Copy input cluster IDs
  if ( srcReq->numInClusters )
  {
    destReq->inClusters = osal_mem_alloc( (short)(srcReq->numInClusters * sizeof ( uint16 )) );
    if ( destReq->inClusters )
    {
      // Copy the clusters
      osal_memcpy( (uint8*)(destReq->inClusters), (uint8 *)(srcReq->inClusters),
                      (srcReq->numInClusters * sizeof ( uint16 )) );
    }
    else
      allOK = FALSE;
  }

  // Copy output cluster IDs
  if ( srcReq->numOutClusters )
  {
    destReq->outClusters = osal_mem_alloc( (short)(srcReq->numOutClusters * sizeof ( uint16 )) );
    if ( destReq->outClusters )
    {
      // Copy the clusters
      osal_memcpy( (uint8 *)(destReq->outClusters), (uint8 *)(srcReq->outClusters),
                      (srcReq->numOutClusters * sizeof ( uint16 )) );
    }
    else
      allOK = FALSE;
  }

  if ( !allOK )
  {
    if ( destReq->inClusters )
      osal_mem_free( destReq->inClusters );
    if ( destReq->outClusters )
      osal_mem_free( destReq->outClusters );
  }

  return ( allOK );
}

static uint8 ZDMatchSendState( uint8 reason, uint8 status, uint8 TransSeq )
{
  uint8 *dstIEEEAddr;
  uint8 dstEP;
  zAddrType_t dstAddr;
  zAddrType_t destinationAddr;
  uint16 msgType;
  uint16 clusterID;
  ZDEndDeviceBind_t *ed = NULL;
  uint8 rspStatus = ZDP_SUCCESS;

  if ( matchED == NULL )
    return ( FALSE );

  // Check sequence number
  if ( reason == ZDMATCH_REASON_BIND_RSP || reason == ZDMATCH_REASON_UNBIND_RSP )
  {
    if ( TransSeq != matchED->transSeq )
      return( FALSE ); // ignore the message
  }

  // turn off timer
  APS_SetEndDeviceBindTimeout( 0, ZDO_EndDeviceBindMatchTimeoutCB );

  if ( reason == ZDMATCH_REASON_TIMEOUT )
  {
    rspStatus = ZDP_TIMEOUT;    // The process will stop
  }

  if ( reason == ZDMATCH_REASON_START || reason == ZDMATCH_REASON_BIND_RSP )
  {
    matchED->sending = ZDMATCH_SENDING_UNBIND;

    if ( reason == ZDMATCH_REASON_BIND_RSP && status != ZDP_SUCCESS )
    {
      rspStatus = status;
    }
  }
  else if ( reason == ZDMATCH_REASON_UNBIND_RSP )
  {
    if ( status == ZDP_SUCCESS )
    {
      matchED->sending = ZDMATCH_SENDING_UNBIND;
    }
    else
    {
      matchED->sending = ZDMATCH_SENDING_BIND;
    }
  }

  if ( reason != ZDMATCH_REASON_START && matchED->sending == ZDMATCH_SENDING_UNBIND )
  {
    // Move to the next cluster ID
    if ( matchED->ed1numMatched )
      matchED->ed1numMatched--;
    else if ( matchED->ed2numMatched )
      matchED->ed2numMatched--;
  }

  // What message do we send now
  if ( matchED->ed1numMatched )
  {
    ed = &(matchED->ed1);
    clusterID = matchED->ed1Matched[matchED->ed1numMatched-1];
    dstIEEEAddr = matchED->ed2.ieeeAddr;
    dstEP = matchED->ed2.endpoint;
  }
  else if ( matchED->ed2numMatched )
  {
    ed = &(matchED->ed2);
    clusterID = matchED->ed2Matched[matchED->ed2numMatched-1];
    dstIEEEAddr = matchED->ed1.ieeeAddr;
    dstEP = matchED->ed1.endpoint;
  }

  dstAddr.addrMode = Addr16Bit;

  // Send the next message
  if ( rspStatus == ZDP_SUCCESS && ed )
  {
    // Send unbind/bind message to source
    if ( matchED->sending == ZDMATCH_SENDING_UNBIND )
      msgType = Unbind_req;
    else
      msgType = Bind_req;

    dstAddr.addr.shortAddr = ed->srcAddr;

    // Save off the transaction sequence number
    matchED->transSeq = ZDP_TransID;

    destinationAddr.addrMode = Addr64Bit;
    osal_cpyExtAddr( destinationAddr.addr.extAddr, dstIEEEAddr );

    ZDP_BindUnbindReq( msgType, &dstAddr, ed->ieeeAddr, ed->endpoint, clusterID,
        &destinationAddr, dstEP, ed->SecurityUse );

    // Set timeout for response
    APS_SetEndDeviceBindTimeout( AIB_MaxBindingTime, ZDO_EndDeviceBindMatchTimeoutCB );
  }
  else
  {
    // Send the response messages to requesting devices
    // send response to first requester
    dstAddr.addr.shortAddr = matchED->ed1.srcAddr;
    ZDP_EndDeviceBindRsp( matchED->ed1.TransSeq, &dstAddr, rspStatus, matchED->ed1.SecurityUse );

    // send response to second requester
    if ( matchED->state == ZDMATCH_SENDING_BINDS )
    {
      dstAddr.addr.shortAddr = matchED->ed2.srcAddr;
      ZDP_EndDeviceBindRsp( matchED->ed2.TransSeq, &dstAddr, rspStatus, matchED->ed2.SecurityUse );
    }

    // Process ended - release memory used
    ZDO_RemoveMatchMemory();
  }

  return ( TRUE );
}

static void ZDO_EndDeviceBindMatchTimeoutCB( void )
{
  ZDMatchSendState( ZDMATCH_REASON_TIMEOUT, ZDP_TIMEOUT, 0 );
}

#endif // ZDO_COORDINATOR

/*********************************************************************
*********************************************************************/


