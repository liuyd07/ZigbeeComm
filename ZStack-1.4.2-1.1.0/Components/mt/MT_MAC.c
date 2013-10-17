/*********************************************************************
    Filename:       MT_MAC.c
    Revised:        $Date: 2007-02-26 12:07:41 -0800 (Mon, 26 Feb 2007) $
    Revision:       $Revision: 13608 $

    Description:

        MonitorTest functions for the MAC layer.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/
#if defined ( MT_MAC_FUNC )             //MAC commands
/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "MTEL.h"
#include "ZMAC.h"
#include "MT_MAC.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* Hal */
#include "hal_uart.h"
#include "SPIMgr.h"

/*********************************************************************
 * MACROS
 */

/* The length in bytes of the pending address fields in the beacon */
#define MT_MAC_PEND_LEN(pendAddrSpec)   ((((pendAddrSpec) & 0x07) * 2) + \
                                        ((((pendAddrSpec) & 0x70) >> 4) * 8))

/* This matches the value used by nwk */
#define MT_MAC_ED_SCAN_MAXCHANNELS      27

/* Maximum size of pending address spec in beacon notify ind */
#define MT_MAC_PEND_LEN_MAX             32

/* Maximum size of the payload SDU in beacon notify ind */
#define MT_MAC_SDU_LEN_MAX              32

/* Maximum length of scan result in bytes */
#define MT_MAC_SCAN_RESULT_LEN_MAX      32

/* Maximum size of beacon payload */
#define MT_MAC_BEACON_PAYLOAD_MAX       16

/*********************************************************************
 * CONSTANTS
 */
#define DEFAULT_NSDU_HANDLE             0x00

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint16 _macCallbackSub;

union
{
  ZMacStartReq_t        startReq;
  ZMacScanReq_t         scanReq;
  ZMacDataReq_t         dataReq;
  ZMacAssociateReq_t    assocReq;
  ZMacAssociateRsp_t    assocRsp;
  ZMacDisassociateReq_t disassocReq;
  ZMacOrphanRsp_t       orphanRsp;
  ZMacRxEnableReq_t     rxEnableReq;
  ZMacSyncReq_t         syncReq;
  ZMacPollReq_t         pollReq;
} mtMac;

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

/* storage for MAC beacon payload */
static uint8 mtMacBeaconPayload[MT_MAC_BEACON_PAYLOAD_MAX];

/*********************************************************************
 * @fn      MT_MacRevExtCpy
 *
 * @brief
 *
 *   Reverse-copy an extended address.
 *
 * @param   pDst - Pointer to data destination
 * @param   pSrc - Pointer to data source
 *
 * @return  void
 */
static void MT_MacRevExtCpy( uint8 *pDst, uint8 *pSrc )
{
  int8 i;

  for ( i = Z_EXTADDR_LEN - 1; i >= 0; i-- )
  {
    *pDst++ = pSrc[i];
  }
}

/*********************************************************************
 * @fn      MT_MacSpi2Addr
 *
 * @brief
 *
 *   Copy an address from an SPI message to an address struct.  The
 *   addrMode in pAddr must already be set.
 *
 * @param   pDst - Pointer to address struct
 * @param   pSrc - Pointer SPI message byte array
 *
 * @return  void
 *********************************************************************/
static void MT_MacSpi2Addr( zAddrType_t *pDst, uint8 *pSrc )
{
  if ( pDst->addrMode == Addr16Bit )
  {
    pDst->addr.shortAddr = BUILD_UINT16( pSrc[7] , pSrc[6] );
  }
  else if ( pDst->addrMode == Addr64Bit )
  {
    MT_MacRevExtCpy( pDst->addr.extAddr, pSrc );
  }
}

/*********************************************************************
 * @fn      MT_MacSpi2Sec
 *
 * @brief
 *
 *   Copy Security information from SPI message to a Sec structure
 *
 * @param   pSec - Pointer to security struct
 * @param   pSrc - Pointer SPI message byte array
 *
 * @return  void
 *********************************************************************/
static void MT_MacSpi2Sec( ZMacSec_t *pSec, uint8 *pSrc )
{
  /* Right now, set everything to zero */
  osal_memset (pSec, 0, sizeof (ZMacSec_t));
}

/*********************************************************************
 * @fn      MT_MacAddr2Spi
 *
 * @brief
 *
 *   Copy an address from an address struct to an SPI message.
 *
 * @param   pDst - Pointer SPI message byte array
 * @param   pSrc - Pointer to address struct
 *
 * @return  void
 *********************************************************************/
static void MT_MacAddr2Spi( uint8 *pDst, zAddrType_t *pSrc )
{
  if ( pSrc->addrMode == Addr16Bit )
  {
    *pDst++ = 0; *pDst++ = 0; *pDst++ = 0;
    *pDst++ = 0; *pDst++ = 0; *pDst++ = 0;
    *pDst++ = HI_UINT16( pSrc->addr.shortAddr );
    *pDst = LO_UINT16( pSrc->addr.shortAddr );
  }
  else if ( pSrc->addrMode == Addr64Bit )
  {
    MT_MacRevExtCpy( pDst, pSrc->addr.extAddr );
  }
}


/*********************************************************************
 * @fn      MT_MacMsgAllocate
 *
 * @brief
 *
 *   Allocate an MT MAC SPI message buffer.
 *
 * @param   cmd - SPI command id
 * @param   len - length of SPI command parameters
 *
 * @return  pointer to allocated message
 *********************************************************************/
static mtOSALSerialData_t *MT_MacMsgAllocate( uint16 cmd, uint8 len )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = (mtOSALSerialData_t *) osal_msg_allocate( len +
                                                     sizeof(mtOSALSerialData_t) +
                                                     SPI_0DATA_MSG_LEN );
  if ( msgPtr )
  {
    msgPtr->hdr.event = CB_FUNC;
    msg = msgPtr->msg = (uint8 *) ( msgPtr + 1 );
    *msg++ = SOP_VALUE;
    *msg++ = HI_UINT16( cmd );
    *msg++ = LO_UINT16( cmd );
    *msg   = len;
  }
  return msgPtr;
}

/*********************************************************************
 * @fn      MT_MacCommandProcessing
 *
 * @brief
 *
 *   Process all the MAC commands that are issued by test tool
 *
 * @param   cmd_id - Command ID
 * @param   len    - Length of received SPI pData message
 * @param   pData  - pointer to received SPI pData message
 *
 * @return  void
 */
void MT_MacCommandProcessing( uint16 cmd_id , byte len , byte *pData )
{
  byte *msg_ptr;
  ZMacStatus_t ret;
  byte attr;

  ret = ZMacSuccess;

  switch ( cmd_id )
  {
    case SPI_CMD_MAC_INIT:
      ret = ZMacInit();

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_INIT, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_START_REQ:
#ifdef RTR_NWK

      /* StartTime */
      mtMac.startReq.StartTime = BUILD_UINT32 (pData[3], pData[2], pData[1], pData[0]);
      pData += 4;

      /* PanID */
      mtMac.startReq.PANID = BUILD_UINT16( pData[1] , pData[0] );
      pData += 2;

      /* Fill in other fields sequentially incrementing the pointer*/
      mtMac.startReq.LogicalChannel    =  *pData++;
      mtMac.startReq.ChannelPage       =  *pData++;
      mtMac.startReq.BeaconOrder       =  *pData++;
      mtMac.startReq.SuperframeOrder   =  *pData++;
      mtMac.startReq.PANCoordinator    =  *pData++;
      mtMac.startReq.BatteryLifeExt    =  *pData++;
      mtMac.startReq.CoordRealignment  =  *pData++;

      /* Realign Security Information */
      MT_MacSpi2Sec( &mtMac.startReq.RealignSec, pData );
      pData += ZTEST_DEFAULT_SEC_LEN;

      /* Beacon Security Information */
      MT_MacSpi2Sec( &mtMac.startReq.BeaconSec, pData );

      /* Call corresponding ZMAC function */
      ret = ZMacStartReq( &mtMac.startReq );
#else
      ret = ZMacDenied;
#endif

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_START_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_SCAN_REQ:

      /* ScanChannels is the 32-bit channel list */
      mtMac.scanReq.ScanChannels = BUILD_UINT32 (pData[3], pData[2], pData[1], pData[0]);
      pData += 4;

      /* Fill in fields sequentially incrementing the pointer */
      mtMac.scanReq.ScanType = *pData++;

      /* ScanDuration */
      mtMac.scanReq.ScanDuration = *pData++;

      /* Channel Page */
      mtMac.scanReq.ChannelPage = *pData++;

      /* MaxResults */
      mtMac.scanReq.MaxResults = *pData++;

      /* Security Information */
      MT_MacSpi2Sec( &mtMac.scanReq.Sec, pData );

      /* Call corresponding ZMAC function */
      ret =  ZMacScanReq( &mtMac.scanReq );

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_SCAN_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);

      break;

    case SPI_CMD_MAC_DATA_REQ:

      /* Destination address mode */
      mtMac.dataReq.DstAddr.addrMode = *pData++;

      /* Destination address */
      MT_MacSpi2Addr( &mtMac.dataReq.DstAddr, pData);
      pData += Z_EXTADDR_LEN;

      /* Destination Pan ID */
      mtMac.dataReq.DstPANId = BUILD_UINT16( pData[1] , pData[0] );
      pData += 2;

      /* Source address mode */
      mtMac.dataReq.SrcAddrMode = *pData++;

      /* Handle */
      mtMac.dataReq.Handle = *pData++;

      /* TxOptions */
      mtMac.dataReq.TxOptions = *pData++;

      /* Channel */
      mtMac.dataReq.Channel = *pData++;

      /* Power */
      mtMac.dataReq.Power = *pData++;

      /* Security Information */
      MT_MacSpi2Sec( &mtMac.dataReq.Sec, pData );
      pData += ZTEST_DEFAULT_SEC_LEN;

      /* Data length */
      mtMac.dataReq.msduLength = *pData++;

      /* Data - Just pass the pointer to the structure */
      mtMac.dataReq.msdu = pData;

      /* Call corresponding ZMAC function */
      ret = ZMacDataReq( &mtMac.dataReq );

      /* Send response back */
      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_DATA_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_ASSOCIATE_REQ:

      /* Logical Channel */
      mtMac.assocReq.LogicalChannel = *pData++;

      /* Channel Page */
      mtMac.assocReq.ChannelPage = *pData++;

      /* Address Mode */
      mtMac.assocReq.CoordAddress.addrMode = *pData++;

      /* Coordinator Address, address mode must be set at this point */
      MT_MacSpi2Addr( &mtMac.assocReq.CoordAddress, pData );
      pData += Z_EXTADDR_LEN;

      /* Coordinator PanID */
      mtMac.assocReq.CoordPANId = BUILD_UINT16( pData[1] , pData[0] );
      pData += 2;

      /* Capability information */
      mtMac.assocReq.CapabilityInformation = *pData++;

      /* Security Information */
      MT_MacSpi2Sec( &mtMac.assocReq.Sec, pData );

      /* Call corresponding ZMAC function */
      ret = ZMacAssociateReq( &mtMac.assocReq );

      /* Send response back */
      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_ASSOCIATE_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_ASSOCIATE_RSP:
#ifdef RTR_NWK

      /* Device extended address */
      MT_MacRevExtCpy( mtMac.assocRsp.DeviceAddress, pData );
      pData += Z_EXTADDR_LEN;

      /* Associated short address */
      mtMac.assocRsp.AssocShortAddress = BUILD_UINT16(pData[1],pData[0]);
      pData += 2;

      /* Status */
      mtMac.assocRsp.Status = *pData++;

      /* Security Information */
      MT_MacSpi2Sec( &mtMac.assocRsp.Sec, pData );

      /* Call corresponding ZMAC function */
      ret = ZMacAssociateRsp( &mtMac.assocRsp );
#else
      ret = ZMacDenied;
#endif

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_ASSOCIATE_RSP, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_DISASSOCIATE_REQ:

      /* Device address mode */
      mtMac.disassocReq.DeviceAddress.addrMode = *pData++;

      /* Device address - Device address mode have to be set to use this function*/
      MT_MacSpi2Addr( &mtMac.disassocReq.DeviceAddress, pData);
      pData += Z_EXTADDR_LEN;

      /* Pan ID */
      mtMac.disassocReq.DevicePanId = BUILD_UINT16( pData[1] , pData[0] );
      pData += 2;

      /* Disassociate reason */
      mtMac.disassocReq.DisassociateReason = *pData++;

      /* TxIndirect */
      mtMac.disassocReq.TxIndirect = *pData++;

      /* Security Information */
      MT_MacSpi2Sec( &mtMac.disassocReq.Sec, pData );

      /* Call corresponding ZMAC function */
      ret = ZMacDisassociateReq( &mtMac.disassocReq );

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_DISASSOCIATE_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_GET_REQ:
      attr = *pData;
      len = SPI_0DATA_MSG_LEN + ZTEST_DEFAULT_PARAM_LEN;

      msg_ptr = osal_mem_alloc( len );
      if ( msg_ptr )
      {
        /* initialize the buffer */
        osal_memset( msg_ptr, 0, len );

        /* Call mac_get_req with attr and pointer to first data position*/
        ret = ZMacGetReq( attr, msg_ptr + DATA_BEGIN );

        /*
           Build SPI message here instead of redundantly calling MT_BuildSPIMsg
           because we have copied data already in the allocated message
        */

        *msg_ptr = SOP_VALUE;
        msg_ptr[CMD_FIELD_HI] =
          (byte)(HI_UINT16( ( uint16 )( SPI_RESPONSE_BIT | SPI_CMD_MAC_GET_REQ )));

        msg_ptr[CMD_FIELD_LO] =
          (byte)(LO_UINT16( ( SPI_RESPONSE_BIT | SPI_CMD_MAC_GET_REQ )));

        msg_ptr[DATALEN_FIELD] = len - SPI_0DATA_MSG_LEN;

        /*
          FCS goes to the last byte in the message and is calculated
          over all the bytes except FCS and SOP
        */
        msg_ptr[len - 1] = SPIMgr_CalcFCS( msg_ptr + 1, (byte)(len-2) );

#if (defined SPI_MGR_DEFAULT_PORT)
        HalUARTWrite ( SPI_MGR_DEFAULT_PORT, msg_ptr, len );
#endif
        osal_mem_free( msg_ptr );
      }
      break;

    case SPI_CMD_MAC_SET_REQ:
      /*
        In the data field of 'msg', the first byte is the attribute and remainder
        is the attribute value. So the pointer 'pData' points directly to the attribute.
        The value of the attribute is from the next byte position
      */
      attr = *pData;

      /* special case for beacon payload */
      if ( attr == ZMacBeaconMSDU )
      {
        osal_memcpy( mtMacBeaconPayload, pData + 1, MT_MAC_BEACON_PAYLOAD_MAX );
        ret = ZMacSetReq( (ZMacAttributes_t)attr ,  (byte *) &mtMacBeaconPayload );
      }
      else
      {
        ret = ZMacSetReq( (ZMacAttributes_t)attr , pData + 1 );
      }
      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_SET_REQ, len, SPI_RESP_LEN_MAC_DEFAULT );
      break;

    case SPI_CMD_MAC_RESET_REQ:

      /*
        The only data byte in the message has a value of 0/1 indicating
        to set the PIB attributes to default values or to leave as is, after reset.
      */

      ret = ZMacReset( *pData );

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_RESET_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_GTS_REQ:
      /* Not supported */
      break;

    case SPI_CMD_MAC_ORPHAN_RSP:

#if defined( USING_FFDCMAC )
      /* Extended address of the device sending the notification */
      MT_MacRevExtCpy( mtMac.orphanRsp.OrphanAddress, pData );
      pData += Z_EXTADDR_LEN;

      /* Short address of the orphan device */
      mtMac.orphanRsp.ShortAddress = BUILD_UINT16( pData[1] , pData[0] );
      pData += 2;

      /* Associated member */
      mtMac.orphanRsp.AssociatedMember = *pData++;

      /* Security Information */
      MT_MacSpi2Sec( &mtMac.orphanRsp.Sec, pData );

      /* Call corresponding ZMAC function */
      ret = ZMacOrphanRsp( &mtMac.orphanRsp );
#else
      ret = ZMacUnsupportedAttribute;
#endif // USING_FFDCMAC

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_ORPHAN_RSP, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_RX_ENABLE_REQ:

      /* First byte - DeferPermit*/
      mtMac.rxEnableReq.DeferPermit = *pData++;

      /* RxOnTime */
      mtMac.rxEnableReq.RxOnTime = BUILD_UINT32 (pData[3], pData[2], pData[1], pData[0]);
      pData += 4;

      /* RxOnDuration */
      mtMac.rxEnableReq.RxOnDuration = BUILD_UINT32 (pData[3], pData[2], pData[1], pData[0]);

      /* Call corresponding ZMAC function */
      ret = ZMacRxEnableReq ( &mtMac.rxEnableReq );

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_RX_ENABLE_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_SYNC_REQ:

      /* LogicalChannel */
      mtMac.syncReq.LogicalChannel = *pData++;

      /* ChannelPage */
      mtMac.syncReq.ChannelPage = *pData++;

      /* TrackBeacon */
      mtMac.syncReq.TrackBeacon    = *pData;

      /* Call corresponding ZMAC function */
      ret = ZMacSyncReq( &mtMac.syncReq );

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_SYNC_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_POLL_REQ:

      /* Coordinator address mode */
      mtMac.pollReq.CoordAddress.addrMode = *pData++;

      /* Coordinator address - Device address mode have to be set to use this function */
      MT_MacSpi2Addr( &mtMac.pollReq.CoordAddress, pData);
      pData += Z_EXTADDR_LEN;

      /* Coordinator Pan ID */
      mtMac.pollReq.CoordPanId = BUILD_UINT16( pData[1] , pData[0] );
      pData += 2;

      /* Security Information */
      MT_MacSpi2Sec( &mtMac.pollReq.Sec, pData );

      /* Call corresponding ZMAC function */
      ret = ZMacPollReq( &mtMac.pollReq );

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_POLL_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;

    case SPI_CMD_MAC_PURGE_REQ:
      /* First and only byte - MsduHandle */
      ret = ZMacPurgeReq (*pData);

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_MAC_DEFAULT;
      MT_SendSPIRespMsg( (byte)ret, SPI_CMD_MAC_PURGE_REQ, len, SPI_RESP_LEN_MAC_DEFAULT);
      break;
  }
}

#if defined ( MT_MAC_CB_FUNC )
/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkStartCnf
 *
 * @brief       Process the callback subscription for nwk_start_cnf
 *
 * @param       byte Status
 *
 * @return      None
 */
void nwk_MTCallbackSubNwkStartCnf( byte Status )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_START_CNF, SPI_RESP_LEN_MAC_DEFAULT );
  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Status */
    *msg = Status;

    osal_msg_send( MT_TaskID, (byte *) msgPtr );
  }
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkDataCnf
 *
 * @brief       Process the callback subscription for nwk_data_cnf
 *
 * @param       pointer of type macnwk_data_cnf_t
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkDataCnf( ZMacDataCnf_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_DATA_CNF, SPI_LEN_NWK_DATA_CNF );
  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Status */
    *msg++ = param->hdr.Status;

    /* Handle */
    *msg++ = param->msduHandle;

    /* Timestamp */
    *msg++ = BREAK_UINT32( param->Timestamp, 3 );
    *msg++ = BREAK_UINT32( param->Timestamp, 2 );
    *msg++ = BREAK_UINT32( param->Timestamp, 1 );
    *msg++ = BREAK_UINT32( param->Timestamp, 0 );

    /* Timestamp2 */
    *msg++ = HI_UINT16( param->Timestamp2);
    *msg = LO_UINT16( param->Timestamp2);

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkDataInd
 *
 * @brief       Process the callback subscription for nwk_data_ind
 *
 * @param       pointer of type macnwk_data_ind_t
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkDataInd( ZMacDataInd_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;
  byte                len;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_DATA_IND, SPI_LEN_NWK_DATA_IND + ZTEST_DEFAULT_DATA_LEN );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Src address mode */
    *msg++ = param->SrcAddr.addrMode;

    /* Src Address */
    MT_MacAddr2Spi( msg, &param->SrcAddr );
    msg += Z_EXTADDR_LEN;

    /* Dst address mode */
    *msg++ = param->DstAddr.addrMode;

    /* Dst address */
    MT_MacAddr2Spi( msg, &param->DstAddr );
    msg += Z_EXTADDR_LEN;

       /* Timestamp */
    *msg++ = BREAK_UINT32( param->Timestamp, 3 );
    *msg++ = BREAK_UINT32( param->Timestamp, 2 );
    *msg++ = BREAK_UINT32( param->Timestamp, 1 );
    *msg++ = BREAK_UINT32( param->Timestamp, 0 );

    /* Timestamp2 */
    *msg++ = HI_UINT16( param->Timestamp2);
    *msg++ = LO_UINT16( param->Timestamp2);

    /* Src Pan Id */
    *msg++ = HI_UINT16( param->SrcPANId );
    *msg++ = LO_UINT16( param->SrcPANId );

    /* Dst Pan Id */
    *msg++ = HI_UINT16( param->DstPANId );
    *msg++ = LO_UINT16( param->DstPANId );

    /* mpdu Link Quality */
    *msg++ = param->mpduLinkQuality;

    /* LQI */
    *msg++ = param->Correlation;

    /* RSSI */
    *msg++ = param->Rssi;

    /* DSN */
    *msg++ = param->Dsn;

    /* Security */
    MT_MacSpi2Sec ((ZMacSec_t *)msg, (uint8 *)&param->Sec);
    msg += ZTEST_DEFAULT_SEC_LEN;

    /* Copy the data portion - For now, the data must be a fixed length */
    if ( param->msduLength < ZTEST_DEFAULT_DATA_LEN )
      len = param->msduLength;
    else
      len = ZTEST_DEFAULT_DATA_LEN;

    *msg++ = len;
    osal_memset( msg, 0, ZTEST_DEFAULT_DATA_LEN );
    osal_memcpy( msg, param->msdu, len );

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkAssociateCnf
 *
 * @brief       Process the callback subscription for nwk_associate_cnf
 *
 * @param       pointer of type macnwk_associate_cnf_t
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkAssociateCnf( ZMacAssociateCnf_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_ASSOCIATE_CNF, SPI_LEN_NWK_ASSOCIATE_CNF );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Status */
    *msg++ = param->hdr.Status;

    /* Short address */
    *msg++ = HI_UINT16( param->AssocShortAddress );
    *msg++ = LO_UINT16( param->AssocShortAddress );

    /* Security */
    MT_MacSpi2Sec ((ZMacSec_t *)msg, (uint8 *)&param->Sec);

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}


/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkAssociateInd
 *
 * @brief       Process the callback subscription for nwk_associate_ind
 *
 * @param       pointer of type macnwk_associate_ind_t
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 *********************************************************************/
byte nwk_MTCallbackSubNwkAssociateInd( ZMacAssociateInd_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_ASSOCIATE_IND, SPI_LEN_NWK_ASSOCIATE_IND );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Extended address */
    MT_MacRevExtCpy( msg, param->DeviceAddress );
    msg += Z_EXTADDR_LEN;

    /* Capability Information */
    *msg++ = param->CapabilityInformation;

    /* Security */
    MT_MacSpi2Sec ((ZMacSec_t *)msg, (uint8 *)&param->Sec);

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}



/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkDisassociateInd
 *
 * @brief       Process the callback subscription for nwk_disassociate_ind
 *
 * @param       pointer of type macnwk_disassociate_ind_t
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 *********************************************************************/
byte nwk_MTCallbackSubNwkDisassociateInd( ZMacDisassociateInd_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_DISASSOCIATE_IND, SPI_LEN_NWK_DISASSOCIATE_IND );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Extended address */
    MT_MacRevExtCpy( msg, param->DeviceAddress );
    msg += Z_EXTADDR_LEN;

    /* Disassociate Reason */
    *msg++ = param->DisassociateReason;

    /* Security */
    MT_MacSpi2Sec ((ZMacSec_t *)msg, (uint8 *)&param->Sec);

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
    }
    else
      return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkDisassociateCnf
 *
 * @brief       Process the callback subscription for nwk_disassociate_cnf
 *
 * @param       param
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 *********************************************************************/
byte nwk_MTCallbackSubNwkDisassociateCnf( ZMacDisassociateCnf_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_DISASSOCIATE_CNF, SPI_LEN_NWK_DISASSOCIATE_CNF );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Status */
    *msg++ = param->hdr.Status;

    /* DeviceAddress */
    *msg++ = param->DeviceAddress.addrMode;

    /* Copy Address */
    MT_MacAddr2Spi( msg, &param->DeviceAddress );
    msg += Z_EXTADDR_LEN;

    /* Pan ID */
    *msg++ = HI_UINT16( param->panID );
    *msg = LO_UINT16( param->panID );

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkOrphanInd
 *
 * @brief       Process the callback subscription for nwk_orphan_ind
 *
 * @param       pointer of type macnwk_orphan_ind_t
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 *********************************************************************/
byte nwk_MTCallbackSubNwkOrphanInd( ZMacOrphanInd_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_ORPHAN_IND, SPI_LEN_NWK_ORPHAN_IND );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Extended address */
    MT_MacRevExtCpy( msg, param->OrphanAddress );
    msg += Z_EXTADDR_LEN;

    /* Security */
    MT_MacSpi2Sec ((ZMacSec_t *)msg, (uint8 *)&param->Sec);

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkScanCnf
 *
 * @brief       Process the callback subscription for nwk_scan_cnf
 *
 * @param       pointer of type macnwk_scan_cnf_t
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkScanCnf( ZMacScanCnf_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;
  byte                resultLen;

  if ( param->ScanType == ZMAC_ED_SCAN )
    resultLen = MT_MAC_ED_SCAN_MAXCHANNELS;
  else if ( param->ScanType == ZMAC_ACTIVE_SCAN )
    resultLen = (param->ResultListSize * sizeof( ZMacPanDesc_t ));
  else if ( param->ScanType == ZMAC_PASSIVE_SCAN )
    resultLen = (param->ResultListSize * sizeof( ZMacPanDesc_t ));
  else if ( param->ScanType == ZMAC_ORPHAN_SCAN )
    resultLen = 0;
  else
    return INVALID_EVENT_ID;

  resultLen = MIN(resultLen, MT_MAC_SCAN_RESULT_LEN_MAX);

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_SCAN_CNF, SPI_LEN_NWK_SCAN_CNF + MT_MAC_SCAN_RESULT_LEN_MAX );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Status */
    *msg++ = param->hdr.Status;

    /* ED max energy parameter no longer used */
    *msg++ = 0;

    /* Scan type */
    *msg++ = param->ScanType;

    /* Channel page */
    *msg++ = param->ChannelPage;

    /* Unscanned channel list */
    *msg++ = BREAK_UINT32( param->UnscannedChannels, 3 );
    *msg++ = BREAK_UINT32( param->UnscannedChannels, 2 );
    *msg++ = BREAK_UINT32( param->UnscannedChannels, 1 );
    *msg++ = BREAK_UINT32( param->UnscannedChannels, 0 );

    /* Result count */
    *msg++ = param->ResultListSize;

    /* PAN descriptor information */
    osal_memcpy( msg, param->Result.pPanDescriptor, resultLen );

    /* clear extra buffer space at end, if any */
    osal_memset( msg, 0, (MT_MAC_SCAN_RESULT_LEN_MAX - resultLen));

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkPollCnf
 *
 * @brief       Process the callback subscription for nwk_poll_cnf
 *
 * @param       byte Status
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkPollCnf( byte Status )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_POLL_CNF, SPI_RESP_LEN_MAC_DEFAULT );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    //The only data byte is Status
    *msg = Status;

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkSyncLossInd
 *
 * @brief       Process the callback subscription for nwk_sync_loss_ind
 *
 * @param       byte LossReason
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkSyncLossInd( ZMacSyncLossInd_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_SYNC_LOSS_IND, SPI_LEN_NWK_SYNC_LOSS_IND );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /*  Status - loss reason */
    *msg++ = param->hdr.Status;

    /* Pan Id */
    *msg++ = HI_UINT16( param->PANId );
    *msg++ = LO_UINT16( param->PANId );

    /* Logical Channel */
    *msg++ = param->LogicalChannel;

    /* Channel Page */
    *msg++ = param->ChannelPage;

    /* Security */
    MT_MacSpi2Sec ((ZMacSec_t *)msg, (uint8 *)&param->Sec);

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkRxEnableCnf
 *
 * @brief       Process the callback subscription for nwk_Rx_Enable_cnf
 *
 * @param
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkRxEnableCnf ( byte Status )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_RX_ENABLE_CNF, SPI_RESP_LEN_MAC_DEFAULT );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* The only data byte is Status */
    *msg = Status;

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubCommStatusInd
 *
 * @brief       Process the callback subscription for
 *              comm_status_ind.
 *
 * @param
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubCommStatusInd ( ZMacCommStatusInd_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_COMM_STATUS_IND, SPI_LEN_NWK_COMM_STATUS_IND );

  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Status */
    *msg++ = param->hdr.Status;

    /* Source address */
    *msg++ = param->SrcAddress.addrMode;
    MT_MacAddr2Spi( msg, &param->SrcAddress );
    msg += Z_EXTADDR_LEN;

    /* Destination address */
    *msg++ = param->DstAddress.addrMode;
    MT_MacAddr2Spi( msg, &param->DstAddress );
    msg += Z_EXTADDR_LEN;

    /* PAN ID */
    *msg++ = HI_UINT16( param->PANId );
    *msg++ = LO_UINT16( param->PANId );

    /* Reason */
    *msg++ = param->Reason;

    /* Security */
    MT_MacSpi2Sec ((ZMacSec_t *)msg, (uint8 *)&param->Sec);

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkPurgeCnf
 *
 * @brief       Process the callback subscription for nwk_purge_cnf
 *
 * @param       pointer of type ZMacPurgeCnf_t
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkPurgeCnf( ZMacPurgeCnf_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_PURGE_CNF, SPI_LEN_NWK_PURGE_CNF );
  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* Status */
    *msg = param->hdr.Status;

    /* Handle */
    *msg++ = param->msduHandle;

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubNwkBeaconNotifyInd
 *
 * @brief       Process the callback subscription for
 *              beacon_notify_ind.
 *
 * @param
 *
 * @return      SUCCESS if message sent succesfully , else N_FAIL
 */
byte nwk_MTCallbackSubNwkBeaconNotifyInd ( ZMacBeaconNotifyInd_t *param )
{
  mtOSALSerialData_t  *msgPtr;
  byte                *msg;

  msgPtr = MT_MacMsgAllocate( SPI_CB_NWK_BEACON_NOTIFY_IND, SPI_LEN_NWK_BEACON_NOTIFY_IND );
  if ( msgPtr )
  {
    msg = msgPtr->msg + DATA_BEGIN;

    /* BSN */
    *msg++ = param->BSN;

    /* Timestamp */
    *msg++ = BREAK_UINT32( param->pPanDesc->TimeStamp, 3 );
    *msg++ = BREAK_UINT32( param->pPanDesc->TimeStamp, 2 );
    *msg++ = BREAK_UINT32( param->pPanDesc->TimeStamp, 1 );
    *msg++ = BREAK_UINT32( param->pPanDesc->TimeStamp, 0 );

    /* Coordinator address mode */
    *msg++ = param->pPanDesc->CoordAddress.addrMode;

    /* Coordinator address */
    MT_MacAddr2Spi( msg, &param->pPanDesc->CoordAddress );
    msg += Z_EXTADDR_LEN;

    /* PAN ID */
    *msg++ = HI_UINT16( param->pPanDesc->CoordPANId );
    *msg++ = LO_UINT16( param->pPanDesc->CoordPANId );

    /* Superframe spec */
    *msg++ = HI_UINT16( param->pPanDesc->SuperframeSpec );
    *msg++ = LO_UINT16( param->pPanDesc->SuperframeSpec );

    /* LogicalChannel */
    *msg++ = param->pPanDesc->LogicalChannel;

    /* GTSPermit */
    *msg++ = param->pPanDesc->GTSPermit;

    /* LinkQuality */
    *msg++ = param->pPanDesc->LinkQuality;

    /* SecurityFailure */
    *msg++ = param->pPanDesc->SecurityFailure;

    /* Security */
    MT_MacSpi2Sec ((ZMacSec_t *)msg, (uint8 *)&param->pPanDesc->Sec);
    msg += ZTEST_DEFAULT_SEC_LEN;

    /* PendingAddrSpec */
    *msg++ = param->PendAddrSpec;

    /* AddrList */
    osal_memset( msg, 0, MT_MAC_PEND_LEN_MAX );
    osal_memcpy( msg, param->AddrList, MIN(MT_MAC_PEND_LEN_MAX, MT_MAC_PEND_LEN(param->PendAddrSpec)) );
    msg += MT_MAC_PEND_LEN_MAX;

    /* SDULength */
    *msg++ = param->sduLength;

    /* SDU */
    osal_memset( msg, 0, MT_MAC_SDU_LEN_MAX );
    osal_memcpy( msg, param->sdu, MIN(MT_MAC_SDU_LEN_MAX, param->sduLength) );
    //msg += MT_MAC_SDU_LEN_MAX;

    if ( osal_msg_send( MT_TaskID, (byte *) msgPtr ) == ZSUCCESS )
      return ZMacSuccess;
    else
      return INVALID_TASK;
  }
  else
    return INVALID_TASK;
}

#endif // MT_MAC_CB_FUNC

/*********************************************************************
*********************************************************************/
#endif // MT_MAC_FUNC
