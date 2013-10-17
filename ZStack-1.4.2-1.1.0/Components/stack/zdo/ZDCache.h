#ifndef ZDCACHE_H
#define ZDCACHE_H

/*********************************************************************
    Filename:       ZDCache.h
    Revised:        $Date: 2006-04-27 11:56:00 -0700 (Thu, 27 Apr 2006) $
    Revision:       $Revision: 10625 $

    Description: Declaration of the ZDO Discovery Cache functionality.

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

// Max EndPoints cached per requesting device.
#if !defined( CACHE_EP_MAX )
  #define CACHE_EP_MAX  4
#endif

#if defined( ZDO_CACHE )

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/*********************************************************************
 * MACROS
 */

#if defined( RTR_NWK )
  // Max devs allowed to cache discovery info.
  #if !defined( CACHE_DEV_MAX )
    #define CACHE_DEV_MAX  2
  #endif
  #define CACHE_SERVER ( ZDO_Config_Node_Descriptor.ServerMask &\
                                         (PRIM_DISC_TABLE | BKUP_DISC_TABLE) )
#else
  #define CACHE_DEV_MAX  0
  #define CACHE_SERVER   0

  // Seconds to wait after powerup before starting to upload discovery cache.
  #if !defined( WAIT_TO_STORE_CACHE )
    #define WAIT_TO_STORE_CACHE  30
  #endif
#endif

// Max Clusters cached per endpoint per requesting device.
#if !defined( CACHE_CR_MAX )
  #define CACHE_CR_MAX  4
#endif

#define Discovery_store_req     ((uint16)0x0015)
#define Node_Desc_store_req     ((uint16)0x0016)
#define Power_Desc_store_req    ((uint16)0x0017)
#define Active_EP_store_req     ((uint16)0x0018)
#define Simple_Desc_store_req   ((uint16)0x0019)
#define Remove_node_cache_req   ((uint16)0x001a)
#define Find_node_cache_req     ((uint16)0x001b)
#define Mgmt_Cache_req          ((uint16)0x001c) //ggg - ?
#define Discovery_store_rsp     ( Discovery_store_req   | ZDO_RESPONSE_BIT )
#define Node_Desc_store_rsp     ( Node_Desc_store_req   | ZDO_RESPONSE_BIT )
#define Power_Desc_store_rsp    ( Power_Desc_store_req  | ZDO_RESPONSE_BIT )
#define Active_EP_store_rsp     ( Active_EP_store_req   | ZDO_RESPONSE_BIT )
#define Simple_Desc_store_rsp   ( Simple_Desc_store_req | ZDO_RESPONSE_BIT )
#define Remove_node_cache_rsp   ( Remove_node_cache_req | ZDO_RESPONSE_BIT )
#define Find_node_cache_rsp     ( Find_node_cache_req   | ZDO_RESPONSE_BIT )
#define Mgmt_Cache_rsp          ( Mgmt_Cache_req        | ZDO_RESPONSE_BIT )

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

// Used for the type parameter in call to ZDCacheGetDesc().
typedef enum {
  eNodeDesc,
  ePowerDesc,
  eActEPDesc,
  eSimpDesc
} eDesc_t;


/*********************************************************************
 * GLOBAL VARIABLES
 */



extern void ZDCacheInit( void );

extern void ZDCacheTimerEvent( void );

#if ( CACHE_DEV_MAX > 0 )
extern void ZDCacheProcessReq( zAddrType_t *src, byte *msg, byte len,
                                                byte cmd, byte seq, byte sty );

extern void ZDCacheProcessMatchDescReq( byte seq, zAddrType_t *src,
                byte inCnt, uint16 *inClusters, byte outCnt, uint16 *outClusters,
                                      uint16 profileID, uint16 aoi, byte sty );

extern void *ZDCacheGetDesc( uint16 aoi, eDesc_t type, byte *status );

extern uint16 ZDCacheGetNwkAddr( byte *ieee );

extern byte * ZDCacheGetExtAddr( uint16 aoi );


#endif

#if ( CACHE_DEV_MAX == 0 )
extern void ZDCacheProcessRsp(
                   zAddrType_t *src, byte *msg, byte len, byte cmd, byte seq );
#endif

/*********************************************************************
*********************************************************************/

#endif  //#if defined( ZDO_CACHE )

#ifdef __cplusplus
}
#endif

#endif /* ZDCACHE_H */
