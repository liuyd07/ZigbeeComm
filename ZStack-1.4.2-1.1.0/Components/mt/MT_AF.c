/*********************************************************************
    Filename:       MT_AF.c
    Revised:        $Date: 2007-04-04 07:38:24 -0700 (Wed, 04 Apr 2007) $
    Revision:       $Revision: 13958 $

    Description:

        MonitorTest functions for the AF layer.

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
#include "OSAL.h"
#include "MTEL.h"
#include "MT_AF.h"
#include "nwk.h"
#include "OnBoard.h"
#include "SPIMgr.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

#if defined ( MT_AF_CB_FUNC )
uint16 _afCallbackSub;
#endif

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

#if defined ( MT_AF_FUNC )
/*********************************************************************
 * @fn      MT_afCommandProcessing
 *
 * @brief
 *
 *   Process all the AF commands that are issued by test tool
 *
 * @param   cmd_id - Command ID
 * @param   len    - Length of received SPI data message
 * @param   data   - pointer to received SPI data message
 *
 * @return  none
 */
void MT_afCommandProcessing( uint16 cmd_id , byte len , byte *pData )
{
  byte i;
  endPointDesc_t *epDesc;
  uint8 af_stat = afStatus_FAILED;

  switch (cmd_id)
  {
    case SPI_CMD_AF_INIT:

      afInit();

      //No response for this command
      break;

    case SPI_CMD_AF_REGISTER:

      // First allocate memory for the AF structure epIntDesc
      epDesc = ( endPointDesc_t * )
                  osal_mem_alloc( sizeof( endPointDesc_t ) );

      if ( epDesc )
      {
        //Assemble the AF structures with the data received
        //First the Endpoint
        epDesc->simpleDesc = ( SimpleDescriptionFormat_t * )
                    osal_mem_alloc( sizeof( SimpleDescriptionFormat_t ) );

        if ( epDesc->simpleDesc )
        {
          epDesc->endPoint = *pData++;
          epDesc->simpleDesc->EndPoint = epDesc->endPoint;

          epDesc->task_id = &MT_TaskID;

          //Now for the simple description part
          epDesc->simpleDesc->AppProfId = BUILD_UINT16( pData[1],pData[0]);
          pData += sizeof( uint16 );
          epDesc->simpleDesc->AppDeviceId = BUILD_UINT16( pData[1],pData[0]);
          pData += sizeof( uint16 );
          epDesc->simpleDesc->AppDevVer = (*pData++) & AF_APP_DEV_VER_MASK ;
          epDesc->simpleDesc->Reserved = (*pData++) & AF_APP_FLAGS_MASK ;

          epDesc->simpleDesc->AppNumInClusters = *pData++;

          if (epDesc->simpleDesc->AppNumInClusters)
          {
            epDesc->simpleDesc->pAppInClusterList = (uint16 *)
            osal_mem_alloc(ZTEST_DEFAULT_PARAM_LEN*sizeof(uint16));

            for (i=0; i<ZTEST_DEFAULT_PARAM_LEN; i++)  {
              epDesc->simpleDesc->pAppInClusterList[i] = BUILD_UINT16(*pData, 0);
              pData++;
            }
          }
          else
          {
            pData += ZTEST_DEFAULT_PARAM_LEN;
          }

          epDesc->simpleDesc->AppNumOutClusters = *pData++;

          if (epDesc->simpleDesc->AppNumOutClusters)
          {
            epDesc->simpleDesc->pAppOutClusterList = (uint16 *)
            osal_mem_alloc(ZTEST_DEFAULT_PARAM_LEN*sizeof(uint16));

            for (i=0; i<ZTEST_DEFAULT_PARAM_LEN; i++)  {
              epDesc->simpleDesc->pAppOutClusterList[i] = BUILD_UINT16(*pData, 0);
              pData++;
            }
          }
          else
          {
            pData += ZTEST_DEFAULT_PARAM_LEN;
          }

          epDesc->latencyReq = (afNetworkLatencyReq_t)*pData;

          if ( afFindEndPointDesc( epDesc->endPoint ) == NULL )
          {
            af_stat = afRegister( epDesc );
          }
        }
        else
        {
          osal_mem_free( epDesc );
          af_stat = afStatus_MEM_FAIL;
        }
      }

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_AF_DEFAULT;
      MT_SendSPIRespMsg( af_stat, SPI_CMD_AF_REGISTER, len, SPI_RESP_LEN_AF_DEFAULT );

      break;

    case SPI_CMD_AF_SENDMSG:
    {
#if ( AF_KVP_SUPPORT )
      afKVPCommandFormat_t kvp;
      afAddOrSend_t addOrSend;
      byte frameType;
#else
      endPointDesc_t *epDesc;
      byte transId;
#endif
      afAddrType_t dstAddr;
      cId_t cId;
      byte txOpts, radius, srcEP;

#if ( AF_KVP_SUPPORT )
      frameType = *pData;
#endif
      pData++;
      txOpts = *pData++;
      radius = *pData++;

      // Fill the AF structures with the data received.
      dstAddr.addrMode = afAddr16Bit;
      dstAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;
      dstAddr.endPoint = *pData++;

      srcEP = *pData++;
      cId = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;
#if ( AF_KVP_SUPPORT )
      addOrSend = (afAddOrSend_t)(*pData++);
      kvp.TransSeqNumber = *pData++;
      kvp.CommandType = *pData++;
      kvp.AttribDataType = *pData++;
      kvp.AttribId = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;
      kvp.ErrorCode = *pData++;
      kvp.DataLength = *pData++;
      kvp.Data = pData;

      af_stat = afAddOrSendMessage( &dstAddr, srcEP, cId,
          addOrSend, frameType, &kvp.TransSeqNumber,
          kvp.CommandType, kvp.AttribDataType, kvp.AttribId, kvp.ErrorCode,
          kvp.DataLength, kvp.Data, (txOpts & ~AF_DISCV_ROUTE),
                                    (txOpts &  AF_DISCV_ROUTE), radius );
#else
      pData++;
      transId = *pData++;
      pData += 5;
      len = *pData++;
      epDesc = afFindEndPointDesc( srcEP );
      if ( epDesc == NULL )
      {
        af_stat = afStatus_INVALID_PARAMETER;
      }
      else
      {
        af_stat = AF_DataRequest( &dstAddr, epDesc, cId, len, pData,
                                  &transId, txOpts, radius );
      }
#endif

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_AF_DEFAULT;
      MT_SendSPIRespMsg( af_stat, SPI_CMD_AF_SENDMSG,
                             len, SPI_RESP_LEN_AF_DEFAULT );
    }
    break;
  }
}
#endif  // #if defined ( MT_AF_FUNC )

#if defined ( MT_AF_CB_FUNC )
/*********************************************************************
 * @fn          af_MTCB_IncomingData
 *
 * @brief       Process the callback subscription for AF Incoming data.
 *
 * @param       pkt - Incoming AF data.
 *
 * @return      none
 */
void af_MTCB_IncomingData( void *pkt )
{
  afIncomingMSGPacket_t *MSGpkt = (afIncomingMSGPacket_t *)pkt;
#if ( AF_KVP_SUPPORT )
  afIncomingKVPPacket_t *KVPpkt = (afIncomingKVPPacket_t *)pkt;
#endif
  byte *memPtr, *ptr;
  /* Frametype, WasBroadcast, LinkQuality, SecurityUse, SrcAddr,
   * SrcEndpoint, DestEndpoint, ClusterId, TransCnt, TransId, CmdType,
   * AttribDataType, AttribId, ErrorCode, TransDataLen =
   * 1+1+1+1+2+1+1+2+1+1+1+2+1+1+1.
   */
  const byte len = 18 + ZTEST_DEFAULT_AF_DATA_LEN;
  byte dataLen;

  if ( MSGpkt->hdr.event != AF_INCOMING_MSG_CMD )
  {
#if ( AF_KVP_SUPPORT )
    dataLen = KVPpkt->cmd.DataLength;
#else
    return;
#endif
  }
  else
  {
    dataLen = MSGpkt->cmd.DataLength;
  }

  if ( dataLen > ZTEST_DEFAULT_AF_DATA_LEN )
  {
    dataLen = ZTEST_DEFAULT_AF_DATA_LEN;
  }

  memPtr = osal_mem_alloc( len );
  if ( !memPtr )
  {
    return;
  }
  ptr = memPtr;

#if ( AF_KVP_SUPPORT )
  if ( MSGpkt->hdr.event != AF_INCOMING_MSG_CMD )
  {
    *ptr++ = KVPpkt->hdr.event;
    *ptr++ = KVPpkt->wasBroadcast;
    *ptr++ = KVPpkt->LinkQuality;
    *ptr++ = KVPpkt->SecurityUse;
    *ptr++ = HI_UINT16( KVPpkt->srcAddr.addr.shortAddr );
    *ptr++ = LO_UINT16( KVPpkt->srcAddr.addr.shortAddr );
    *ptr++ = KVPpkt->srcAddr.endPoint;
    *ptr++ = KVPpkt->endPoint;
    *ptr++ = HI_UINT16( KVPpkt->clusterId );
    *ptr++ = LO_UINT16( KVPpkt->clusterId );
    *ptr++ = KVPpkt->totalTrans;
    *ptr++ = KVPpkt->cmd.TransSeqNumber;
    *ptr++ = KVPpkt->cmd.CommandType;
    *ptr++ = KVPpkt->cmd.AttribDataType;
    *ptr++ = HI_UINT16( KVPpkt->cmd.AttribId );
    *ptr++ = LO_UINT16( KVPpkt->cmd.AttribId );
    *ptr++ = KVPpkt->cmd.ErrorCode;
    *ptr++ = KVPpkt->cmd.DataLength;
    osal_memcpy( ptr, KVPpkt->cmd.Data, dataLen );
  }
  else
#endif
  {
    *ptr++ = MSGpkt->hdr.event;
    *ptr++ = MSGpkt->wasBroadcast;
    *ptr++ = MSGpkt->LinkQuality;
    *ptr++ = MSGpkt->SecurityUse;
    *ptr++ = HI_UINT16( MSGpkt->srcAddr.addr.shortAddr );
    *ptr++ = LO_UINT16( MSGpkt->srcAddr.addr.shortAddr );
    *ptr++ = MSGpkt->srcAddr.endPoint;
    *ptr++ = MSGpkt->endPoint;
    *ptr++ = HI_UINT16( MSGpkt->clusterId );
    *ptr++ = LO_UINT16( MSGpkt->clusterId );
    osal_memset( ptr, 0, 7 );
    ptr += 7;
    osal_memcpy( ptr, MSGpkt->cmd.Data, dataLen );
  }

  if ( dataLen < ZTEST_DEFAULT_AF_DATA_LEN )
  {
    osal_memset( (ptr + dataLen), 0, (ZTEST_DEFAULT_AF_DATA_LEN - dataLen) );
  }

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  MT_BuildAndSendZToolCB( SPI_CB_AF_DATA_IND, len, memPtr );
#endif
  osal_mem_free( memPtr );
}
#endif  // #if defined ( MT_AF_CB_FUNC )

/*********************************************************************
*********************************************************************/
