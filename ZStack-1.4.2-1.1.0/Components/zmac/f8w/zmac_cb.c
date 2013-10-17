/********************************************************************************************************
  Filename:       zmac_cb.c
  Revised:        $Date: 2007-02-22 16:28:22 -0800 (Thu, 22 Feb 2007) $
  Revision:       $Revision: 13562 $

  Description:

  This file contains the NWK functions that the ZMAC calls

  Notes:

  Copyright (c) 2006 by Texas Instruments, Inc.
  All Rights Reserved.  Permission to use, reproduce, copy, prepare
  derivative works, modify, distribute, perform, display or sell this
  software and/or its documentation for any purpose is prohibited
  without the express written consent of Texas Instruments, Inc.

 ********************************************************************************************************/


/********************************************************************************************************
 *                                               INCLUDES
 ********************************************************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "ZMAC.h"
#include "MT_MAC.h"

#if !defined NONWK
#include "nwk.h"
#include "nwk_bufs.h"
#endif

#if defined( MACSIM )
  #include "mac_sim.h"
#endif

extern void *ZMac_ScanBuf;

/********************************************************************************************************
 *                                                 MACROS
 ********************************************************************************************************/

#define ZMAC_EVENT_ID           0x00
#define ZMAC_EVENT_STATUS       0x01

/********************************************************************************************************
 *                                               CONSTANTS
 ********************************************************************************************************/

#if !defined NONWK

#define INVALID_SHORT_ADDR    0xFFFE
#define BROADCAST_ADDR        0xFFFF

/* Lookup table for size of structures. Must match with the order of MAC callback events */
const uint8 CODE zmacCBSizeTable [] = {
  0,
  sizeof (ZMacAssociateInd_t),
  sizeof (ZMacAssociateCnf_t),
  sizeof (ZMacDisassociateInd_t),
  sizeof (ZMacDisassociateCnf_t),
  sizeof (ZMacBeaconNotifyInd_t) ,
  /*  0, // sizeof (ZMacGTSInd_t), */
  /*  0, // sizeof (ZMacGTSCnf_t), */
  sizeof (ZMacOrphanInd_t),
  sizeof (ZMacScanCnf_t),
  sizeof (ZMacStartCnf_t),
  sizeof (ZMacSyncLossInd_t),
  sizeof (ZMacPollCnf_t),
  sizeof (ZMacCommStatusInd_t),
  /*  0, // sizeof (ZMacRxEnableCnf_t), */
  sizeof (ZMacDataCnf_t),
  sizeof (ZMacDataInd_t),
  sizeof (ZMacPurgeCnf_t),
  0,
  sizeof (ZMacPollInd_t)
};

uint16 getZMacDynamicSize(uint8 event, macCbackEvent_t *pData);
uint16 getZMacDynamicSize(uint8 event, macCbackEvent_t *pData)
{
  switch(event)
  {
  case MAC_MLME_BEACON_NOTIFY_IND:
    return sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength;
  case MAC_MLME_SCAN_CNF:
    if (pData->scanCnf.scanType == ZMAC_ED_SCAN)
      return ZMAC_ED_SCAN_MAXCHANNELS;
    else
      return sizeof( ZMacPanDesc_t ) * pData->scanCnf.resultListSize;
  }

  return 0;
}

const uint8 CODE zmacCBEventTable [] = {
  0xff,                         /* Unused */
  MACCB_ASSOCIATE_IND_CMD,      /* MAC_MLME_ASSOCIATE_IND */
  MACCB_ASSOCIATE_CNF_CMD,      /* MAC_MLME_ASSOCIATE_CNF */
  MACCB_DISASSOCIATE_IND_CMD,   /* MAC_MLME_DISASSOCIATE_IND */
  MACCB_DISASSOCIATE_CNF_CMD,   /* MAC_MLME_DISASSOCIATE_CNF */
  MACCB_BEACON_NOTIFY_IND_CMD,  /* MAC_MLME_BEACON_NOTIFY_IND */
  /*  0xff,  */                 /* MAC_MLME_GTS_IND */
  /*  0xff,  */                 /* MAC_MLME_GTS_CNF */
  MACCB_ORPHAN_IND_CMD,         /* MAC_MLME_ORPHAN_IND */
  MACCB_SCAN_CNF_CMD,           /* MAC_MLME_SCAN_CNF */
  MACCB_START_CNF_CMD,          /* MAC_MLME_START_CNF */
  MACCB_SYNC_LOSS_IND_CMD,      /* MAC_MLME_SYNC_LOSS_IND */
  MACCB_POLL_CNF_CMD,           /* MAC_MLME_POLL_CNF */
  MACCB_COMM_STATUS_IND_CMD,    /* MAC_MLME_COMM_STATUS_IND */
  /*  0xff,  */                 /* MAC_MLME_RX_ENABLE_CNF */
  MACCB_DATA_CNF_CMD,           /* MAC_MCPS_DATA_CNF */
  MACCB_DATA_IND_CMD,           /* MAC_MCPS_DATA_IND */
  MACCB_PURGE_CNF_CMD,          /* MAC_MCPS_PURGE_CNF */
  0xff,                         /* MAC_PWR_ON_CNF */
  MACCB_POLL_IND_CMD            /* MAC_MLME_POLL_IND */
};

#endif /* !defined NONWK */

/********************************************************************************************************
 *                                                TYPEDEFS
 ********************************************************************************************************/

/********************************************************************************************************
 *                                               GLOBALS
 ********************************************************************************************************/
/* Send Callbacks to MT */
void zmacSendMTCallback( macCbackEvent_t *pData );


/********************************************************************************************************
 * @fn       MAC_CbackEvent()
 *
 * @brief    convert MAC data confirm and indication to ZMac and send to NWK
 *
 * @param    pData - pointer to macCbackEvent_t
 *
 * @return   none
 ********************************************************************************************************/
void  MAC_CbackEvent(macCbackEvent_t *pData)
{
#ifdef MT_MAC_CB_FUNC
  zmacSendMTCallback ( pData );
#elif !defined NONWK

  uint8 macEvent = pData->hdr.event;
  uint16 msgLen = zmacCBSizeTable[macEvent] + getZMacDynamicSize(macEvent, pData);
  macCbackEvent_t *msgPtr;

  /* Allocate osal msg buffer */
  if (msgLen > 0)
  {
    if (macEvent == MAC_MCPS_DATA_IND)
      msgPtr = pData;
    else
    {
      if ( macEvent == MAC_MCPS_DATA_CNF )
      {
        osal_msg_deallocate( (uint8*)pData->dataCnf.pDataReq );
      }
      msgPtr = (macCbackEvent_t *)osal_msg_allocate(msgLen);
    }

    if (msgPtr)
    {
      if (macEvent != MAC_MCPS_DATA_IND)
        osal_memcpy(msgPtr, pData, zmacCBSizeTable[macEvent]);

      switch (macEvent) {
      case MAC_MLME_BEACON_NOTIFY_IND:
        {
          macMlmeBeaconNotifyInd_t *pBeacon;

          pBeacon = (macMlmeBeaconNotifyInd_t*)msgPtr;

          osal_memcpy(pBeacon + 1, pBeacon->pPanDesc, sizeof(macPanDesc_t));
          pBeacon->pPanDesc = (macPanDesc_t *) (pBeacon + 1);
          osal_memcpy(pBeacon->pPanDesc + 1, pBeacon->pSdu, pBeacon->sduLength);
          pBeacon->pSdu = (uint8 *) (pBeacon->pPanDesc + 1);
        }
        break;

      case MAC_MLME_SCAN_CNF:
        {
          macMlmeScanCnf_t *pScan = (macMlmeScanCnf_t*)msgPtr;

          if (ZMac_ScanBuf != NULL)
          {
            if (pScan->scanType == ZMAC_ED_SCAN)
            {
              pScan->result.pEnergyDetect = (uint8*) (pScan + 1);
              osal_memcpy(pScan->result.pEnergyDetect, ZMac_ScanBuf, ZMAC_ED_SCAN_MAXCHANNELS);
            }
            else
            {
              pScan->result.pPanDescriptor = (macPanDesc_t*) (pScan + 1);
              osal_memcpy(pScan + 1, ZMac_ScanBuf, sizeof( ZMacPanDesc_t ) * pScan->resultListSize);
            }

            osal_mem_free(ZMac_ScanBuf);
            ZMac_ScanBuf = NULL;
          }
        }
        break;

      case MAC_MLME_START_CNF:
        msgPtr->hdr.status = pData->startCnf.hdr.status;
        break;

      case MAC_MCPS_DATA_IND:
        {
          /* Data Ind is unconventional: to save an alloc/copy, reuse the MAC
             buffer and re-organize the contents into ZMAC format. */
          uint16 shortAddr = INVALID_SHORT_ADDR;
          ZMacDataInd_t *pDataInd = (ZMacDataInd_t *) pData;
          uint8 event, status, len, *msdu;

          MAC_MlmeGetReq( MAC_SHORT_ADDRESS, &shortAddr );
          if ( shortAddr == INVALID_SHORT_ADDR || shortAddr == BROADCAST_ADDR )
          {
            osal_msg_deallocate( (uint8 *)msgPtr );
            return;
          }

          /* Store parameters */
          event = pData->hdr.event;
          status = pData->hdr.status;
          len = pData->dataInd.msdu.len;
          msdu = pData->dataInd.msdu.p;

          /* Copy header */
#if defined ( ZBIT )
          // ZBIT requires two copies to handle 32 bit alignment  
          osal_memcpy(&pDataInd->SrcAddr, &pData->dataInd.mac, sizeof(zAddrType_t) * 2 );
          osal_memcpy(&pDataInd->Timestamp, &pData->dataInd.mac.timestamp, sizeof(macDataInd_t) - sizeof(ZMacEventHdr_t) - sizeof(zAddrType_t) * 2);
#else
          osal_memcpy(&pDataInd->SrcAddr, &pData->dataInd.mac, sizeof(macDataInd_t) - sizeof(ZMacEventHdr_t));
#endif
          /* Security - set to zero for now */
          pDataInd->Sec.SecurityLevel = false;

          /* Restore parameters */
          pDataInd->hdr.Status = status;
          pDataInd->hdr.Event = event;
          pDataInd->msduLength = len;

          if (len)
            pDataInd->msdu = msdu;
          else
            pDataInd->msdu = NULL;

          break;
        }
      }

      msgPtr->hdr.event = zmacCBEventTable[macEvent];
      osal_msg_send( NWK_TaskID, (uint8 *)msgPtr );
    }
  }
#endif
}

/********************************************************************************************************
 * @fn       zmacSendMTCallback()
 *
 * @brief    convert MAC data confirm to ZMac and send to NWK
 *
 * @param    status -
 * @param    msduHandle -
 *
 * @return   none
 ********************************************************************************************************/

void zmacSendMTCallback ( macCbackEvent_t *pData )
{
#ifdef MT_MAC_CB_FUNC

  /* Check if MT has subscribed for this callback If so, pass it as an event to MonitorTest */
  switch (pData->hdr.event)
  {
    case MAC_MLME_ASSOCIATE_IND:
      if ( _macCallbackSub & CB_ID_NWK_ASSOCIATE_IND )
        nwk_MTCallbackSubNwkAssociateInd ( (ZMacAssociateInd_t *)pData );
      break;

    case MAC_MLME_ASSOCIATE_CNF:
      if ( _macCallbackSub & CB_ID_NWK_ASSOCIATE_CNF )
        nwk_MTCallbackSubNwkAssociateCnf ( (ZMacAssociateCnf_t *)pData );
      break;

    case MAC_MLME_DISASSOCIATE_IND:
      if ( _macCallbackSub & CB_ID_NWK_DISASSOCIATE_IND )
        nwk_MTCallbackSubNwkDisassociateInd ( (ZMacDisassociateInd_t *)pData );
      break;

    case MAC_MLME_DISASSOCIATE_CNF:
      if ( _macCallbackSub & CB_ID_NWK_DISASSOCIATE_CNF )
        nwk_MTCallbackSubNwkDisassociateCnf ( (ZMacDisassociateCnf_t *)pData );
      break;

    case MAC_MLME_BEACON_NOTIFY_IND:
      if ( _macCallbackSub & CB_ID_NWK_BEACON_NOTIFY_IND )
        nwk_MTCallbackSubNwkBeaconNotifyInd( (ZMacBeaconNotifyInd_t *)pData );
      break;

    case MAC_MLME_ORPHAN_IND:
      if ( _macCallbackSub & CB_ID_NWK_ORPHAN_IND )
        nwk_MTCallbackSubNwkOrphanInd( (ZMacOrphanInd_t *) pData );
      break;

    case MAC_MLME_SCAN_CNF:
      if ( _macCallbackSub & CB_ID_NWK_SCAN_CNF )
      {
        pData->scanCnf.result.pEnergyDetect = ZMac_ScanBuf;
        nwk_MTCallbackSubNwkScanCnf ( (ZMacScanCnf_t *) pData );
      }

      if (ZMac_ScanBuf != NULL)
      {
        osal_mem_free(ZMac_ScanBuf);
        ZMac_ScanBuf = NULL;
      }
      break;

    case MAC_MLME_START_CNF:
      if ( _macCallbackSub & CB_ID_NWK_START_CNF )
        nwk_MTCallbackSubNwkStartCnf ( pData->hdr.status );
      break;

    case MAC_MLME_SYNC_LOSS_IND:
      if ( _macCallbackSub & CB_ID_NWK_SYNC_LOSS_IND )
       nwk_MTCallbackSubNwkSyncLossInd( (ZMacSyncLossInd_t *) pData );
      break;

    case MAC_MLME_POLL_CNF:
      if ( _macCallbackSub & CB_ID_NWK_POLL_CNF )
         nwk_MTCallbackSubNwkPollCnf( pData->hdr.status );
      break;

    case MAC_MLME_COMM_STATUS_IND:
      if ( _macCallbackSub & CB_ID_NWK_COMM_STATUS_IND )
        nwk_MTCallbackSubCommStatusInd ( (ZMacCommStatusInd_t *) pData );
      break;

    case MAC_MCPS_DATA_CNF:
      osal_msg_deallocate((uint8*)pData->dataCnf.pDataReq);

      if ( _macCallbackSub & CB_ID_NWK_DATA_CNF )
        nwk_MTCallbackSubNwkDataCnf( (ZMacDataCnf_t *) pData );
      break;

    case MAC_MCPS_DATA_IND:
        {
          /*
             Data Ind is unconventional: to save an alloc/copy, reuse the MAC
             buffer and re-organize the contents into ZMAC format.
          */
          ZMacDataInd_t *pDataInd = (ZMacDataInd_t *) pData;
          uint8 event, status, len, *msdu;

          /* Store parameters */
          event = pData->hdr.event;
          status = pData->hdr.status;
          len = pData->dataInd.msdu.len;
          msdu = pData->dataInd.msdu.p;

          /* Copy header */
          osal_memcpy(&pDataInd->SrcAddr, &pData->dataInd.mac, sizeof(ZMacDataInd_t) - sizeof(ZMacEventHdr_t));

          /* Security - set to zero for now*/
          pDataInd->Sec.SecurityLevel = false;

          /* Restore parameters */
          pDataInd->hdr.Status = status;
          pDataInd->hdr.Event = event;
          pDataInd->msduLength = len;

          if (len)
            pDataInd->msdu = msdu;
          else
            pDataInd->msdu = NULL;

          if ( _macCallbackSub & CB_ID_NWK_DATA_IND )
            nwk_MTCallbackSubNwkDataInd ( pDataInd );

          /* free buffer */
          osal_msg_deallocate( (uint8 *) pData );
        }
        break;

    case MAC_MCPS_PURGE_CNF:
      if ( _macCallbackSub & CB_ID_NWK_PURGE_CNF )
        nwk_MTCallbackSubNwkPurgeCnf( (ZMacPurgeCnf_t *) pData);
      break;

    default:
      break;
  }

#endif
}

/********************************************************************************************************
 * @fn      MAC_CbackCheckPending
 *
 * @brief   Return number of pending indirect msg
 *
 * @param   None
 *
 * @return  Number of indirect msg holding
 ********************************************************************************************************/
uint8 MAC_CbackCheckPending(void)
{
#if !defined (NONWK) && defined (RTR_NWK)
  return (nwkDB_ReturnIndirectHoldingCnt());
#else
  return (0);
#endif
}


/********************************************************************************************************
 ********************************************************************************************************/


