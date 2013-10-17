#ifndef APSMEDE_H
#define APSMEDE_H
/******************************************************************************
    Filename:       APSMEDE.h
    Revised:        $Date: 2007-02-23 11:29:38 -0800 (Fri, 23 Feb 2007) $
    Revision:       $Revision: 13588 $

    Description:

    Primitives of the Application Support Sub Layer Data Entity (APSDE) and
    Management Entity (APSME).

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "nwk_globals.h"
#include "AssocList.h"
#include "nwk_bufs.h"
#include "BindingTable.h"
#include "ssp.h"

/******************************************************************************
 * MACROS
 */

/******************************************************************************
 * CONSTANTS
 */
// Frame control fields
#define APS_FRAME_TYPE_MASK         0x03
#define APS_DATA_FRAME              0x00
#define APS_CMD_FRAME               0x01
#define APS_ACK_FRAME               0x02

#define APS_DELIVERYMODE_MASK       0x0C
#define APS_FC_DM_UNICAST           0x00
#define APS_FC_DM_INDIRECT          0x04
#define APS_FC_DM_BROADCAST         0x08
#define APS_FC_DM_GROUP             0x0C

#define APS_FC_INDADDRMODE          0x10
#define APS_FC_SECURITY             0x20
#define APS_FC_ACK_REQ              0x40
#define APS_FC_EXTENDED             0x80

#define APS_XFC_FRAG_MASK           0x03
#define APS_XFC_FIRST_FRAG          0x01
#define APS_XFC_FRAGMENT            0x02

#define APS_FRAME_CTRL_FIELD_LEN     0x01
#define APS_DSTEP_ID_FIELD_LEN       0x01
#define APS_GROUP_ID_FIELD_LEN       0x02
#define APS_SRCEP_ID_FIELD_LEN       0x01
#define APS_CLUSTERID_FIELD_LEN_V1_0 0x01
#define APS_CLUSTERID_FIELD_LEN      0x02
#define APS_PROFILEID_FIELD_LEN      0x02
#define APS_FRAME_CNT_FIELD_LEN      0x01
#define APS_XFRAME_CTRL_FIELD_LEN    0x01
#define APS_BLOCK_CNT_FIELD_LEN      0x01
#define APS_ACK_BITS_FIELD_LEN       0x01

// Tx Options (bitmap values)
#define APS_TX_OPTIONS_SECURITY_ENABLE  0x01
//#define APS_TX_OPTIONS_USE_NWK_KEY    0x02 remove from spec
#define APS_TX_OPTIONS_ACK              0x04
#define APS_TX_OPTIONS_PERMIT_FRAGMENT  0x08
#define APS_TX_OPTIONS_SKIP_ROUTING     0x10
#define APS_TX_OPTIONS_FIRST_FRAGMENT   0x20

// APSDE header fields
#define APS_HDR_FC 0

// APSME CMD id index
#define APSME_CMD_ID 0

// APS commands
#define APSME_CMD_SKKE_1        1
#define APSME_CMD_SKKE_2        2
#define APSME_CMD_SKKE_3        3
#define APSME_CMD_SKKE_4        4
#define APSME_CMD_TRANSPORT_KEY 5
#define APSME_CMD_UPDATE_DEVICE 6
#define APSME_CMD_REMOVE_DEVICE 7
#define APSME_CMD_REQUEST_KEY   8
#define APSME_CMD_SWITCH_KEY    9

// APSME CMD packet fields (APSME_CMD_SKKE_*)
#define APSME_SKKE_METHOD      0
#define APSME_SKKE_INIT_ADDR   1
#define APSME_SKKE_RESP_ADDR   9
#define APSME_SKKE_PAYLOAD     17
#define APSME_SKKE_PAYLOAD_LEN SEC_KEY_LEN
#define APSME_SKKE_LEN         33

// APSME CMD packet fields (APSME_CMD_TRANSPORT_KEY)
#define APSME_TK_KEY_TYPE      1
#define APSME_TK_KEY           2
#define APSME_TK_COMMON_LEN    (uint8)         \
                               (APSME_TK_KEY + \
                                SEC_KEY_LEN   )
#define APSME_TK_KEY_SEQ_LEN   1
#define APSME_TK_INITIATOR_LEN 1

#define APSME_TK_MASTER_DST_ADDR  18
#define APSME_TK_MASTER_SRC_ADDR  26
#define APSME_TK_MASTER_KEY_LEN   34

#define APSME_TK_NWK_KEY_SEQ      18
#define APSME_TK_NWK_DST_ADDR     19
#define APSME_TK_NWK_SRC_ADDR     27
#define APSME_TK_NWK_KEY_LEN      35

#define APSME_TK_APP_PARTNER_ADDR 18
#define APSME_TK_APP_INITIATOR    26
#define APSME_TK_APP_KEY_LEN      27

// APSME CMD packet fields (APSME_CMD_UPDATE_DEVICE)
#define APSME_UD_SECURED_JOIN   0
#define APSME_UD_UNSECURED_JOIN 1
#define APSME_UD_LEAVE          2

#define APSME_UD_EADDR     1
#define APSME_UD_SADDR_LSB 9
#define APSME_UD_SADDR_MSB 10
#define APSME_UD_STATUS    11
#define APSME_UD_LEN       12

// APSME CMD packet fields (APSME_CMD_REMOVE_DEVICE)
#define APSME_RD_LEN   9
#define APSME_RD_EADDR 1

// APSME CMD packet fields (APSME_CMD_REQUEST_KEY)
#define APSME_RK_KEY_TYPE 1
#define APSME_RK_EADDR    2
#define APSME_RK_NWK_LEN  2
#define APSME_RK_APP_LEN  10

// APSME CMD packet fields (APSME_CMD_SWITCH_KEY)
#define APSME_SK_SEQ_NUM 1
#define APSME_SK_LEN     2

// APSME Coordinator/Trust Center NWK addresses
#define APSME_TRUSTCENTER_NWKADDR  NWK_PAN_COORD_ADDR

/******************************************************************************
 * TYPEDEFS
 */

// AIB item Ids
typedef enum
{
  apsAddressMap = 0xA0,

  // Proprietary Items
  apsMaxBindingTime,
  apsBindingTable,
  apsNumBindingTableEntries,
  apsMAX_AIB_ITEMS  // Must be the last entry
} ZApsAttributes_t;

// Type of information being queried
typedef enum
{
  NWK_ADDR_LIST,
  EXT_ADDRESS,
  SIMPLE_DESC,
  NODE_DESC,
  POWER_DESC,
  SVC_MATCH
} APSME_query_t;

#define APS_ILLEGAL_DEVICES             0x02

// Structure returned from APSME_GetRequest for apsBindingTable
typedef struct
{
  byte srcAddr[Z_EXTADDR_LEN];  // Source Addr
  byte srcEP;                   // Endpoint/interface of source device
  uint16 clusterID;             // Cluster ID
  zAddrType_t dstAddr;          // Destination address
  byte dstEP;                   // Endpoint/interface of dest device
} apsBindingItem_t;

typedef struct
{
  byte FrmCtrl;
  byte XtndFrmCtrl;
  byte DstEndPoint;
  byte SrcEndPoint;
  uint16 GroupID;
  uint16 ClusterID;
  uint16 ProfileID;
  byte wasBroadcast;
  byte apsHdrLen;
  byte *asdu;
  byte asduLength;
  byte ApsCounter;
  uint8 transID;
  uint8 BlkCount;
  uint8 AckBits;
} aps_FrameFormat_t;

// APS Data Service Primitives
typedef struct
{
  zAddrType_t dstAddr;
  uint8       srcEP;
  uint8       dstEP;
  uint16      clusterID;
  uint16      profileID;
  uint16      asduLen;
  uint8*      asdu;
  uint16      txOptions;
  uint8       transID;
  uint8       discoverRoute;
  uint8       radiusCounter;
  uint8       apsCount;
  uint8       blkCount;
} APSDE_DataReq_t;

typedef struct
{
  uint16 dstAddr;
  uint8  dstEP;
  uint8  srcEP;
  uint8  transID;
  uint8  status;
} APSDE_DataCnf_t;

typedef struct
{
  uint8 secure;
} APSDE_DataReqMTU_t;

// APS Security Related Primitives
typedef struct
{
  uint16 dstAddr;
  uint8* respExtAddr;
  uint8  method;
  uint8  secure;
} APSME_EstablishKeyReq_t;

typedef struct
{
  uint8* partExtAddr;
  uint8  status;
} APSME_EstablishKeyCfm_t;

typedef struct
{
  uint16 srcAddr;
  uint8* initExtAddr;
  uint8  method;
  uint8  secure;
} APSME_EstablishKeyInd_t;

typedef struct
{
  uint16 dstAddr;
  uint8* initExtAddr;
  uint8  accept;
  uint8  secure;
} APSME_EstablishKeyRsp_t;

typedef struct
{
  uint16 dstAddr;
  uint8  keyType;
  uint8  keySeqNum;
  uint8* key;
  uint8* extAddr;
  uint8  initiator;
  uint8  secure;
} APSME_TransportKeyReq_t;

typedef struct
{
  uint16 srcAddr;
  uint8  keyType;
  uint8  keySeqNum;
  uint8* key;
  uint8* dstExtAddr;
  uint8* srcExtAddr;
  uint8  initiator;
  uint8  secure;
} APSME_TransportKeyInd_t;

typedef struct
{
  uint16 dstAddr;
  uint16 devAddr;
  uint8* devExtAddr;
  uint8  status;
} APSME_UpdateDeviceReq_t;

typedef struct
{
  uint16 srcAddr;
  uint8* devExtAddr;
  uint16 devAddr;
  uint8  status;
} APSME_UpdateDeviceInd_t;

typedef struct
{
  uint16 parentAddr;
  uint8* childExtAddr;
} APSME_RemoveDeviceReq_t;

typedef struct
{
  uint16 srcAddr;
  uint8* childExtAddr;
} APSME_RemoveDeviceInd_t;

typedef struct
{
  uint8  dstAddr;
  uint8  keyType;
  uint8* partExtAddr;
} APSME_RequestKeyReq_t;

typedef struct
{
  uint16 srcAddr;
  uint8  keyType;
  uint8* partExtAddr;
} APSME_RequestKeyInd_t;

typedef struct
{
  uint16 dstAddr;
  uint8  keySeqNum;
} APSME_SwitchKeyReq_t;

typedef struct
{
  uint16 srcAddr;
  uint8  keySeqNum;
} APSME_SwitchKeyInd_t;

// APS Incoming Command Packet
typedef struct
{
  osal_event_hdr_t hdr;
  uint8*           asdu;
  uint8            asduLen;
  uint8            secure;
  uint16           nwkAddr;
  uint8            nwkSecure;
} APSME_CmdPkt_t;

// APS Key Data Types
//typedef struct
//{
//  uint16 di;
//  uint8* masterKey;
//} APSME_MasterKeyData_t;

typedef struct
{
  uint8* key;
  uint32 txFrmCntr;
  uint32 rxFrmCntr;
} APSME_LinkKeyData_t;

typedef struct
{
  uint8   frmCtrl;
  uint8   xtndFrmCtrl;
  uint8   srcEP;
  uint8   dstEP;
  uint16  groupID;
  uint16  clusterID;
  uint16  profileID;
  uint8   asduLen;
  uint8*  asdu;
  uint8   hdrLen;
  uint8   apsCounter;
  uint8   transID;
  uint8   blkCount;
  uint8   ackBits;
} APSDE_FrameData_t;

typedef struct
{
  uint8  frmCtrl;
  uint8  xtndFrmCtrl;
  uint8  srcEP;
  uint8  dstEP;
  uint16 clusterID;
  uint16 profileID;
  uint8  asduLen;
  uint16 dstAddr;
  uint8  transID;
} APSDE_StoredFrameData_t;

typedef struct
{
//ZMacDataReq_t     mfd;
  NLDE_FrameData_t  nfd;
  APSDE_FrameData_t afd;
} APSDE_FrameFormat_t;

typedef struct
{
  uint16 dstAddr;
  uint8  frmCtrl;
  uint8  xtndFrmCtrl;
  uint8  asduLen;
} APSDE_FrameAlloc_t;

typedef struct
{
  //input
  APSDE_FrameAlloc_t   fa;

  //output
  APSDE_FrameFormat_t* aff;
  SSP_Info_t*          si;
  uint8                status;
} APSDE_FrameBlk_t;

/******************************************************************************
 * GLOBAL VARIABLES
 */

/******************************************************************************
 * APS Data Service
 *   APSDE-DATA
 */

/*
 * This function requests the transfer of data from the next higher layer
 * to a single peer entity.
 */
extern ZStatus_t APSDE_DataReq( APSDE_DataReq_t* req );

/*
 * This function requests the MTU(Max Transport Unit) of the APS Data Service
 */
extern uint8 APSDE_DataReqMTU( APSDE_DataReqMTU_t* fields );

/*
 * This function reports the results of a request to transfer a data
 * PDU (ASDU) from a local higher layer entity to another single peer entity.
 */
extern void APSDE_DataConfirm( nwkDB_t *rec, ZStatus_t Status );
extern void APSDE_DataCnf( APSDE_DataCnf_t* cnf );

/*
 * This function indicates the transfer of a data PDU (ASDU) from the
 * APS sub-layer to the local application layer entity.
 */
extern void APSDE_DataIndication( aps_FrameFormat_t *aff, zAddrType_t *SrcAddress,
                                uint8 LinkQuality, byte SecurityUse, uint32 timestamp );

/******************************************************************************
 * APS Management Service
 *   APSME-BIND
 *   APSME-UNBIND
 */

/*
 * This function allows the next higher layer to request to bind two devices together
 * either by proxy or directly, if issued on the coordinator.
 *
 * NOTE: The APSME-BIND.confirm is returned by this function and is not a
 *       seperate callback function.
 */
extern ZStatus_t APSME_BindRequest( zAddrType_t *SrcAddr, byte SrcEndpInt,
                            uint16 ClusterId, zAddrType_t *DstAddr, byte DstEndpInt);

/*
 * This function allows the next higher layer to request to unbind two devices
 * either by proxy or directly, if issued on the coordinator.
 *
 * NOTE: The APSME-UNBIND.confirm is returned by this function and is not a
 *       seperate callback function.
 */
extern ZStatus_t APSME_UnBindRequest( zAddrType_t *SrcAddr, byte SrcEndpInt,
                            uint16 ClusterId, zAddrType_t *DstAddr, byte DstEndpInt);

/*
 * This function allows the next higher layer to read the value of an attribute
 * from the AIB (APS Information Base)
 */
extern ZStatus_t APSME_GetRequest( ZApsAttributes_t AIBAttribute,
                                    uint16 Index, byte *AttributeValue );

/*
 * This function allows the next higher layer to write the value of an attribute
 * into the AIB (APS Information Base)
 */
extern ZStatus_t APSME_SetRequest( ZApsAttributes_t AIBAttribute,
                                    uint16 Index, byte *AttributeValue );

/*
 * This function gets the EXT address based on the NWK address.
 */
extern uint8 APSME_LookupExtAddr( uint16 nwkAddr, uint8* extAddr );

/*
 * This function gets the NWK address based on the EXT address.
 */
extern uint8 APSME_LookupNwkAddr( uint8* extAddr, uint16* nwkAddr );

#if 0     // NOT IMPLEMENTED
/*
 * This function allows the next higher layer to be notified of the results of its
 * request to unbind two devices directly or by proxy.
 */
extern void APSME_UnbindConfirm( zAddrType_t CoorAddr,ZStatus_t Status,
                           uint16 SrcAddr, byte SrcEndpInt, byte ObjectId,
                           uint16 DstAddr, byte DstEndpInt);
/*
 * This function allows the next higher layer to be notified of the results of its
 * request to bind two devices directly or by proxy.
 */
extern void APSME_BindConfirm( zAddrType_t CoorAddr,ZStatus_t Status,
                           uint16 SrcAddr, byte SrcEndpInt, byte ObjectId,
                           uint16 DstAddr, byte DstEndpInt);
#endif  // NOT IMPLEMENTED

/******************************************************************************
 * APS Incoming Command Packet Handler
 */

/*
 * APSME_CmdPkt handles APS CMD packets.
 */
extern void APSME_CmdPkt( APSME_CmdPkt_t* pkt );

/******************************************************************************
 * APS Frame Allocation And Routing
 */
/*
 * APSDE_FrameAlloc allocates an APS frame.
 */
extern void APSDE_FrameAlloc( APSDE_FrameBlk_t* blk );

/*
 * APSDE_FrameSend sends an APS frame.
 */
extern void APSDE_FrameSend( APSDE_FrameBlk_t* blk );

/******************************************************************************
 * APS Security Related Functions
 */
/*
 * APSME_FrameSecurityRemove removes security from APS frame.
 */
extern ZStatus_t APSME_FrameSecurityRemove(uint16             srcAddr,
                                           aps_FrameFormat_t* aff);

/*
 * APSME_FrameSecurityApply applies security from APS frame.
 */
extern ZStatus_t APSME_FrameSecurityApply(uint16             dstAddr,
                                          aps_FrameFormat_t* aff);

/*
 * Configure APS security mode
 */
void APSME_SecurityNM( void );   // NULL MODE        - NO SECURITY
void APSME_SecurityRM_ED( void );// RESIDENTIAL MODE - END DEVICE
void APSME_SecurityRM_RD( void );// RESIDENTIAL MODE - ROUTER DEVICE
void APSME_SecurityRM_CD( void );// RESIDENTIAL MODE - COORD DEVICE
void APSME_SecurityCM_ED( void );// COMMERCIAL MODE  - END DEVICE
void APSME_SecurityCM_RD( void );// COMMERCIAL MODE  - ROUTER DEVICE
void APSME_SecurityCM_CD( void );// COMMERCIAL MODE  - COORD DEVICE

/*
 * Configure SKKE slot data
 */
void APSME_SKKE_SlotInit( uint8 total );

/******************************************************************************
 * APS Security Service Primitives - API, NHLE Calls Routines
 *
 *   APSME_EstablishKeyReq
 *   APSME_EstablishKeyRsp
 *   APSME_TransportKeyReq
 *   APSME_UpdateDeviceReq
 *   APSME_RemoveDeviceReq
 *   APSME_RequestKeyReq
 *   APSME_SwitchKeyReq
 */
/*
 * APSME_EstablishKeyReq primitive.
 */
extern ZStatus_t APSME_EstablishKeyReq( APSME_EstablishKeyReq_t* req );

/*
 * APSME_EstablishKeyRsp primitive.
 */
extern ZStatus_t APSME_EstablishKeyRsp( APSME_EstablishKeyRsp_t* rsp );

/*
 * APSME_TransportKeyReq primitive.
 */
extern ZStatus_t APSME_TransportKeyReq( APSME_TransportKeyReq_t* req );

/*
 * APSME_UpdateDeviceReq primitive.
 */
extern ZStatus_t APSME_UpdateDeviceReq( APSME_UpdateDeviceReq_t* req );

/*
 * APSME_RemoveDeviceReq primitive.
 */
extern ZStatus_t APSME_RemoveDeviceReq( APSME_RemoveDeviceReq_t* req );

/*
 * APSME_RequestKeyReq primitive.
 */
extern ZStatus_t APSME_RequestKeyReq( APSME_RequestKeyReq_t* req );

/*
 * APSME_SwitchKeyReq primitive.
 */
extern ZStatus_t APSME_SwitchKeyReq( APSME_SwitchKeyReq_t* req );

/******************************************************************************
 * APS Security Primitive Stubs - API, NHLE Implements Callback Stubs
 *
 *   APSME_EstablishKeyCfm
 *   APSME_EstablishKeyInd
 *   APSME_TransportKeyInd
 *   APSME_UpdateDeviceInd
 *   APSME_RemoveDeviceInd
 *   APSME_RequestKeyInd
 *   APSME_SwitchKeyInd
 */

/*
 * APSME_EstablishKeyCfm primitive.
 */
extern void APSME_EstablishKeyCfm( APSME_EstablishKeyCfm_t* cfm );

/*
 * APSME_EstablishKeyInd primitive.
 */
extern void APSME_EstablishKeyInd( APSME_EstablishKeyInd_t* ind );

/*
 * APSME_TransportKeyInd primitive.
 */
extern void APSME_TransportKeyInd( APSME_TransportKeyInd_t* ind );

/*
 * APSME_UpdateDeviceInd primitive.
 */
extern void APSME_UpdateDeviceInd( APSME_UpdateDeviceInd_t* ind );

/*
 * APSME_RemoveDeviceInd primitive.
 */
extern void APSME_RemoveDeviceInd( APSME_RemoveDeviceInd_t* ind );

/*
 * APSME_RequestKeyInd primitive.
 */
extern void APSME_RequestKeyInd( APSME_RequestKeyInd_t* ind );

/*
 * APSME_SwitchKeyInd primitive.
 */
extern void APSME_SwitchKeyInd( APSME_SwitchKeyInd_t* ind );

/******************************************************************************
 * APS Security Support - NHLE Implements Callback Stubs
 *
 *   APSME_MasterKeyGet
 *   APSME_LinkKeySet
 *   APSME_LinkKeyDataGet
 *   APSME_KeyFwdToChild
 */

/*
 * APSME_MasterKeyExtGet stub.
 */
extern ZStatus_t APSME_MasterKeyGet( uint8*  extAddr, uint8** key );

/*
 * APSME_LinkKeyDataGet stub.
 */
extern ZStatus_t APSME_LinkKeySet( uint8* extAddr, uint8* key );

/*
 * APSME_LinkKeyDataGet stub.
 */
extern ZStatus_t APSME_LinkKeyDataGet( uint16                nwkAddr,
                                       APSME_LinkKeyData_t** data );

/*
 * APSME_FwdKeyToChild stub.
 */
extern uint8 APSME_KeyFwdToChild( APSME_TransportKeyInd_t* ind );

/******************************************************************************
******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* NLMEDE_H */


