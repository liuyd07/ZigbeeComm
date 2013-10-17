/*********************************************************************
    Filename:       ZDCache.c
    Revised:        $Date: 2007-02-15 15:11:28 -0800 (Thu, 15 Feb 2007) $
    Revision:       $Revision: 13481 $

    Description: Implementation of the ZDO Discovery Cache functionality.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/
#if defined( ZDO_CACHE )

/*********************************************************************
 * INCLUDES
 */
#include "AF.h"
#include "nwk_globals.h"
#include "nwk_util.h"
#include "OSAL.h"
#include "ZDCache.h"
#include "ZDConfig.h"
#include "ZDObject.h"


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define MAX_PKT_LEN  (MAX_DATA_PACKET_LEN - NWK_HDR_LEN)

#if ( CACHE_DEV_MAX == 0 )
  #define WAIT_ON_RESP_CACHE  5

  #define FIND_RSP_MAX  16
#endif

#define SendMsg( CMD, LEN, DATA ) \
  afAddOrSendMessage(   &msgAddr,             /* *DestAddr         */\
                        ZDO_EP,               /*  Endpoint         */\
                        (CMD),                /*  ClusterId        */\
                        SEND_MESSAGE,         /*  TransCount       */\
                        FRAMETYPE_MSG,        /*  FrameType        */\
                        &tranSeq,             /* *TransSeqNumber   */\
                        NULL,                 /*  CommandType      */\
                        NULL,                 /*  AttribDataType   */\
                        NULL,                 /*  AttribId         */\
                        NULL,                 /*  ErrorCode        */\
                        (LEN),                /*  DataLength       */\
                        (DATA),               /* *Data             */\
                        AF_TX_OPTIONS_NONE,   \
                        FALSE,                /*  Discover route   */\
                        radius )

/*********************************************************************
 * TYPEDEFS
 */

#if ( CACHE_DEV_MAX == 0 )
typedef enum {
  eCacheWait,
  eCacheClean,
  eCacheFind,
  eCacheRequest,
  eNodeDescStore,
  ePwrDescStore,
  eActiveEPStore,
  eSimpDescStore,
  eCacheDone
} eCacheState;
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
 * LOCAL FUNCTIONS
 */

#if ( CACHE_DEV_MAX > 0 )
static byte processDiscoveryStoreReq  ( byte *data );
static byte processSimpleDescStoreReq ( byte *data, byte idx );
static void processFindNodeCacheReq   ( byte *data );

static byte getCnt    ( void );
static byte getIdx    ( uint16 addr );
static byte getNth    ( byte Nth );
static byte getIdxExt ( byte *ieee );
static byte getIdxEP  ( byte idx, byte ep );
static byte purgeAddr ( uint16 addr );
static byte purgeIEEE ( byte *ieee );
#endif

/*********************************************************************
 * LOCAL VARIABLES
 */

static afAddrType_t msgAddr;
static byte radius;
static byte tranSeq;
static byte secUse;

#if ( CACHE_DEV_MAX > 0 )
static uint16                      NwkAddr[CACHE_DEV_MAX];
static byte                        ExtAddr[CACHE_DEV_MAX][Z_EXTADDR_LEN];
static uint32                       Expiry[CACHE_DEV_MAX];
static NodeDescriptorFormat_t     NodeDesc[CACHE_DEV_MAX];
static NodePowerDescriptorFormat_t NodePwr[CACHE_DEV_MAX];
static byte                          EPCnt[CACHE_DEV_MAX];
static byte                          EPArr[CACHE_DEV_MAX][CACHE_EP_MAX];
static SimpleDescriptionFormat_t  SimpDesc[CACHE_DEV_MAX][CACHE_EP_MAX];
static uint16               InClusters[CACHE_DEV_MAX][CACHE_EP_MAX][CACHE_CR_MAX];
static uint16              OutClusters[CACHE_DEV_MAX][CACHE_EP_MAX][CACHE_CR_MAX];
#elif ( CACHE_DEV_MAX == 0 )
static uint16 cacheFindAddr[FIND_RSP_MAX];
static byte EPArr[CACHE_EP_MAX];
static byte EPCnt;
static byte cacheCnt;  // Count of responding cache devices.
static byte cacheRsp;  // Results of last discovery cache request.
#endif



#if ( CACHE_DEV_MAX > 0 )
/*********************************************************************
 * @fn          processDiscoveryStoreReq
 *
 * @brief       Process a Discovery_store_req
 *
 * @return      none
 */
static byte processDiscoveryStoreReq( byte *data )
{
  uint16 aoi = BUILD_UINT16( data[0], data[1] );
  byte rtrn = ZDP_INSUFFICIENT_SPACE;
  byte idx;

  data += 2;
  // First purge any outdated cache with this network address or IEEE.
  purgeAddr( aoi );
  purgeIEEE( data );

  // If an invalid addr cannot be found, the cache arrays are full.
  if ( (idx = getIdx( INVALID_NODE_ADDR )) != CACHE_DEV_MAX )
  {
    osal_cpyExtAddr( ExtAddr[idx], data );
    data += Z_EXTADDR_LEN;

    if ( (*data++ == sizeof( NodeDescriptorFormat_t )) &&
         (*data++ == sizeof( NodePowerDescriptorFormat_t )) &&
         (*data++ <= CACHE_EP_MAX) &&
         (*data   < CACHE_EP_MAX) )
    {
      rtrn = ZDP_SUCCESS;
      NwkAddr[idx] = aoi;
      Expiry[idx] = 0;  //TBD - what value & how to keep alive?
      EPCnt[idx] = 0;
    }
  }

  return rtrn;
}

/*********************************************************************
 * @fn          processSimpleDescStoreReq
 *
 * @brief       Process a Simple_Desc_store_req
 *
 * @return      none
 */
static byte processSimpleDescStoreReq( byte *data, byte idx )
{
  byte rtrn = ZDP_INSUFFICIENT_SPACE;
  SimpleDescriptionFormat_t desc;

  // Skip first byte == total length of descriptor.
  if (ZDO_ParseSimpleDescBuf( data+1, &desc ))  {
    // malloc failed -- we can't do this...
    return ZDP_NOT_PERMITTED;
  }

  if ( (desc.AppNumInClusters <= CACHE_CR_MAX) &&
       (desc.AppNumOutClusters <= CACHE_CR_MAX) )
  {
    byte epIdx = getIdxEP( idx, desc.EndPoint );

    if ( epIdx == CACHE_EP_MAX )
    {
      rtrn = ZDP_NOT_PERMITTED;
    }
    else
    {
      SimpleDescriptionFormat_t *pDesc = &(SimpDesc[idx][epIdx]);

      osal_memcpy( pDesc->pAppInClusterList, desc.pAppInClusterList,
                                                       desc.AppNumInClusters*sizeof(uint16) );
      osal_memcpy( pDesc->pAppOutClusterList, desc.pAppOutClusterList,
                                                      desc.AppNumOutClusters*sizeof(uint16) );

      pDesc->EndPoint = desc.EndPoint;
      pDesc->AppProfId = desc.AppProfId;
      pDesc->AppDeviceId = desc.AppDeviceId;
      pDesc->AppDevVer = desc.AppDevVer;
      pDesc->Reserved = desc.Reserved;
      pDesc->AppNumInClusters = desc.AppNumInClusters;
      pDesc->AppNumOutClusters = desc.AppNumOutClusters;

      rtrn = ZDP_SUCCESS;
    }
  }

  // free up the malloc'ed cluster lists
  if (desc.AppNumInClusters)  {
    osal_mem_free(desc.pAppInClusterList);
  }
  if (desc.AppNumOutClusters)  {
    osal_mem_free(desc.pAppOutClusterList);
  }

  return rtrn;
}

/*********************************************************************
 * @fn          processFindNodeCacheReq
 *
 * @brief       Process a Find_node_cache_req
 *
 * @return      none
 */
static void processFindNodeCacheReq( byte *data )
{
  uint16 aoi = BUILD_UINT16( data[0], data[1] );
  byte idx = getIdx( aoi );

  if ( idx == CACHE_DEV_MAX )
  {
    idx = getIdxExt ( data+2 );
  }

  if ( idx != CACHE_DEV_MAX )
  {
    byte buf[2+2+Z_EXTADDR_LEN];

    buf[0] = LO_UINT16( ZDAppNwkAddr.addr.shortAddr );
    buf[1] = HI_UINT16( ZDAppNwkAddr.addr.shortAddr );
    buf[2] = LO_UINT16( NwkAddr[idx] );
    buf[3] = HI_UINT16( NwkAddr[idx] );
    osal_cpyExtAddr( buf+4, ExtAddr[idx] );

    SendMsg( Find_node_cache_rsp, 2+2+Z_EXTADDR_LEN, buf );
  }
}

/*********************************************************************
 * @fn          processMgmtCacheReq
 *
 * @brief       Process a Mgmt_cache_req
 *
 * @return      none
 */
static void processMgmtCacheReq( byte idx )
{
  const byte max =
              (((CACHE_DEV_MAX-idx) * (2 + Z_EXTADDR_LEN) + 1) < MAX_PKT_LEN) ?
               ((CACHE_DEV_MAX-idx) * (2 + Z_EXTADDR_LEN) + 1) : MAX_PKT_LEN;
  byte *buf = osal_mem_alloc( max );
  byte status = ZDP_INSUFFICIENT_SPACE;
  byte cnt = 1;

  if ( buf == NULL )
  {
    buf = &status;
  }
  else
  {
    byte *ptr = buf;
    *ptr++ = ZDP_SUCCESS;
    *ptr++ = getCnt();
    *ptr++ = idx;
    *ptr++ = 0;

    for ( idx = getNth( idx ); idx < CACHE_DEV_MAX; idx++ )
    {
      if ( cnt >= (max - (2 + Z_EXTADDR_LEN)) )
      {
        break;
      }

      if ( NwkAddr[idx] != INVALID_NODE_ADDR )
      {
        ptr = osal_cpyExtAddr( ptr, ExtAddr[idx] );
        *ptr++ = LO_UINT16( NwkAddr[idx] );
        *ptr++ = HI_UINT16( NwkAddr[idx] );
        cnt += (2 + Z_EXTADDR_LEN);
        buf[3]++;
      }
    }
  }

  SendMsg( Mgmt_Cache_rsp, cnt, buf );

  if ( buf != NULL )
  {
    osal_mem_free( buf );
  }
}

/*********************************************************************
 * @fn      getCnt
 *
 * @return  The count of valid cache entries.
 *
 */
static byte getCnt( void )
{
  byte cnt = 0;
  byte idx;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( NwkAddr[idx] != INVALID_NODE_ADDR )
    {
      cnt++;
    }
  }

  return cnt;
}

/*********************************************************************
 * @fn      getIdx
 *
 * @brief   Find the idx into the Discovery Cache Arrays corresponding to
 *          the given short address.
 *
 * @param   uint16 - a valid 16 bit short address.
 *
 * @return  If address found, return the valid index, else CACHE_DEV_MAX.
 *
 */
static byte getIdx( uint16 addr )
{
  byte idx;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( addr == NwkAddr[idx] )
    {
      break;
    }
  }

  return idx;
}

/*********************************************************************
 * @fn      getNth
 *
 * @param   byte - the Nth idx sought.
 *
 * @return  If N or more entries exist, return the index of the Nth entry,
 *          else CACHE_DEV_MAX.
 *
 */
static byte getNth( byte Nth )
{
  byte cnt = 0;
  byte idx;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( NwkAddr[idx] != INVALID_NODE_ADDR )
    {
      if ( cnt++ >= Nth )
      {
        break;
      }
    }
  }

  return idx;
}

/*********************************************************************
 * @fn      getIdxExt
 *
 * @brief   Find the idx into the Discovery Cache Arrays corresponding to
 *          the given IEEE address.
 *
 * @param   byte * - a valid buffer containing an extended IEEE address.
 *
 * @return  If address found, return the valid index, else CACHE_DEV_MAX.
 *
 */
static byte getIdxExt( byte *ieee )
{
  byte idx;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( osal_ExtAddrEqual( ieee, ExtAddr[idx] ) &&
                                          (NwkAddr[idx] != INVALID_NODE_ADDR) )
    {
      break;
    }
  }

  return idx;
}

/*********************************************************************
 * @fn      getIdxEP
 *
 * @brief   Find the idx into the EndPoint Array corresponding to the idx of a
 *          cached short address.
 *
 * @param   byte - a valid index of a cached 16 bit short address.
 * @param   byte - the EndPoint of interest.
 *
 * @return  If EndPoint found, return the valid index, else CACHE_EP_MAX.
 *
 */
static byte getIdxEP( byte idx, byte ep )
{
  byte epIdx;

  for ( epIdx = 0; epIdx < EPCnt[idx]; epIdx++ )
  {
    if ( ep == EPArr[idx][epIdx] )
    {
      break;
    }
  }

  if ( epIdx == EPCnt[idx] )
  {
    epIdx = CACHE_EP_MAX;
  }

  return epIdx;
}

/*********************************************************************
 * @fn      purgeAddr
 *
 * @brief   Purge every instance of given network address from Discovery Cache.
 *
 * @param   uint16 - a 16-bit network address.
 *
 */
static byte purgeAddr( uint16 addr )
{
  byte idx, cnt = 0;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( NwkAddr[idx] == addr )
    {
      NwkAddr[idx] = INVALID_NODE_ADDR;
      cnt++;
    }
  }

  return cnt;
}

/*********************************************************************
 * @fn      purgeIEEE
 *
 * @brief   Purge every instance of given IEEE from Discovery Cache.
 *
 * @param   ZLongAddr_t - a valid IEEE address.
 *
 */
static byte purgeIEEE( byte *ieee )
{
  byte idx, cnt = 0;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( osal_ExtAddrEqual( ieee, ExtAddr[idx] ) &&
                                          (NwkAddr[idx] != INVALID_NODE_ADDR) )
    {
      NwkAddr[idx] = INVALID_NODE_ADDR;
      cnt++;
    }
  }

  return cnt;
}
#endif

/*********************************************************************
 * @fn          ZDCacheInit
 *
 * @brief       Initialize ZDO Cache environment.
 *
 */
void ZDCacheInit( void )
{

  msgAddr.endPoint = ZDO_EP;
  msgAddr.addrMode = afAddr16Bit;
  msgAddr.addr.shortAddr = INVALID_NODE_ADDR;
  radius = AF_DEFAULT_RADIUS;
  (void)secUse;

#if ( CACHE_DEV_MAX > 0 )
  byte idx;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    byte epIdx;
    NwkAddr[idx] = INVALID_NODE_ADDR;

    for ( epIdx = 0; epIdx < CACHE_EP_MAX; epIdx++ )
    {
      SimpDesc[idx][epIdx].pAppInClusterList = InClusters[idx][epIdx];
      SimpDesc[idx][epIdx].pAppOutClusterList= OutClusters[idx][epIdx];
    }
  }
#elif ( CACHE_DEV_MAX == 0 )
  // Client cache work done by ZDCacheTimerEvent, driven by NWK_AUTO_POLL_EVT.
#endif
}

/*********************************************************************
 * @fn          ZDCacheTimerEvent
 *
 * @brief       ZDP maintenance, aging discovery cache.
 *              Invoked at RTG_TIMER_INTERVAL.
 */
void ZDCacheTimerEvent( void )
{
#if ( CACHE_DEV_MAX > 0 )
  byte idx;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( NwkAddr[idx] != INVALID_NODE_ADDR )
    {
      if ( --Expiry[idx] == 0 )
      {
        NwkAddr[idx] = INVALID_NODE_ADDR;
      }
    }
  }
#elif ( CACHE_DEV_MAX == 0 )
  static eCacheState state = eCacheWait;
  static byte reqIdx = 0;
  static byte wCnt = 0;

  byte strtFind = FALSE;
  byte cmd = 0;
  byte len = 2 + Z_EXTADDR_LEN;
  byte *msg, *ptr;

  wCnt++;
  if ( state == eCacheWait )
  {
    if ( wCnt < WAIT_TO_STORE_CACHE )
    {
      return;
    }
  }
  else if ( wCnt < WAIT_ON_RESP_CACHE )
  {
    return;
  }

  msg = osal_mem_alloc( MAX_PKT_LEN );
  if ( msg == NULL )
  {
    return;
  }

  msg[0] = LO_UINT16( ZDAppNwkAddr.addr.shortAddr );
  msg[1] = HI_UINT16( ZDAppNwkAddr.addr.shortAddr );
  osal_cpyExtAddr( msg+2, saveExtAddr );
  ptr = msg+2+Z_EXTADDR_LEN;

  switch ( state )
  {
  case eCacheWait:
    state = eCacheClean;
    msgAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
    cmd = Find_node_cache_req;
    cacheCnt = 0;
    reqIdx = 0;
    break;

  case eCacheClean:
    if ( reqIdx < cacheCnt )
    {
      msgAddr.addr.shortAddr = cacheFindAddr[reqIdx++];
      cmd = Remove_node_cache_req;
    }
    else
    {
      strtFind = TRUE;
      radius = 0;
    }
    break;

  case eCacheFind:
    if ( cacheCnt != 0 )
    {
      state = eCacheRequest;
      reqIdx = 0;
    }
    else
    {
      strtFind = TRUE;
    }
    break;

  case eCacheRequest:
    if ( (cacheRsp == ZDP_SUCCESS) && (reqIdx != 0) )
    {
      cmd = Node_Desc_store_req;
      len += sizeof( NodeDescriptorFormat_t );
      ptr = osal_memcpy( ptr, &ZDO_Config_Node_Descriptor,
                                            sizeof( NodeDescriptorFormat_t ) );
      state = eNodeDescStore;
    }
    else if ( reqIdx < cacheCnt )
    {
      EPCnt = afNumEndPoints() - 1;  // -1 for ZDO endpoint descriptor.

      if ( EPCnt < CACHE_EP_MAX )
      {
        byte idx;
        afEndPoints( EPArr, true );

        msgAddr.addr.shortAddr = cacheFindAddr[reqIdx++];

        cmd = Discovery_store_req;
        len += (4 + EPCnt);
        *ptr++ = sizeof( NodeDescriptorFormat_t );
        *ptr++ = sizeof( NodePowerDescriptorFormat_t );
        *ptr++ = EPCnt + 1;  // NOT -1 for size in bytes of EP list.
        *ptr++ = EPCnt;

        for ( idx = 0; idx < EPCnt; idx++ )
        {
          SimpleDescriptionFormat_t *sDesc;
          byte free = afFindSimpleDesc( &sDesc, EPArr[idx] );

          if ( sDesc != NULL )
          {
            *ptr++ = 8 + sDesc->AppNumInClusters + sDesc->AppNumOutClusters;

            if ( free )
            {
              osal_mem_free( sDesc );
            }
          }
          else
          {
            *ptr++ = 8;
          }
        }
      }
    }
    else
    {
      strtFind = TRUE;
    }
    break;

  case eNodeDescStore:
    if ( cacheRsp == ZDP_SUCCESS )
    {
      cmd = Power_Desc_store_req;
      len += sizeof( NodePowerDescriptorFormat_t );
      ptr = osal_memcpy( ptr, &ZDO_Config_Power_Descriptor,
                                       sizeof( NodePowerDescriptorFormat_t ) );
      state = ePwrDescStore;
    }
    else
    {
      strtFind = TRUE;
    }
    break;

  case ePwrDescStore:
    if ( cacheRsp == ZDP_SUCCESS )
    {
      cmd = Active_EP_store_req;
      len += EPCnt + 1;
      *ptr++ = EPCnt;
      afEndPoints( ptr, true );
      state = eActiveEPStore;
    }
    else
    {
      strtFind = TRUE;
    }
    break;

  case eActiveEPStore:
    if ( cacheRsp == ZDP_SUCCESS )
    {
      state = eSimpDescStore;
      reqIdx = 0;
    }
    else
    {
      strtFind = TRUE;
    }
    break;

  case eSimpDescStore:
    if ( (cacheRsp == ZDP_SUCCESS) || (reqIdx == 0) )
    {
      if ( reqIdx >= EPCnt )
      {
        state = eCacheDone;
      }
      else
      {
        SimpleDescriptionFormat_t *sDesc;
        byte free = afFindSimpleDesc( &sDesc, EPArr[reqIdx++] );

        if ( sDesc != NULL )
        {
          cmd = Simple_Desc_store_req;
          len += 1 + 8 + sDesc->AppNumInClusters + sDesc->AppNumOutClusters;
          *ptr++ = 8 + sDesc->AppNumInClusters + sDesc->AppNumOutClusters;
          *ptr++ = sDesc->EndPoint;
          *ptr++ = LO_UINT16( sDesc->AppProfId );
          *ptr++ = HI_UINT16( sDesc->AppProfId );
          *ptr++ = LO_UINT16( sDesc->AppDeviceId );
          *ptr++ = HI_UINT16( sDesc->AppDeviceId );
          *ptr++ = (sDesc->Reserved << 4) | sDesc->AppDevVer;
          *ptr++ = sDesc->AppNumInClusters;
          ptr = osal_memcpy( ptr, sDesc->pAppInClusterList,
                                                      sDesc->AppNumInClusters );
          *ptr++ = sDesc->AppNumOutClusters;
          osal_memcpy(ptr, sDesc->pAppOutClusterList, sDesc->AppNumOutClusters);

          if ( free )
          {
            osal_mem_free( sDesc );
          }
        }
      }
    }
    else
    {
      strtFind = TRUE;
    }
    break;

  case eCacheDone:
#if !defined( NWK_AUTO_POLL )
    NLME_SetPollRate( 0 );  // Stop the timer to allow max power savings.
#endif
    break;

  default:
    break;
  }  // switch ( state )

  if ( strtFind )
  {
    if ( state >= eNodeDescStore )
    {
      SendMsg( Remove_node_cache_req, 2+Z_EXTADDR_LEN, msg );
    }

    state = eCacheFind;
    if ( ++radius > (2 * _NIB.MaxDepth) )
    {
      radius = 1;
    }
    msgAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
    cacheCnt = 0;
    cmd = Discovery_Register_req;
  }
  cacheRsp = ZDP_TIMEOUT;
  wCnt = 0;

  if ( cmd != 0 )
  {
    SendMsg( cmd, len, msg );
  }
  osal_mem_free( msg );
#endif
}

#if ( CACHE_DEV_MAX > 0 )
/*********************************************************************
 * @fn          ZDCacheProcessReq
 *
 * @brief       Build and send a response to a Discovery Cache request.
 *
 */
void ZDCacheProcessReq( zAddrType_t *src, byte *msg, byte len,
                                                 byte cmd, byte seq, byte sty )
{
  byte status = ZDP_SUCCESS;
  byte sent = FALSE;

  msgAddr.addr.shortAddr = src->addr.shortAddr;
  tranSeq = seq;
  secUse = sty;

  if ( !CACHE_SERVER )
  {
    // Broadcast reqs without a negative response specified.
    if ( (cmd == Discovery_Register_req) || (cmd == Find_node_cache_req) )
    {
      sent = TRUE;
    }
    else
    {
      status = ZDP_NOT_SUPPORTED;
    }
  }
  else if ( cmd == Discovery_Register_req )
  {
    if ( getIdx( INVALID_NODE_ADDR ) == CACHE_DEV_MAX )
    {
      status = ZDP_TABLE_FULL;
    }
  }
  else if ( cmd == Discovery_store_req )
  {
    status = processDiscoveryStoreReq( msg );
  }
  else if ( cmd == Find_node_cache_req )
  {
    processFindNodeCacheReq( msg );
    sent = TRUE;
  }
  else if ( cmd == Mgmt_Cache_req )
  {
    processMgmtCacheReq( *msg );
    sent = TRUE;
  }
  else
  {
    uint16 aoi = BUILD_UINT16( msg[0], msg[1] );
    byte idx = getIdx( aoi );

    if ( cmd == Remove_node_cache_req )
    {
      if ( (purgeAddr( aoi ) + purgeIEEE( msg+2 )) == 0 )
      {
        status = ZDP_NOT_PERMITTED;
      }
    }
    else if ( idx == CACHE_DEV_MAX )
    {
      status = ZDP_NOT_PERMITTED;
    }
    else
    {
      msg += (2 + Z_EXTADDR_LEN);

      switch ( cmd )
      {
      case Node_Desc_store_req:
        osal_memcpy( NodeDesc+idx, msg, sizeof( NodeDescriptorFormat_t ) );
        break;

      case Power_Desc_store_req:
        osal_memcpy( NodePwr+idx, msg, sizeof( NodePowerDescriptorFormat_t ) );
        break;

      case Active_EP_store_req:
        if ( *msg < CACHE_EP_MAX )
        {
          EPCnt[idx] = *msg++;
          osal_memcpy( EPArr+idx, msg, EPCnt[idx] );
        }
        else
        {
          status = ZDP_INSUFFICIENT_SPACE;
        }
        break;

      case Simple_Desc_store_req:
        status = processSimpleDescStoreReq( msg, idx );
        break;
      }
    }
  }

  if ( !sent )
  {
    SendMsg( (cmd | ZDO_RESPONSE_BIT ), 1, &status );
  }
}

/*********************************************************************
 * @fn        ZDCacheProcessMatchDescReq
 *
 * @brief     Build and send a response to a Broadcast Discovery Cache request.
 *
 * @return    None.
 */
void ZDCacheProcessMatchDescReq( byte seq, zAddrType_t *src,
                 byte inCnt, uint16 *inClusters, byte outCnt, uint16 *outClusters,
                                       uint16 profileID, uint16 aoi, byte sty )
{
  byte buf[ 1 + 2 + 1 + CACHE_EP_MAX ];  // Status + AOI + Len + EP list.
  byte aoiMatch = (aoi == NWK_BROADCAST_SHORTADDR) ? TRUE : FALSE;
  SimpleDescriptionFormat_t *sDesc;
  byte idx, epIdx, epCnt;
  secUse = sty;

  if ( !CACHE_SERVER )
  {
    return;
  }

  buf[0] = ZDP_SUCCESS;
  buf[1] = LO_UINT16( aoi );
  buf[2] = HI_UINT16( aoi );
  buf[3] = 0;

  msgAddr.addr.shortAddr = src->addr.shortAddr;

  // Respond by proxy for all devices that have registered an endpoint.
  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( ((aoi == NWK_BROADCAST_SHORTADDR) &&
          (NwkAddr[idx] != INVALID_NODE_ADDR)) || (aoi == NwkAddr[idx]) )
    {
      sDesc = SimpDesc[idx];
      epCnt = 4;

      for ( epIdx = 0; epIdx < EPCnt[idx]; epIdx++, sDesc++ )
      {
        if ( sDesc->AppProfId == profileID )
        {
          // If there are no search input/ouput clusters - respond.
          if ( ((inCnt == 0) && (outCnt == 0))                               ||
                 ZDO_AnyClusterMatches( inCnt, inClusters,
                         sDesc->AppNumInClusters, sDesc->pAppInClusterList ) ||
                 ZDO_AnyClusterMatches( outCnt, outClusters,
                         sDesc->AppNumOutClusters, sDesc->pAppOutClusterList ))
          {
            buf[epCnt++] = sDesc->EndPoint;
          }
        }
      }

      if ( epCnt > 4 )
      {
        buf[1] = LO_UINT16( NwkAddr[idx] );
        buf[2] = HI_UINT16( NwkAddr[idx] );
        buf[3] = epCnt-4;
        SendMsg( Match_Desc_rsp, epCnt, buf );
      }

      if ( aoi == NwkAddr[idx] )
      {
        aoiMatch = TRUE;

        if ( epCnt == 4 )
        {
          buf[0] = ZDP_NO_MATCH;
          SendMsg( Match_Desc_rsp, 4, buf );
        }
        break;
      }
    }
  }

  if ( !aoiMatch )
  {
    buf[0] = ZDP_DEVICE_NOT_FOUND;
    SendMsg( Match_Desc_rsp, 4, buf );
  }
}

/*********************************************************************
 * @fn          ZDCacheGetDesc
 *
 * @brief       Return the cached descriptor requested.
 *
 * @param       aoi - NWK AddrOfInterest
 *
 * @return      The corresponding descriptor *, if the aoi is found,
 *              NULL otherwise.
 */
void *ZDCacheGetDesc( uint16 aoi, eDesc_t type, byte *stat )
{
  byte epIdx, idx = getIdx( aoi );
  void *rtrn = NULL;
  byte *ptr;

  if ( idx != CACHE_DEV_MAX )
  {
    byte cnt;

    *stat = ZDP_SUCCESS;

    switch ( type )
    {
    case eNodeDesc:
      rtrn = (void *)(NodeDesc+idx);
      break;

    case ePowerDesc:
      rtrn = (void *)(NodePwr+idx);
      break;

    case eActEPDesc:
      /* Tricky overload of return values:
       *   if rtrn val is zero (NULL), status is put in stat[0] -
       *                            either ZDP_DEVICE_NOT_FOUND or ZDP_SUCCESS.
       *   otherwise, w/ rtrn val > 0, stat is loaded byte for byte w/
       *     all active endpts other than ZDO. Thus, stat must be pointing to
       *     an array of at least CACHE_EP_MAX bytes.
       */
      ptr = EPArr[ idx ];
      for ( cnt = 0, idx = 0; idx < CACHE_EP_MAX; idx++ )
      {
        if ( ptr[idx] == 0xff )
          break;
        else if ( ptr[idx] == ZDO_EP )
          continue;
        else
        {
          *stat++ = ptr[idx];
          cnt++;
        }
      }
      rtrn = (void *)cnt;
      break;

    case eSimpDesc:
      // Tricky re-use of status parameter as endPoint to find.
      epIdx = getIdxEP( idx, *stat );

      if ( epIdx == CACHE_EP_MAX )
      {
        *stat = ZDP_DEVICE_NOT_FOUND;
      }
      else
      {
        rtrn = (void *)(&SimpDesc[idx][epIdx]);
      }
      break;

    default:
      *stat = ZDP_INVALID_REQTYPE;
      break;
    }
  }
  else
  {
    *stat = ZDP_DEVICE_NOT_FOUND;
  }

  return rtrn;
}

/*********************************************************************
 * @fn      ZDCacheGetNwkAddr
 *
 * @brief   Find the Network Address of a discovery cache entry that
 *          corresponds to the given IEEE address.
 *
 * @param   byte * - a valid buffer containing an extended IEEE address.
 *
 * @return  If address found, return the nwk addr, else INVALIDE_NODE_ADDR.
 *
 */
uint16 ZDCacheGetNwkAddr( byte *ieee )
{
  uint16 addr = INVALID_NODE_ADDR;
  byte idx;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( osal_ExtAddrEqual( ieee, ExtAddr[idx] ) &&
                                          (NwkAddr[idx] != INVALID_NODE_ADDR) )
    {
      addr = NwkAddr[idx];
      break;
    }
  }

  return addr;
}

/*********************************************************************
 * @fn      ZDCacheGetExtAddr
 *
 * @brief   Find the Extended Address of a discovery cache entry that
 *          corresponds to the given Network address.
 *
 * @param   aoi - the Network Address of interest.
 *
 * @return  If address found, return a ptr to the extended ieee addr, else NULL.
 *
 */
byte * ZDCacheGetExtAddr( uint16 aoi )
{
  byte *ieee = NULL;
  byte idx;

  for ( idx = 0; idx < CACHE_DEV_MAX; idx++ )
  {
    if ( aoi == NwkAddr[idx] )
    {
      ieee = ExtAddr[idx];
      break;
    }
  }

  return ieee;
}
#endif

#if ( CACHE_DEV_MAX == 0 )
/*********************************************************************
 * @fn          ZDCacheProcessRsp
 *
 * @brief       Process a response to a Discovery Cache request.
 *
 * @return      none
 */
void ZDCacheProcessRsp(
                    zAddrType_t *src, byte *msg, byte len, byte cmd, byte seq )
{
  if ( ((cmd==Find_node_cache_rsp) && (*msg==ZDP_NOT_SUPPORTED) && (len==3)) ||
       ((cmd!=Find_node_cache_rsp) &&
                             ((*msg != ZDP_SUCCESS) || (seq != (tranSeq-1)))) )
  {
    return;
  }

  cacheRsp = ZDP_SUCCESS;

  switch ( cmd )
  {
  case Discovery_Register_rsp:
  case Find_node_cache_rsp:
    if ( cacheCnt < FIND_RSP_MAX )
    {
      byte idx;

      // Only log unique response addresses.
      for ( idx = 0; idx < cacheCnt; idx++ )
      {
        if ( cacheFindAddr[idx] == src->addr.shortAddr )
        {
          break;
        }
      }

      if ( idx == cacheCnt )
      {
        cacheFindAddr[cacheCnt++] = src->addr.shortAddr;
      }
    }
    break;

  default:
    break;
  }

}
#endif


/*********************************************************************
*********************************************************************/

#endif  //#if defined( ZDO_CACHE )
