#ifndef RTG_H
#define RTG_H

/*********************************************************************
    Filename:       rtg.h
    Revised:        $Date: 2007-01-08 12:56:09 -0800 (Mon, 08 Jan 2007) $
    Revision:       $Revision: 13236 $
    Description:

      Interface to mesh routing functions

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

#if defined (RTR_NWK)

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "nwk_util.h"
#include "nwk_bufs.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define RTG_TIMER_INTERVAL            1000

/*********************************************************************
 * TYPEDEFS
 */

typedef enum
{
  RTG_SUCCESS,
  RTG_FAIL,
  RTG_TBL_FULL,
  RTG_HIGHER_COST,
  RTG_NO_ENTRY,
  RTG_INVALID_PATH,
  RTG_INVALID_PARAM
} RTG_Status_t;

// status values for routing entries
#define RT_INIT       0
#define RT_ACTIVE     1
#define RT_DISC       2
#define RT_LINK_FAIL  3
#define RT_REPAIR     4

// Routing table entry
//   Notice, if you change this structure, you must also change
//   rtgItem_t in ZDProfile.h
typedef struct
{
  uint16  dstAddress;
  uint16  nextHopAddress;
  byte    expiryTime;
  byte    status;
} rtgEntry_t;

// Route discovery table entry
typedef struct
{
  byte    rreqId;
  uint16  srcAddress;
  uint16  previousNode;
  byte    forwardCost;
  byte    residualCost;
  byte    expiryTime;
} rtDiscEntry_t;

// Broadcast table entry.
typedef struct
{
  uint16 srcAddr;
  uint8  bdt; // broadcast delivery time
  uint8  pat; // passive ack timeout
  uint8  mbr; // max broadcast retries
  uint8  handle;
  // Count of non-sleeping neighbors and router children.
  uint8  ackCnt;
  uint8  id;
} bcastEntry_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern rtgEntry_t rtgTable[];
extern rtDiscEntry_t rtDiscTable[];

/*********************************************************************
 * FUNCTIONS
 */

extern void RTG_Init( void );

extern byte RTG_GetRtgEntry( uint16 DstAddress );

extern RTG_Status_t RTG_RemoveRtgEntry( uint16 DstAddress );

extern uint16 RTG_GetNextHop( uint16 DstAddress );

extern byte RTG_ProcessRreq(
           NLDE_FrameFormat_t *ff, uint16 macSrcAddress, uint16 *nextHopAddr );

extern void RTG_ProcessRrep( NLDE_FrameFormat_t *ff );

extern void RTG_ProcessRErr( NLDE_FrameFormat_t *ff );

extern void RTG_TimerEvent( void );

extern uint16 RTG_AllocNewAddress( byte deviceType );

extern void RTG_DeAllocAddress( uint16 shortAddr );

extern void RTG_BcastTimerHandler( void );

extern byte RTG_BcastChk( NLDE_FrameFormat_t *ff, uint16 macSrcAddr );

extern byte RTG_BcastAdd(NLDE_FrameFormat_t*ff, uint16 macSrcAddr, byte handle);

extern void RTG_BcastDel( byte handle );

extern void RTG_DataReq( ZMacPollInd_t *param );

extern byte RTG_PoolAdd( NLDE_FrameFormat_t *ff );

extern uint16 RTG_GetTreeRoute( uint16 dstAddress );

extern RTG_Status_t RTG_CheckRtStatus( uint16 DstAddress, byte RtStatus );

extern uint8 RTG_ProcessRtDiscBits( uint8 rtDiscFlag, uint16 dstAddress );

extern uint8 RTG_RouteMaintanence( uint16 DstAddress, uint16 SrcAddress );

extern void RTG_FillCSkipTable( byte *children, byte *routers,
                                byte depth, uint16 *pTbl );


extern uint8 RTG_IsAncestor( uint16 deviceAddress  );

#endif// RTR_NWK

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* RTG_H */
