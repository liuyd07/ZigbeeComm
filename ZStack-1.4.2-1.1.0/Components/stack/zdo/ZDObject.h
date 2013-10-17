#ifndef ZDOBJECT_H
#define ZDOBJECT_H

/*********************************************************************
    Filename:       ZDObject.h
    Revised:        $Date: 2006-09-22 13:22:02 -0700 (Fri, 22 Sep 2006) $
    Revision:       $Revision: 12074 $
    Description:

      This file contains the interface to the Zigbee Device Object.

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
#include "NLMEDE.h"
#include "ZDApp.h"
#include "ZDCache.h"

/*********************************************************************
 * CONSTANTS
 */
#define ZDO_MAX_LQI_ITEMS       3
#define ZDO_MAX_NWKDISC_ITEMS   5
#define ZDO_MAX_RTG_ITEMS       10
#define ZDO_MAX_BIND_ITEMS      3

/*********************************************************************
 * TYPEDEFS
 */
typedef enum
{
  MODE_JOIN,
  MODE_RESUME,
//MODE_SOFT,      // Not supported yet
  MODE_HARD,
  MODE_REJOIN
} devStartModes_t;

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS - API
 */

/*
 * ZDO_Init - ZDObject and ZDApp Initialization.
 */
extern void ZDO_Init( void );

/*
 * ZDO_StartDevice - Start the device in a network
 */
extern void ZDO_StartDevice( byte logicalType, devStartModes_t startMode,
                             byte beaconOrder, byte superframeOrder );

/*
 * ZDO_UpdateNwkStatus - Update nwk state in the apps
 */
extern void ZDO_UpdateNwkStatus( devStates_t state );

/*
 * ZDO_DoEndDeviceBind - Perform the End Device Bind
 */
extern void ZDO_DoEndDeviceBind( ZDEndDeviceBind_t *bindReq );

/*
 * ZDO_MatchEndDeviceBind - Match End Device Bind Requests
 */
extern void ZDO_MatchEndDeviceBind( ZDEndDeviceBind_t *bindReq );

/*********************************************************************
 * Call Back Functions from ZDProfile  - API
 */

extern byte ZDO_AnyClusterMatches(
                              byte ACnt, uint16 *AList, byte BCnt, uint16 *BList );

/*
 * ZDO_ProcessNodeDescReq - Process the Node_Desc_req message.
 */
extern void ZDO_ProcessNodeDescReq(
                             byte seq, zAddrType_t *src, byte *aoi, byte sty );

/*
 * ZDO_ProcessPowerDescReq - Process the Power_Desc_req message.
 */
extern void ZDO_ProcessPowerDescReq(
                             byte seq, zAddrType_t *src, byte *aoi, byte sty );

/*
 * ZDO_ProcessSimpleDescReq - Process the Simple_Desc_req message
 */
extern void ZDO_ProcessSimpleDescReq(
                             byte seq, zAddrType_t *src, byte *msg, byte sty );

/*
 * ZDO_ProcessActiveEPReq - Process the Active_EP_req message
 */
extern void ZDO_ProcessActiveEPReq(
                             byte seq, zAddrType_t *src, byte *msg, byte sty );

/*
 * ZDO_ProcessMatchDescReq - Process the Match_Desc_req message
 */
extern void ZDO_ProcessMatchDescReq(
                             byte seq, zAddrType_t *src, byte *msg, byte sty );

/*
 * ZDO_ProcessEndDeviceBindReq - Process the End_Device_Bind_req message
 */
extern void ZDO_ProcessEndDeviceBindReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                              byte SecurityUse );

/*
 * ZDO_ProcessBindUnBindReq - Process the Bind_req and Unbind_req messages
 */
extern void ZDO_ProcessBindUnbindReq( byte TransSeq, zAddrType_t *SrcAddr, uint16 ClusterID,
                                     byte *msg, byte SecurityUse );

/*
 * ZDO_ProcessAddrRsp - Process the NWK_addr_rsp or IEEE_addr_rsp message.
 */
extern void ZDO_ProcessAddrRsp( zAddrType_t *src, uint16 cId, byte *msg, byte msgLen );

/*
 * ZDO_ProcessNodeDescRsp - Process the Node_Desc_rsp message
 */
extern void ZDO_ProcessNodeDescRsp( zAddrType_t *SrcAddress, byte *msg );

/*
 * ZDO_ProcessPowerDescRsp - Process the Power_Desc_rsp message
 */
extern void ZDO_ProcessPowerDescRsp( zAddrType_t *SrcAddr, byte *msg );

/*
 * ZDO_ProcessSimpleDescRsp - Process the Simple_Desc_rsp message
 */
extern void ZDO_ProcessSimpleDescRsp( zAddrType_t *SrcAddr, byte *msg );

/*
 * ZDO_ProcessEPListRsp - Process the Active_EP_rsp or
 *                          Match_Desc_rsp message
 */
extern void ZDO_ProcessEPListRsp( zAddrType_t *SrcAddr,
                                   uint16 ClusterID, byte *msg );

/*
 * ZDO_ProcessBindUnbindRsp - Process the End_Device_Bind_rsp, Bind_rsp,r
 *                          or Unbind_rsp message
 */
extern void ZDO_ProcessBindUnbindRsp( zAddrType_t *SrcAddr,
                              uint16 ClusterID, byte Status, uint8 TransSeq );
/*
 * ZDO_ProcessServerDiscRsp - Process the Server_Discovery_rsp message.
 */
#if defined ( ZDO_SERVERDISC_REQUEST )
void ZDO_ProcessServerDiscRsp(zAddrType_t *srcAddr,byte *msg, byte SecurityUse);
#endif

/*
 * ZDO_ProcessServerDiscReq - Process the Server_Discovery_req message.
 */
#if defined ( ZDO_SERVERDISC_RESPONSE )
void ZDO_ProcessServerDiscReq( byte transID, zAddrType_t *srcAddr, byte *msg,
                               byte SecurityUse );
#endif

/*********************************************************************
 * Call Back Functions from APS  - API
 */

/*
 * ZDO_EndDeviceTimeoutCB - Called when the binding timer expires
 */
extern void ZDO_EndDeviceTimeoutCB( void );

/*********************************************************************
 * Optional Management Messages
 */

#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
  /*
   * ZDO_ProcessMgmtNwkDiscReq - Called to parse the incoming
   * Management Network Discover Response
   */
  extern void ZDO_ProcessMgmNwkDiscRsp( zAddrType_t *SrcAddr,
                                            byte *msg, byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_NWKDISC_RESPONSE )
  /*
   * ZDO_ProcessMgmtNwkDiscReq - Called to parse the incoming
   * Management LQI Request
   */
  extern void ZDO_ProcessMgmtNwkDiscReq( byte TransSeq, zAddrType_t *SrcAddr,
                                         byte *msg, byte SecurityUse );

  /*
   * ZDO_FinishProcessingMgmtNwkDiscReq - Called to parse the incoming
   * Management LQI Request
   */
  extern void ZDO_FinishProcessingMgmtNwkDiscReq(byte ResultCount,
                                             networkDesc_t *NetworkList );
#endif

#if defined ( ZDO_MGMT_LQI_REQUEST )
  /*
   * ZDO_ProcessMgmtLqiRsp - This function handles parsing the incoming
   * Management LQI response and then generates a callback to the
   * ZD application.
   */
  extern void ZDO_ProcessMgmtLqiRsp( zAddrType_t *SrcAddr,
                                     byte *msg, byte SecurityUse );
#endif

#if defined( ZDO_MGMT_LQI_RESPONSE )
  /*
   * ZDO_ProcessMgmtLqiReq - Called to parse the incoming
   * Management LQI Request
   */
  extern void ZDO_ProcessMgmtLqiReq( byte TransSeq, zAddrType_t *SrcAddr, byte StartIndex,
                                     byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_RTG_REQUEST )
  /*
   * ZDO_ProcessMgmtRtgRsp - This function handles parsing the incoming
   * Management Routing response and then generates a callback to the
   * ZD application.
   */
  extern void ZDO_ProcessMgmtRtgRsp( zAddrType_t *SrcAddr,
                                          byte *msg, byte SecurityUse );
#endif

#if defined( ZDO_MGMT_RTG_RESPONSE )
  /*
   * ZDO_ProcessMgmtRtgReq - Called to parse the incoming
   * Management Routing Request
   */
  extern void ZDO_ProcessMgmtRtgReq( byte TransSeq, zAddrType_t *SrcAddr,
                                    byte StartIndex, byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_BIND_RESPONSE )
  extern void ZDO_ProcessMgmtBindReq( byte TransSeq, zAddrType_t *SrcAddr, byte StartIndex,
                                      byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_BIND_REQUEST )
  extern void ZDO_ProcessMgmtBindRsp( zAddrType_t *SrcAddr, byte *msg,
                                            byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_JOINDIRECT_RESPONSE ) && defined ( RTR_NWK )
  extern void ZDO_ProcessMgmtDirectJoinReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                                          byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_JOINDIRECT_REQUEST )
  extern void ZDO_ProcessMgmtDirectJoinRsp( zAddrType_t *SrcAddr, byte Status,
                                          byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_LEAVE_RESPONSE )
  extern void ZDO_ProcessMgmtLeaveReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                                       byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_PERMIT_JOIN_REQUEST )
  extern void ZDO_ProcessMgmtPermitJoinRsp( zAddrType_t *SrcAddr, byte Status,
                                            byte SecurityUse);
#endif

#if defined ( ZDO_MGMT_PERMIT_JOIN_RESPONSE )
  extern void ZDO_ProcessMgmtPermitJoinReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                                       byte SecurityUse );
#endif

#if defined ( ZDO_MGMT_LEAVE_REQUEST )
  extern void ZDO_ProcessMgmtLeaveRsp( zAddrType_t *SrcAddr, byte Status,
                                       byte SecurityUse );
#endif

#if defined ( ZDO_USERDESC_REQUEST )
  extern void ZDO_ProcessUserDescRsp( zAddrType_t *SrcAddr, byte *msg,
                                      byte SecurityUse );
#endif

#if defined ( ZDO_USERDESC_RESPONSE )
  extern void ZDO_ProcessUserDescReq( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                                      byte SecurityUse );
#endif

#if defined ( ZDO_USERDESCSET_REQUEST )
  extern void ZDO_ProcessUserDescConf( zAddrType_t *SrcAddr, byte *msg,
                                       byte SecurityUse );
#endif

#if defined ( ZDO_USERDESCSET_RESPONSE )
  extern void ZDO_ProcessUserDescSet( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                                      byte SecurityUse );
#endif

#if defined ( ZDO_ENDDEVICE_ANNCE )
void ZDO_ProcessEndDeviceAnnce( byte TransSeq, zAddrType_t *SrcAddr, byte *msg,
                                byte SecurityUse );
#endif

#if defined ( ZDO_SIMPLEDESC_REQUEST ) || \
                                (defined( ZDO_CACHE ) && ( CACHE_DEV_MAX > 0 ))
void ZDO_BuildSimpleDescBuf( byte *buf, SimpleDescriptionFormat_t *desc );

uint8 ZDO_ParseSimpleDescBuf( byte *buf, SimpleDescriptionFormat_t *desc );
#endif

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZDOBJECT_H */
