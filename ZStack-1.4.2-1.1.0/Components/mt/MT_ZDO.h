/*********************************************************************
    Filename:       MT_ZDO.h
    Revised:        $Date: 2006-11-30 12:05:15 -0800 (Thu, 30 Nov 2006) $
    Revision:       $Revision: 12906 $

    Description:

        MonitorTest functions for APS.

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
#include "ZComDef.h"
#include "MTEL.h"
#include "APSMEDE.h"
#include "AF.h"
#include "ZDProfile.h"
#include "ZDObject.h"
#include "ZDApp.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
//ZDO commands
#define SPI_CMD_ZDO_AUTO_ENDDEVICEBIND_REQ    0x0A00
#define SPI_CMD_ZDO_AUTO_FIND_DESTINATION_REQ 0x0A01
#define SPI_CMD_ZDO_NWK_ADDR_REQ              0x0A02
#define SPI_CMD_ZDO_IEEE_ADDR_REQ             0x0A03
#define SPI_CMD_ZDO_NODE_DESC_REQ             0x0A04
#define SPI_CMD_ZDO_POWER_DESC_REQ            0x0A05
#define SPI_CMD_ZDO_SIMPLE_DESC_REQ           0x0A06
#define SPI_CMD_ZDO_ACTIVE_EPINT_REQ          0x0A07
#define SPI_CMD_ZDO_MATCH_DESC_REQ            0x0A08
#define SPI_CMD_ZDO_COMPLEX_DESC_REQ          0x0A09
#define SPI_CMD_ZDO_USER_DESC_REQ             0x0A0A
#define SPI_CMD_ZDO_END_DEV_BIND_REQ          0x0A0B
#define SPI_CMD_ZDO_BIND_REQ                  0x0A0C
#define SPI_CMD_ZDO_UNBIND_REQ                0x0A0D
#define SPI_CMD_ZDO_MGMT_NWKDISC_REQ          0x0A0E
#define SPI_CMD_ZDO_MGMT_LQI_REQ              0x0A0F
#define SPI_CMD_ZDO_MGMT_RTG_REQ              0x0A10
#define SPI_CMD_ZDO_MGMT_BIND_REQ             0x0A11
#define SPI_CMD_ZDO_MGMT_DIRECT_JOIN_REQ      0x0A12
#define SPI_CMD_ZDO_USER_DESC_SET             0x0A13
#define SPI_CMD_ZDO_END_DEV_ANNCE             0x0A14
#define SPI_CMD_ZDO_MGMT_LEAVE_REQ            0x0A15
#define SPI_CMD_ZDO_MGMT_PERMIT_JOIN_REQ      0x0A16
#define SPI_CMD_ZDO_SERVERDISC_REQ            0X0A17
#define SPI_CMD_ZDO_NETWORK_START_REQ         0X0A18

#define SPI_ZDO_CB_TYPE                       0x0A80

#define SPI_CB_ZDO_NWK_ADDR_RSP               0x0A80
#define SPI_CB_ZDO_IEEE_ADDR_RSP              0x0A81
#define SPI_CB_ZDO_NODE_DESC_RSP              0x0A82
#define SPI_CB_ZDO_POWER_DESC_RSP             0x0A83
#define SPI_CB_ZDO_SIMPLE_DESC_RSP            0x0A84
#define SPI_CB_ZDO_ACTIVE_EPINT_RSP           0x0A85
#define SPI_CB_ZDO_MATCH_DESC_RSP             0x0A86
#define SPI_CB_ZDO_END_DEVICE_BIND_RSP        0x0A87
#define SPI_CB_ZDO_BIND_RSP                   0x0A88
#define SPI_CB_ZDO_UNBIND_RSP                 0x0A89
#define SPI_CB_ZDO_MGMT_NWKDISC_RSP           0x0A8A
#define SPI_CB_ZDO_MGMT_LQI_RSP               0x0A8B
#define SPI_CB_ZDO_MGMT_RTG_RSP               0x0A8C
#define SPI_CB_ZDO_MGMT_BIND_RSP              0x0A8D
#define SPI_CB_ZDO_MGMT_DIRECT_JOIN_RSP       0x0A8E
#define SPI_CB_ZDO_USER_DESC_RSP              0x0A8F

#define SPI_ZDO_CB2_TYPE                      0x0A90

#define SPI_CB_ZDO_USER_DESC_CNF              0x0A90
#define SPI_CB_ZDO_MGMT_LEAVE_RSP             0x0A91
#define SPI_CB_ZDO_MGMT_PERMIT_JOIN_RSP       0x0A92
#define SPI_CB_ZDO_SERVERDISC_RSP             0x0A93

#define SPI_RESP_LEN_ZDO_DEFAULT              0x01

#define CB_ID_ZDO_NWK_ADDR_RSP               0x00000001
#define CB_ID_ZDO_IEEE_ADDR_RSP              0x00000002
#define CB_ID_ZDO_NODE_DESC_RSP              0x00000004
#define CB_ID_ZDO_POWER_DESC_RSP             0x00000008
#define CB_ID_ZDO_SIMPLE_DESC_RSP            0x00000010
#define CB_ID_ZDO_ACTIVE_EPINT_RSP           0x00000020
#define CB_ID_ZDO_MATCH_DESC_RSP             0x00000040
#define CB_ID_ZDO_END_DEVICE_BIND_RSP        0x00000080
#define CB_ID_ZDO_BIND_RSP                   0x00000100
#define CB_ID_ZDO_UNBIND_RSP                 0x00000200
#define CB_ID_ZDO_MGMT_NWKDISC_RSP           0x00000400
#define CB_ID_ZDO_MGMT_LQI_RSP               0x00000800
#define CB_ID_ZDO_MGMT_RTG_RSP               0x00001000
#define CB_ID_ZDO_MGMT_BIND_RSP              0x00002000
#define CB_ID_ZDO_MGMT_DIRECT_JOIN_RSP       0x00004000
#define CB_ID_ZDO_USER_DESC_RSP              0x00008000
#define CB_ID_ZDO_USER_DESC_CONF             0x00010000
#define CB_ID_ZDO_MGMT_LEAVE_RSP             0x00020000
#define CB_ID_ZDO_MGMT_PERMIT_JOIN_RSP       0x00040000
#define CB_ID_ZDO_SERVERDISC_RSP             0x00080000

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern uint32 _zdoCallbackSub;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*
 *   Process all the NWK commands that are issued by test tool
 */
extern void MT_ZdoCommandProcessing( uint16 cmd_id , byte len , byte *pData );

/*********************************************************************
 * Callback FUNCTIONS
 */

/*
 *  NwkAddrRsp or IEEEAddrRsp
 *
 *  @MT SPI_CB_ZDO_NWK_ADDR_RSP
 *  (byte SrcAddrMode,
 *   byte *SrcAddr,
 *   byte Status,
 *   byte *IEEEAddr,
 *   uint16 nwkAddr,
 *   byte NumAssocDev,
 *   byte StartIndex,
 *   byte *AssocDevList)
 *
 *  @MT SPI_CB_ZDO_IEEE_ADDR_RSP
 *  (byte SrcAddrMode,
 *   byte *SrcAddr,
 *   byte Status,
 *   byte *IEEEAddr,
 *   byte NumAssocDev,
 *   byte StartIndex,
 *   byte *AssocDevList)
 *
 */
extern void zdo_MTCB_NwkIEEEAddrRspCB( uint16 type, zAddrType_t *SrcAddr,
              byte Status, byte *IEEEAddr, uint16 nwkAddr, byte NumAssocDev,
              byte StartIndex, uint16 *AssocDevList );

/*
 *  NodeDescRsp
 *
 *  @MT SPI_CB_ZDO_NODE_DESC_RSP
 *  (byte Status,
 *   uint16 SrcAddr,
 *   uint16 NWKAddrOfInterest,
 *   byte NodeType,
 *   byte Flags,
 *   byte Capabilities,
 *   byte BufferSize,
 *   uint16 TransferSize)
 */
extern void zdo_MTCB_NodeDescRspCB( zAddrType_t *SrcAddr, byte Status,
            uint16 nwkAddr, NodeDescriptorFormat_t *pNodeDesc );

/*
 *  PowerDescRsp
 *
 *  @MT SPI_CB_ZDO_POWER_DESC_RSP
 *  (byte Status,
 *   uint16 SrcAddr,
 *   uint16 NWKAddrOfInterest,
 *   byte Power1,
 *   byte Power2)
 *
 */
extern void zdo_MTCB_PowerDescRspCB( zAddrType_t *SrcAddr, byte Status,
          uint16 nwkAddr, NodePowerDescriptorFormat_t *pPwrDesc );

/*
 *   SimpleDescRsp
 *
 *  @MT SPI_CB_ZDO_SIMPLE_DESC_RSP
 *  (byte Status,
 *   uint16 SrcAddr,
 *   uint16 NWKAddrOfInterest,
 *   byte SimpleLen,
 *   byte Endpoint,
 *   uint16 AppProfID,
 *   uint16 AppDevID,
 *   byte AppVerFlags,
 *   byte AppInClusterCount,
 *   byte AppInClusterList[15],
 *   byte AppOutClusterCount,
 *   byte AppOutClusterList[15])
 *
 */
extern void zdo_MTCB_SimpleDescRspCB( zAddrType_t *SrcAddr, byte Status,
    uint16 nwkAddr, byte EPIntf, SimpleDescriptionFormat_t *pSimpleDesc );

/*
 *   ActiveEPRsp or MatchDescRsp
 *
 *  @MT SPI_CB_ZDO_ACTIVE_EPINT_RSP
 *  (byte Status,
 *   uint16 SrcAddr,
 *   uint16 NWKAddrOfInterest,
 *   byte ActiveEndpointCount,
 *   byte ActiveEndpointList[15])
 *
 *  @MT SPI_CB_ZDO_MATCH_DESC_RSP
 *  (byte Status,
 *   unit16 SrcAddr,
 *   uint16 NWKAddrOfInterest,
 *   byte MatchCount,
 *   byte MatchEndpointList[15])
 *
 */
extern void zdo_MTCB_MatchActiveEPRspCB( uint16 type, zAddrType_t *SrcAddr, byte Status,
                  uint16 nwkAddr, byte epIntfCnt, byte *epIntfList );

/*
 *   BindRsp, EndDeviceBindRsp, UnBindRsp
 *
 *  @MT SPI_CB_ZDO_END_DEVICE_BIND_RSP
 *  (byte Status,
 *   UInt16 SrcAddr)
 *
 *  @MT SPI_CB_ZDO_BIND_RSP
 *  (byte Status,
 *   UInt16 SrcAddr)
 *
 *  @MT SPI_CB_ZDO_UNBIND_RSP
 *  (byte Status,
 *   UInt16 SrcAddr)
 *
 */
extern void zdo_MTCB_BindRspCB( uint16 type,
                  zAddrType_t *SrcAddr, byte Status );



/*
 *   Management Network Discovery Response
 *
 *  @MT SPI_CB_ZDO_MGMT_NWKDISC_RSP
 *  (uint16 SrcAddr,
 *   byte Status,
 *   byte NetworkCount,
 *   byte StartIndex,
 *   byte NetworkListCount,
 *   byte *NetworkList)
 *
 */
extern void zdo_MTCB_MgmtNwkDiscRspCB( uint16 SrcAddr, byte Status,
                        byte NetworkCount, byte StartIndex,
                        byte networkListCount, mgmtNwkDiscItem_t *pList );

/*
 *   Management LQI Response
 *
 *  @MT SPI_CB_ZDO_MGMT_LQI_RSP
 *  (uint16 SrcAddr,
 *   byte Status,
 *   byte NeighborLQIEntries,
 *   byte StartIndex,
 *   byte *NeighborLqiList)
 *
 */
extern void zdo_MTCB_MgmtLqiRspCB( uint16 SrcAddr, byte Status,
                         byte NeighborLqiEntries,
                         byte StartIndex, byte NeighborLqiCount,
                         neighborLqiItem_t *pList );

/*
 *   Management Routing Response
 *
 *  @MT SPI_CB_ZDO_MGMT_RTG_RSP
 *  (uint16 SrcAddr,
 *   byte Status,
 *   byte RtgCount,
 *   byte StartIndex,
 *   byte RtgListCount,
 *   byte *RtgList)
 *
 */
extern void zdo_MTCB_MgmtRtgRspCB( uint16 SrcAddr, byte Status,
                        byte RtgCount, byte StartIndex,
                        byte RtgListCount, rtgItem_t *pList );

/*
 *   Management Binding Response
 *
 *  @MT SPI_CB_ZDO_MGMT_BIND_RSP
 *  (uint16 SrcAddr,
 *   byte Status,
 *   byte BindCount,
 *   byte StartIndex,
 *   byte BindListCount,
 *   byte *BindList)
 *
 */
extern void zdo_MTCB_MgmtBindRspCB( uint16 SrcAddr, byte Status,
                        byte BindCount, byte StartIndex,
                        byte BindListCount, apsBindingItem_t *pList );

/*
 *   Management Direct Join Response
 *
 *  @MT SPI_CB_ZDO_MGMT_DIRECT_JOIN_RSP
 *  (uint16 SrcAddr,
 *   byte Status)
 *
 */
extern void zdo_MTCB_MgmtDirectJoinRspCB( uint16 SrcAddr, byte Status,
                        byte SecurityUse );

/*
 *   Management Leave Response
 *
 *  @MT SPI_CB_ZDO_MGMT_LEAVE_RSP
 *  (byte Status,
 *   uint16 SrcAddr)
 *
 */
extern void zdo_MTCB_MgmtLeaveRspCB( uint16 SrcAddr, byte Status,
                        byte SecurityUse );

/*
 *   Management Permit Join Response
 *
 *  @MT SPI_CB_ZDO_MGMT_PERMIT_JOIN_RSP
 *  (uint16 SrcAddr,
 *   byte Status)
 *
 */
extern void zdo_MTCB_MgmtPermitJoinRspCB( uint16 SrcAddr, byte Status,
                        byte SecurityUse );

/*
 *   User Descriptor Response
 *
 *  @MT SPI_CB_ZDO_USER_DESC_RSP
 *  (byte Status,
 *   uint16 SrcAddr)
 *
 */
extern void zdo_MTCB_UserDescRspCB( uint16 SrcAddr, byte status,
                          uint16 nwkAddrOfInterest,
                          byte userDescLen, byte *userDesc, byte SecurityUse );

/*
 *   User Descriptor Confirm
 *
 *  @MT SPI_CB_ZDO_USER_DESC_CNF
 *  (byte Status,
 *   uint16 SrcAddr)
 *
 */
extern void zdo_MTCB_UserDescConfCB( uint16 SrcAddr, byte status,
                          byte SecurityUse );

/*
 *  Server Discovery Response.
 *
 *  @MT SPI_CB_ZDO_SERVER_DISC_CNF
 *
 */
#if defined ( ZDO_SERVERDISC_REQUEST )
void zdo_MTCB_ServerDiscRspCB( uint16 srcAddr, byte status, 
                               uint16 serverMask, byte SecurityUse );
#endif

/*********************************************************************
*********************************************************************/
