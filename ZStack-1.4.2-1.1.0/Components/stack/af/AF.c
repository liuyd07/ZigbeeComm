/*********************************************************************
  Filename:       AF.c
  Revised:        $Date: 2007-05-14 17:34:18 -0700 (Mon, 14 May 2007) $
  Revision:       $Revision: 14296 $

  Description:    General Operational Framework
                  - Device Description helper functions

  Notes:

  If supporting KVP, then afRegister() ties endpoint to KVP only in V2.
  If KVP in V2,

  Copyright (c) 2006 by Texas Instruments, Inc.
  All Rights Reserved.  Permission to use, reproduce, copy, prepare
  derivative works, modify, distribute, perform, display or sell this
  software and/or its documentation for any purpose is prohibited
  without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "OSAL.h"
#include "AF.h"
#include "nwk_globals.h"
#include "nwk_util.h"
#include "aps_groups.h"
#include "ZDProfile.h"
#include "aps_frag.h"

#if ( AF_FLOAT_SUPPORT )
  #include "math.h"
#endif

#if defined ( MT_AF_CB_FUNC )
  #include "MT_AF.h"
#endif

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * @fn      afSend
 *
 * @brief   Helper macro for V1 API to invoke V2 API.
 *
 * input parameters
 *
 * @param  *dstAddr - Full ZB destination address: Nwk Addr + End Point.
 * @param   srcEP - Origination (i.e. respond to or ack to) End Point.
 * @param   cID - A valid cluster ID as specified by the Profile.
 * @param   len - Number of bytes of data pointed to by next param.
 * @param  *buf - A pointer to the data bytes to send.
 * @param   options - Valid bit mask of AF Tx Options as defined in AF.h.
 * @param  *transID - A pointer to a byte which can be modified and which will
 *                    be used as the transaction sequence number of the msg.
 *
 * output parameters
 *
 * @param  *transID - Incremented by one if the return value is success.
 *
 * @return  afStatus_t - See previous definition of afStatus_... types.
 */
#if ( AF_V1_SUPPORT )
#define afSend( dstAddr, srcEP, cID, len, buf, transID, options, radius ) \
        afDataRequest( (dstAddr), afFindEndPointDesc( (srcEP) ), \
                          (cID), (len), (buf), (transID), (options), (radius) )
#else
#define afSend( dstAddr, srcEP, cID, len, buf, transID, options, radius ) \
        AF_DataRequest( (dstAddr), afFindEndPointDesc( (srcEP) ), \
                          (cID), (len), (buf), (transID), (options), (radius) )
#endif

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

#if ( AF_KVP_SUPPORT )
typedef struct
{
  uint16 clusterID;
  byte transCount;
  byte options;
  byte srcEP;
  afAddrType_t dstAddr;	
  uint16 msgLen;
  byte *msg;
} afMultiHdr_t;
#endif

/*********************************************************************
 * GLOBAL VARIABLES
 */

epList_t *epList;

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

#if ( AF_KVP_SUPPORT )
static void afBuildKVPIncoming( aps_FrameFormat_t *aff, endPointDesc_t *epDesc,
                zAddrType_t *SrcAddress, uint8 LinkQuality, byte SecurityUse );
#endif

static void afBuildMSGIncoming( aps_FrameFormat_t *aff, endPointDesc_t *epDesc,
                zAddrType_t *SrcAddress, uint8 LinkQuality, byte SecurityUse,
                uint32 timestamp );

#if ( AF_KVP_SUPPORT )
static afMultiHdr_t *multiInit( afAddrType_t *dstAddr,
                     byte srcEndPoint, uint16 clusterID, byte FrameType,
                     byte txOptions, bool DiscoverRoute, byte RadiusCounter );

static bool multiAppend( afMultiHdr_t *hdr, byte bufLen, byte *buf,
       byte CommandType, byte AttribDataType, uint16 AttribId, byte ErrorCode );

static afStatus_t multiSend( afMultiHdr_t *hdr, byte *seqNum, byte radius );
#endif

#if ( AF_V1_SUPPORT )
static afStatus_t afDataRequest( afAddrType_t *dstAddr, endPointDesc_t *srcEP,
                           uint16 cID, uint16 len, uint8 *buf, uint8 *transID,
                           uint8 options, uint8 radius );
#endif

static epList_t *afFindEndPointDescList( byte EndPoint );

static pDescCB afGetDescCB( endPointDesc_t *epDesc );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      afInit
 *
 * @brief   Initialization function for the AF.
 *
 * @param   none
 *
 * @return  none
 */
void afInit( void )
{
  // Start with no endpoint defined
  epList = NULL;
}

/*********************************************************************
 * @fn      afRegisterExtended
 *
 * @brief   Register an Application's EndPoint description.
 *
 * @param   epDesc - pointer to the Application's endpoint descriptor.
 * @param   descFn - pointer to descriptor callback function
 *
 * NOTE:  The memory that epDesc is pointing to must exist after this call.
 *
 * @return  Pointer to epList_t on success, NULL otherwise.
 */
epList_t *afRegisterExtended( endPointDesc_t *epDesc, pDescCB descFn )
{
  epList_t *ep;
  epList_t *epSearch;

  ep = osal_mem_alloc( sizeof ( epList_t ) );
  if ( ep )
  {
    // Fill in the new list entry
    ep->epDesc = epDesc;
    #if !defined ( REFLECTOR )
    ep->reflectorAddr = NWK_PAN_COORD_ADDR;
    #else
    ep->reflectorAddr = INVALID_NODE_ADDR;
    #endif
    // Default to allow Match Descriptor.
    ep->flags = eEP_AllowMatch;
#if ( AF_KVP_SUPPORT )
    ep->flags |= ((epDesc->endPoint == ZDO_EP) ? 0 : eEP_UsesKVP);
#endif
    ep->pfnDescCB = descFn;
    ep->nextDesc = NULL;

    // Does a list exist?
    if ( epList == NULL )
      epList = ep;  // Make this the first entry
    else
    {
      // Look for the end of the list
      epSearch = epList;
      while( epSearch->nextDesc != NULL )
        epSearch = epSearch->nextDesc;

      // Add new entry to end of list
      epSearch->nextDesc = ep;
    }
  }

  return ep;
}

/*********************************************************************
 * @fn      afRegister
 *
 * @brief   Register an Application's EndPoint description.
 *
 * @param   epDesc - pointer to the Application's endpoint descriptor.
 *
 * NOTE:  The memory that epDesc is pointing to must exist after this call.
 *
 * @return  afStatus_SUCCESS - Registered
 *          afStatus_MEM_FAIL - not enough memory to add descriptor
 */
afStatus_t afRegister( endPointDesc_t *epDesc )
{
  epList_t *ep = afRegisterExtended( epDesc, NULL );

  return ((ep == NULL) ? afStatus_MEM_FAIL : afStatus_SUCCESS);
}

#if ( AF_KVP_SUPPORT )
/*********************************************************************
 * @fn      afRegisterFlags
 *
 * @brief   Register an Application's EndPoint description.
 *
 * @param   epDesc - pointer to the Application's endpoint descriptor.
 *
 * NOTE:  The memory that epDesc is pointing to must exist after this call.
 *
 * @return  afStatus_SUCCESS - Registered
 *          afStatus_MEM_FAIL - not enough memory to add descriptor
 */
afStatus_t afRegisterFlags( endPointDesc_t *epDesc, eEP_Flags flags )
{
  epList_t *ep = afRegisterExtended( epDesc, NULL );

  if ( ep != NULL )
  {
    ep->flags = flags;
    return afStatus_SUCCESS;
  }
  else
  {
    return afStatus_MEM_FAIL;
  }
}
#endif

/*********************************************************************
 * @fn          afDataConfirm
 *
 * @brief       This function will generate the Data Confirm back to
 *              the application.
 *
 * @param       endPoint - confirm end point
 * @param       transID - transaction ID from APSDE_DATA_REQUEST
 * @param       status - status of APSDE_DATA_REQUEST
 *
 * @return      none
 */
void afDataConfirm( uint8 endPoint, uint8 transID, ZStatus_t status )
{
  endPointDesc_t *epDesc;
  afDataConfirm_t *msgPtr;

  // Find the endpoint description
  epDesc = afFindEndPointDesc( endPoint );
  if ( epDesc == NULL )
    return;

  // Determine the incoming command type
  msgPtr = (afDataConfirm_t *)osal_msg_allocate( sizeof(afDataConfirm_t) );
  if ( msgPtr )
  {
    // Build the Data Confirm message
    msgPtr->hdr.event = AF_DATA_CONFIRM_CMD;
    msgPtr->hdr.status = status;
    msgPtr->endpoint = endPoint;
#if ( AF_V1_SUPPORT )
    msgPtr->transID = transID;
#elif ( AF_KVP_SUPPORT )
    {
      epList_t *pList = afFindEndPointDescList( endPoint );
      if ( pList )
      {
        if ( pList->flags & eEP_UsesKVP )
        {
          msgPtr->transID = transID;
        }
      }
    }
#else
    (void)transID;
#endif

    // send message through task message
    osal_msg_send( *(epDesc->task_id), (byte *)msgPtr );
  }
}

/*********************************************************************
 * @fn          afIncomingData
 *
 * @brief       Transfer a data PDU (ASDU) from the APS sub-layer to the AF.
 *
 * @param       aff  - pointer to APS frame format
 * @param       SrcAddress  - Source address
 * @param       LinkQuality - incoming message's link quality
 * @param       SecurityUse - Security enable/disable
 *
 * @return      none
 */
void afIncomingData( aps_FrameFormat_t *aff, zAddrType_t *SrcAddress,
                     uint8 LinkQuality, byte SecurityUse, uint32 timestamp )
{
  endPointDesc_t *epDesc = NULL;
  uint16 epProfileID = 0xFFFF;  // Invalid Profile ID
  epList_t *pList;
  uint8 grpEp;

  if ( (NLME_GetProtocolVersion() != ZB_PROT_V1_0)
      && ((aff->FrmCtrl & APS_DELIVERYMODE_MASK) == APS_FC_DM_GROUP) )
  {
    // Find the first endpoint for this group
    grpEp = aps_FindGroupForEndpoint( aff->GroupID, APS_GROUPS_FIND_FIRST );
    if ( grpEp == APS_GROUPS_EP_NOT_FOUND )
      return;   // No endpoint found

    epDesc = afFindEndPointDesc( grpEp );
    if ( epDesc == NULL )
      return;   // Endpoint descriptor not found

    pList = afFindEndPointDescList( epDesc->endPoint );
  }
  else if ( aff->DstEndPoint == AF_BROADCAST_ENDPOINT )
  {
    // Set the list
    if ( (pList = epList) )
    {
      epDesc = pList->epDesc;
    }
  }
  else if ( (epDesc = afFindEndPointDesc( aff->DstEndPoint )) )
  {
    pList = afFindEndPointDescList( epDesc->endPoint );
  }

  while ( epDesc )
  {
    if ( pList->pfnDescCB )
    {
      uint16 *pID = (uint16 *)(pList->pfnDescCB(
                                 AF_DESCRIPTOR_PROFILE_ID, epDesc->endPoint ));
      if ( pID )
      {
        epProfileID = *pID;
        osal_mem_free( pID );
      }
    }
    else if ( epDesc->simpleDesc )
    {
      epProfileID = epDesc->simpleDesc->AppProfId;
    }

    if ( (aff->ProfileID == epProfileID) ||
         ((epDesc->endPoint == ZDO_EP) && (aff->ProfileID == ZDO_PROFILE_ID)) )
    {
#if ( AF_KVP_SUPPORT )
      if ( (FRAMETYPE_KVP == HI_UINT8(aff->asdu[0])) &&
           (pList->flags & eEP_UsesKVP) )
      {
        afBuildKVPIncoming( aff, epDesc, SrcAddress, LinkQuality, SecurityUse );
      }
      else
#endif
      {
        afBuildMSGIncoming( aff, epDesc, SrcAddress, LinkQuality, SecurityUse, timestamp );
      }
    }

    if ( (NLME_GetProtocolVersion() != ZB_PROT_V1_0)
      && ((aff->FrmCtrl & APS_DELIVERYMODE_MASK) == APS_FC_DM_GROUP) )
    {
      // Find the next endpoint for this group
      grpEp = aps_FindGroupForEndpoint( aff->GroupID, grpEp );
      if ( grpEp == APS_GROUPS_EP_NOT_FOUND )
        return;   // No endpoint found

      epDesc = afFindEndPointDesc( grpEp );
      if ( epDesc == NULL )
        return;   // Endpoint descriptor not found

      pList = afFindEndPointDescList( epDesc->endPoint );
    }
    else if ( aff->DstEndPoint == AF_BROADCAST_ENDPOINT )
    {
      pList = pList->nextDesc;
      if ( pList )
        epDesc = pList->epDesc;
      else
        epDesc = NULL;
    }
    else
      epDesc = NULL;
  }
}

#if ( AF_KVP_SUPPORT )
/*********************************************************************
 * @fn          afBuildKVPIncoming
 *
 * @brief       Build the message for the app
 *
 * @param
 *
 * @return
 */
static void afBuildKVPIncoming( aps_FrameFormat_t *aff, endPointDesc_t *epDesc,
                 zAddrType_t *SrcAddress, uint8 LinkQuality, byte SecurityUse )
{
  afIncomingKVPPacket_t *KVPpkt = (afIncomingKVPPacket_t *)osal_mem_alloc(
                           sizeof( afIncomingKVPPacket_t ) );
  byte *asdu = aff->asdu;
  const byte total = LO_UINT8( *asdu++ );
  byte count;

  if ( KVPpkt == NULL )
  {
    return;
  }

  KVPpkt->hdr.event = ((total > 1) ? AF_INCOMING_GRP_KVP_CMD :
                                     AF_INCOMING_KVP_CMD );
  KVPpkt->clusterId = aff->ClusterID;
  afCopyAddress( &(KVPpkt->srcAddr), SrcAddress );
  KVPpkt->srcAddr.endPoint = aff->SrcEndPoint;
  KVPpkt->endPoint = epDesc->endPoint;
  KVPpkt->wasBroadcast = aff->wasBroadcast;
  KVPpkt->LinkQuality = LinkQuality;
  KVPpkt->SecurityUse = SecurityUse;
  KVPpkt->totalTrans = total;
  KVPpkt->cmd.TransSeqNumber = *asdu++;

  for ( count = 1; count <= total; count++ )
  {
    afIncomingKVPPacket_t *msgKVPpkt;

    KVPpkt->cmd.CommandType = LO_UINT8(*asdu);
    KVPpkt->cmd.AttribDataType = HI_UINT8(*asdu++);
    KVPpkt->cmd.AttribId = BUILD_UINT16( asdu[0], asdu[1] );
    asdu += 2;

    /* ErrorCode */
    if ((KVPpkt->cmd.CommandType == CMDTYPE_GET_RESP) ||
        (KVPpkt->cmd.CommandType == CMDTYPE_SET_RESP) ||
        (KVPpkt->cmd.CommandType == CMDTYPE_EVENT_RESP))
    {
      KVPpkt->cmd.ErrorCode = *asdu++;
    }
    else
    {
      KVPpkt->cmd.ErrorCode = ERRORCODE_SUCCESS;
    }

    /* DataLength */
    if ( (KVPpkt->cmd.CommandType == CMDTYPE_GET_ACK) ||
         (KVPpkt->cmd.CommandType == CMDTYPE_SET_RESP) ||
         (KVPpkt->cmd.CommandType == CMDTYPE_EVENT_RESP ) )
    {
      KVPpkt->cmd.DataLength = 0;
    }
    else if ((KVPpkt->cmd.AttribDataType == DATATYPE_CHAR_STR) ||
             (KVPpkt->cmd.AttribDataType == DATATYPE_OCTET_STR))
    {
      KVPpkt->cmd.DataLength = *asdu++;
    }
    else
    {
      KVPpkt->cmd.DataLength = GetDataTypeLength( (byte)KVPpkt->cmd.AttribDataType );
    }

    msgKVPpkt = (afIncomingKVPPacket_t *)osal_msg_allocate( (short)
                 (sizeof( afIncomingKVPPacket_t ) + KVPpkt->cmd.DataLength) );

    if ( msgKVPpkt == NULL )
    {
      // Maybe not enought for an octet string, but next might be no data,
      // so don't return here, just continue to look at the rest.
      asdu += KVPpkt->cmd.DataLength;
      continue;
    }

    osal_memcpy( msgKVPpkt, KVPpkt, sizeof( afIncomingKVPPacket_t ) );

    msgKVPpkt->count = count;

    if ( msgKVPpkt->cmd.DataLength )
    {
      msgKVPpkt->cmd.Data = (byte *)(msgKVPpkt + 1);
      osal_memcpy( msgKVPpkt->cmd.Data, asdu, msgKVPpkt->cmd.DataLength );
      asdu += msgKVPpkt->cmd.DataLength;
    }
    else
    {
      msgKVPpkt->cmd.Data = NULL;
    }

#if defined ( MT_AF_CB_FUNC )
    // If MT has subscribed for this callback, don't send as a message.
    if ( _afCallbackSub & CB_ID_AF_DATA_IND )
    {
      af_MTCB_IncomingData( (void *)msgKVPpkt );
      // Release the memory.
      osal_msg_deallocate( (void *)msgKVPpkt );
    }
    else
#endif
    {
      osal_msg_send( *(epDesc->task_id), (uint8*)msgKVPpkt );
    }
  }

  osal_mem_free( KVPpkt );
}
#endif

/*********************************************************************
 * @fn          afBuildMSGIncoming
 *
 * @brief       Build the message for the app
 *
 * @param
 *
 * @return      pointer to next in data buffer
 */
static void afBuildMSGIncoming( aps_FrameFormat_t *aff, endPointDesc_t *epDesc,
                 zAddrType_t *SrcAddress, uint8 LinkQuality, byte SecurityUse,
                 uint32 timestamp )
{
  afIncomingMSGPacket_t *MSGpkt;
#if ( AF_V1_SUPPORT )
  const byte proVer = NLME_GetProtocolVersion();
  const byte len = sizeof( afIncomingMSGPacket_t ) +
              ((proVer == ZB_PROT_V1_0) ? aff->asdu[2] : aff->asduLength);
  byte *asdu = aff->asdu + ((proVer == ZB_PROT_V1_0) ? 1 : 0);
#else
  const byte len = sizeof( afIncomingMSGPacket_t ) + aff->asduLength;
  byte *asdu = aff->asdu;
#endif
  MSGpkt = (afIncomingMSGPacket_t *)osal_msg_allocate( len );

  if ( MSGpkt == NULL )
  {
    return;
  }

  MSGpkt->hdr.event = AF_INCOMING_MSG_CMD;
  MSGpkt->groupId = aff->GroupID;
  MSGpkt->clusterId = aff->ClusterID;
  afCopyAddress( &MSGpkt->srcAddr, SrcAddress );
  MSGpkt->srcAddr.endPoint = aff->SrcEndPoint;
  MSGpkt->endPoint = epDesc->endPoint;
  MSGpkt->wasBroadcast = aff->wasBroadcast;
  MSGpkt->LinkQuality = LinkQuality;
  MSGpkt->SecurityUse = SecurityUse;
  MSGpkt->timestamp = timestamp;

#if ( AF_V1_SUPPORT )
  if ( proVer == ZB_PROT_V1_0 )
  {
    MSGpkt->cmd.TransSeqNumber = *asdu++;
    MSGpkt->cmd.DataLength = *asdu++;
  }
  else
#endif
  {
    MSGpkt->cmd.TransSeqNumber = 0;
    MSGpkt->cmd.DataLength = aff->asduLength;
  }

  if ( MSGpkt->cmd.DataLength )
  {
    MSGpkt->cmd.Data = (byte *)(MSGpkt + 1);
    osal_memcpy( MSGpkt->cmd.Data, asdu, MSGpkt->cmd.DataLength );
  }
  else
  {
    MSGpkt->cmd.Data = NULL;
  }

#if defined ( MT_AF_CB_FUNC )
  // If MT has subscribed for this callback, don't send as a message.
  if ( _afCallbackSub & CB_ID_AF_DATA_IND )
  {
    af_MTCB_IncomingData( (void *)MSGpkt );
    // Release the memory.
    osal_msg_deallocate( (void *)MSGpkt );
  }
  else
#endif
  {
    // Send message through task message.
    osal_msg_send( *(epDesc->task_id), (uint8 *)MSGpkt );
  }
}

#if ( AF_KVP_SUPPORT )
/*********************************************************************
 * @fn      multiInit
 *
 * @brief   Initialize a multi header.
 *
 * @param   none
 *
 * @return  Pointer to the new memory allocated for the afMultiHdr_t structure.
 */
static afMultiHdr_t *multiInit( afAddrType_t *dstAddr,
                     byte srcEndPoint, uint16 clusterID, byte FrameType,
                     byte txOptions, bool DiscoverRoute, byte RadiusCounter )
{
  afMultiHdr_t *hdr = (afMultiHdr_t *)osal_mem_alloc( sizeof( afMultiHdr_t ) );

  if ( hdr == NULL )
  {
    return NULL;
  }

  hdr->clusterID = clusterID;
  hdr->transCount = 0;
  hdr->options = txOptions & AF_ACK_REQUEST;
  hdr->options |= (DiscoverRoute) ? AF_DISCV_ROUTE : 0;
  hdr->srcEP = srcEndPoint;
  hdr->dstAddr.endPoint = dstAddr->endPoint;
  hdr->dstAddr.addrMode = dstAddr->addrMode;
  hdr->dstAddr.addr.shortAddr = dstAddr->addr.shortAddr;
  hdr->msgLen = 0;
  hdr->msg = NULL;

  return hdr;
}

/*********************************************************************
 * @fn      multiAppend
 *
 * @brief   Concatenate a new KVP message too a mulit-KVP message.
 *
 * @param   none
 *
 * @return  TRUE for success, FALSE otherwise.
 */
static bool multiAppend( afMultiHdr_t *hdr, byte bufLen, byte *buf,
       byte CommandType, byte AttribDataType, uint16 AttribId, byte ErrorCode )
{
  byte len = (byte)hdr->msgLen;
  byte *msg = osal_mem_alloc( (short)(AF_HDR_KVP_MAX_LEN + bufLen + len) );

  if ( msg == NULL )
  {
    return FALSE;
  }

  if ( hdr->msg == NULL )  // Need to make space for FrameType/Count & SeqNum.
  {
    len += 5;
    hdr->msg = msg;
    msg += 2;
  }
  else                     // Need to copy existing buffer over.
  {
    len += 3;
    osal_memcpy( msg, hdr->msg, hdr->msgLen );
    osal_mem_free( hdr->msg );
    hdr->msg = msg;
    msg += hdr->msgLen;
  }

  *msg++ = BUILD_UINT8( AttribDataType, CommandType );
  *msg++ = LO_UINT16( AttribId);
  *msg++ = HI_UINT16( AttribId );

  if ((CommandType == CMDTYPE_GET_RESP) ||
      (CommandType == CMDTYPE_SET_RESP) ||
      (CommandType == CMDTYPE_EVENT_RESP) )
  {
    *msg++ = ErrorCode;
    len++;
  }

  if ( (CommandType == CMDTYPE_GET_ACK) ||
       (CommandType == CMDTYPE_SET_RESP) ||
       (CommandType == CMDTYPE_EVENT_RESP ) )
  {
    bufLen = 0;
  }
  else if ((AttribDataType == DATATYPE_CHAR_STR) ||
           (AttribDataType == DATATYPE_OCTET_STR))
  {
    *msg++ = bufLen;
    len++;
  }

  if ( buf && bufLen )
  {
    osal_memcpy( msg, buf, bufLen );
    len += bufLen;
  }

  hdr->msgLen = len;
  hdr->transCount++;

  return TRUE;
}

/*********************************************************************
 * @fn      multiSend
 *
 * @brief   Send whatever has been buffered to send multi.
 *
 * @param   none
 *
 * @return  afStatus_t
 */
static afStatus_t multiSend( afMultiHdr_t *hdr, byte *seqNum, byte radius )
{
  afStatus_t stat;

  if ( hdr == NULL )
  {
    return afStatus_MEM_FAIL;
  }

  hdr->msg[0] = BUILD_UINT8( FRAMETYPE_KVP, hdr->transCount );
  hdr->msg[1] = *seqNum;

  stat = afSend( &hdr->dstAddr, hdr->srcEP, hdr->clusterID,
                  hdr->msgLen, hdr->msg, seqNum, hdr->options, radius );

  osal_mem_free( hdr->msg );
  osal_mem_free( hdr );

  return stat;
}

/*********************************************************************
 * @fn      afAddOrSendMessage
 *
 * @brief   Fills in the cmd format structure and sends the out-going message.
 *
 * input parameters
 *
 * @param  *dstAddr         - Full ZB destination address: Nwk Addr + End Point.
 * @param   srcEndPoint     - Origination (i.e. respond to or ack to) End Point.
 * @param   clusterID       - A valid cluster ID as specified by the Profile
 *          (e.g. see CLUSTERID_... defs in HomeControlLighting.h).
 * @param   AddOrSend       - A valid afAddOrSend_t type.
 * @param   FrameType       - A valid frame type (see prev def of FRAMETYPE_...)
 * @param  *TransSeqNumber  - A pointer to a byte which can be modified and
 *          which will be used as the transaction sequence number of the msg.
 * @param   CommandType     -
 *          KVP Frame Type  : A valid command type (see prev def of CMDTYPE_...)
 *          MSG Frame Type  : Set to 0.
 * @param   AttribDataType  -
 *          KVP Frame Type  : A valid data type (see prev def of DATATYPE_...)
 *          MSG Frame Type  : Set to 0.
 * @param   AttribId        -
 *          KVP Frame Type  : A valid Attribute ID as specified by the Profile.
 *            (e.g. see OnOffSRC_OnOff in HomeControlLighting.h).
 *          MSG Frame Type  : Set to 0.
 * @param   ErrorCode       -
 *          KVP Frame Type  : A valid error code (see prev def of ERRORCODE_...)
 *          Note: Set to 0 unless the CommandType is one of the CMDTYPE_..._ACK.
 *          MSG Frame Type  : Set to 0.
 * @param   DataLength      - Number of bytes of data pointed to by next param.
 *          KVP Frame Type  : This must match value expected for the
 *          AttributeDataType specified above or the send will fail.
 *          MSG Frame Type  : No restriction or checks.
 * @param  *Data            - A pointer to the data bytes to send.
 * @param   txOptions       - A valid bit mask (see prev def of APS_TX_OPTIONS_)
 * @param   DiscoverRoute   - Normally set to FALSE. Perhaps set to TRUE after
 *          receiving an AF_DATA_CONFIRM_CMD with status other than ZSuccess.
 * @param   RadiusCounter   - Normally set to AF_DEFAULT_RADIUS.
 *
 * output parameters
 *
 * @param  *TransSeqNumber  - Incremented by one if the return value is success.
 *
 * @return  afStatus_t      - See previous definition of afStatus_... types.
 */
afStatus_t afAddOrSendMessage(
    afAddrType_t *dstAddr, byte srcEndPoint, cId_t clusterID,
    afAddOrSend_t AddOrSend, byte FrameType, byte *TransSeqNumber,
    byte CommandType, byte AttribDataType, uint16 AttribId, byte ErrorCode,
    byte DataLength, byte *Data,
    byte txOptions, byte DiscoverRoute, byte RadiusCounter )
{
  static afMultiHdr_t *hdr;
  afStatus_t stat = afStatus_FAILED;

  if ( FrameType == FRAMETYPE_MSG )
  {
    if ( AddOrSend != SEND_MESSAGE )
    {
      return afStatus_INVALID_PARAMETER;
    }

    return afFillAndSendMessage( dstAddr, srcEndPoint, clusterID,
      1, FrameType, TransSeqNumber, NULL, NULL, NULL, NULL,
      DataLength, Data, txOptions, DiscoverRoute, RadiusCounter );
  }

  if ( FrameType != FRAMETYPE_KVP )
  {
    return afStatus_INVALID_PARAMETER;
  }

  // Cannot mix Cluster ID's in aggregation packet.
  if ( (hdr != NULL) && (hdr->clusterID != clusterID) )
  {
    multiSend( hdr, TransSeqNumber, RadiusCounter );
    hdr = NULL;
  }

  if ( hdr == NULL )
  {
    hdr = multiInit( dstAddr, srcEndPoint, clusterID, FrameType,
                     txOptions, DiscoverRoute, RadiusCounter );
    if ( hdr == NULL )
    {
      return afStatus_MEM_FAIL;
    }
  }

  if ( FALSE == multiAppend( hdr, (byte)DataLength, Data,
                           CommandType, AttribDataType, AttribId, ErrorCode ) )
  {
    if ( hdr->msg != NULL )
    {
      multiSend( hdr, TransSeqNumber, RadiusCounter );
    }
    else
    {
      osal_mem_free( hdr );
    }
    hdr = NULL;
    return afStatus_MEM_FAIL;
  }

  // Only supporting up to 16 concatenated messages.
  if ( (AddOrSend == SEND_MESSAGE) || (hdr->transCount >= 16) )
  {
    stat = multiSend( hdr, TransSeqNumber, RadiusCounter );
    hdr = NULL;
  }
  else
  {
    stat = afStatus_SUCCESS;
    (*TransSeqNumber)++;
  }

  return stat;
}
#endif

#if ( AF_V1_SUPPORT || AF_KVP_SUPPORT )
/*********************************************************************
 * @fn      afFillAndSendMessage
 *
 * @brief   Fills in the cmd format structure and sends the out-going message.
 *
 * input parameters
 *
 * @param  *dstAddr         - Full ZB destination address: Nwk Addr + End Point.
 * @param   srcEndPoint     - Origination (i.e. respond to or ack to) End Point.
 * @param   clusterID       - A valid cluster ID as specified by the Profile.
 * @param   TransCount      - Set to 1.
 * @param   FrameType       - A valid frame type (see prev def of FRAMETYPE_...)
 * @param  *TransSeqNumber  - A pointer to a byte which can be modified and
 *          which will be used as the transaction sequence number of the msg.
 * @param   CommandType     -
 *          KVP Frame Type  : A valid command type (see prev def of CMDTYPE_...)
 *          MSG Frame Type  : Set to 0.
 * @param   AttribDataType  -
 *          KVP Frame Type  : A valid data type (see prev def of DATATYPE_...)
 *          MSG Frame Type  : Set to 0.
 * @param   AttribId        -
 *          KVP Frame Type  : A valid Attribute ID as specified by the Profile.
 *            (e.g. see OnOffSRC_OnOff in HomeControlLighting.h).
 *          MSG Frame Type  : Set to 0.
 * @param   ErrorCode       -
 *          KVP Frame Type  : A valid error code (see prev def of ERRORCODE_...)
 *          Note: Set to 0 unless the CommandType is one of the CMDTYPE_..._ACK.
 *          MSG Frame Type  : Set to 0.
 * @param   DataLength      - Number of bytes of data pointed to by next param.
 *          KVP Frame Type  : This must match value expected for the
 *          AttributeDataType specified above or the send will fail.
 *          MSG Frame Type  : No restriction or checks.
 * @param  *Data            - A pointer to the data bytes to send.
 * @param   txOptions       - A valid bit mask (see prev def of APS_TX_OPTIONS_)
 * @param   DiscoverRoute   - Normally set to FALSE. Perhaps set to TRUE after
 *          receiving an AF_DATA_CONFIRM_CMD with status other than ZSuccess.
 * @param   RadiusCounter   - Normally set to AF_DEFAULT_RADIUS.
 *
 * output parameters
 *
 * @param  *TransSeqNumber  - Incremented by one if the return value is success.
 *
 * @return  afStatus_t      - See previous definition of afStatus_... types.
 */
afStatus_t afFillAndSendMessage (
  afAddrType_t *dstAddr, byte srcEndPoint, cId_t clusterID,
  byte TransCount, byte FrameType, byte *TransSeqNumber,
  byte CommandType, byte AttribDataType, uint16 AttribId, byte ErrorCode,
  byte DataLength, byte *Data,
  byte txOptions, byte DiscoverRoute, byte RadiusCounter )
{
  const byte proVer = NLME_GetProtocolVersion();
  afStatus_t stat = afStatus_FAILED;
  byte *buf = Data;

#if ( AF_KVP_SUPPORT )
  if ( FrameType == FRAMETYPE_KVP )
  {
    return afAddOrSendMessage( dstAddr, srcEndPoint, clusterID,
      SEND_MESSAGE, FrameType, TransSeqNumber,
      CommandType, AttribDataType, AttribId, ErrorCode,
      DataLength, Data, txOptions, DiscoverRoute, RadiusCounter );
  }
#endif

  if ( FrameType != FRAMETYPE_MSG )
  {
    return afStatus_INVALID_PARAMETER;
  }

  if ( TransCount != 1 )
  {
    return afStatus_INVALID_PARAMETER;
  }

  if ( proVer == ZB_PROT_V1_0 )
  {
    buf = osal_mem_alloc( (short)(DataLength+3) );
    if ( buf == NULL )
    {
      return afStatus_MEM_FAIL;
    }

    buf[0] = BUILD_UINT8( FRAMETYPE_MSG, 1 );
    buf[1] = *TransSeqNumber;
    buf[2] = (byte)DataLength;

    // Copy the data portion of the packet.
    if ( Data && DataLength )
    {
      osal_memcpy( buf+3, Data, DataLength );
      DataLength += 3;
    }
    else
    {
      DataLength = 3;
    }
  }

  stat = afSend( dstAddr, srcEndPoint, clusterID,
                 DataLength, buf, TransSeqNumber, txOptions, RadiusCounter );

  if ( proVer == ZB_PROT_V1_0 )
  {
    osal_mem_free( buf );
  }

  return stat;
}
#endif

/*********************************************************************
 * @fn      AF_DataRequest
 *
 * @brief   Common functionality for invoking APSDE_DataReq() for both
 *          KVP-Send/SendMulti and MSG-Send.
 *
 * input parameters
 *
 * @param  *dstAddr - Full ZB destination address: Nwk Addr + End Point.
 * @param  *srcEP - Origination (i.e. respond to or ack to) End Point Descr.
 * @param   cID - A valid cluster ID as specified by the Profile.
 * @param   len - Number of bytes of data pointed to by next param.
 * @param  *buf - A pointer to the data bytes to send.
 * @param  *transID - A pointer to a byte which can be modified and which will
 *                    be used as the transaction sequence number of the msg.
 * @param   options - Valid bit mask of Tx options.
 * @param   radius - Normally set to AF_DEFAULT_RADIUS.
 *
 * output parameters
 *
 * @param  *transID - Incremented by one if the return value is success.
 *
 * @return  afStatus_t - See previous definition of afStatus_... types.
 */
#if ( AF_V1_SUPPORT )
static afStatus_t afDataRequest( afAddrType_t *dstAddr, endPointDesc_t *srcEP,
                           uint16 cID, uint16 len, uint8 *buf, uint8 *transID,
                           uint8 options, uint8 radius )
#else
afStatus_t AF_DataRequest( afAddrType_t *dstAddr, endPointDesc_t *srcEP,
                           uint16 cID, uint16 len, uint8 *buf, uint8 *transID,
                           uint8 options, uint8 radius )
#endif
{
  pDescCB pfnDescCB;
  ZStatus_t stat;
  APSDE_DataReq_t req;
  afDataReqMTU_t mtu;


  if ( srcEP == NULL )
  {
    return afStatus_INVALID_PARAMETER;
  }

  // Enforce consistent values on the destination address / address mode.
  if ( dstAddr->addrMode == afAddrNotPresent )
  {
    dstAddr->endPoint = ZDO_EP;
    req.dstAddr.addr.shortAddr = afGetReflector( srcEP->endPoint );
  }
  else // Handle types: afAddr16Bit
  {    //               afAddrBroadcast
       //               afAddrGroup
    req.dstAddr.addr.shortAddr = dstAddr->addr.shortAddr;

    if ( ( dstAddr->addrMode == afAddr16Bit     ) ||
         ( dstAddr->addrMode == afAddrBroadcast )    )
    {
      // Check for valid broadcast values
      if( ADDR_NOT_BCAST != NLME_IsAddressBroadcast( dstAddr->addr.shortAddr )  )
      {
        // Force mode to broadcast
        dstAddr->addrMode = afAddrBroadcast;
      }
      else
      {
        // Address is not a valid broadcast type
        if ( dstAddr->addrMode == afAddrBroadcast )
        {
          return afStatus_INVALID_PARAMETER;
        }
      }
    }
    else if ( dstAddr->addrMode != afAddrGroup )
    {
      return afStatus_INVALID_PARAMETER;
    }
  }
  req.dstAddr.addrMode = dstAddr->addrMode;
  req.profileID = ZDO_PROFILE_ID;

  if ( (pfnDescCB = afGetDescCB( srcEP )) )
  {
    uint16 *pID = (uint16 *)(pfnDescCB(
                                 AF_DESCRIPTOR_PROFILE_ID, srcEP->endPoint ));
    if ( pID )
    {
      req.profileID = *pID;
      osal_mem_free( pID );
    }
  }
  else if ( srcEP->simpleDesc )
  {
    req.profileID = srcEP->simpleDesc->AppProfId;
  }

  req.txOptions = 0;

  if ( ( options & AF_ACK_REQUEST              ) &&
       ( req.dstAddr.addrMode != AddrBroadcast ) &&
       ( req.dstAddr.addrMode != AddrGroup     )    )
  {
    req.txOptions |=  APS_TX_OPTIONS_ACK;
  }

  if ( options & AF_SKIP_ROUTING )
  {
    req.txOptions |=  APS_TX_OPTIONS_SKIP_ROUTING;
  }

  if ( options & AF_EN_SECURITY )
  {
    req.txOptions |= APS_TX_OPTIONS_SECURITY_ENABLE;
    mtu.aps.secure = TRUE;
  }
  else
  {
    mtu.aps.secure = FALSE;
  }

  mtu.kvp = FALSE;

  req.transID       = *transID;
  req.srcEP         = srcEP->endPoint;
  req.dstEP         = dstAddr->endPoint;
  req.clusterID     = cID;
  req.asduLen       = len;
  req.asdu          = buf;
  req.discoverRoute = (uint8)((options & AF_DISCV_ROUTE) ? 1 : 0);
  req.radiusCounter = radius;

  if (len > afDataReqMTU( &mtu ) )
  {
    if (apsfSendFragmented)
    {
      req.txOptions |= AF_FRAGMENTED | APS_TX_OPTIONS_ACK;
      stat = (*apsfSendFragmented)( &req );
    }
    else
    {
      stat = afStatus_INVALID_PARAMETER;
    }
  } 
  else
  {
    stat = APSDE_DataReq( &req );
  }

  /*
   * If this is an EndPoint-to-EndPoint message on the same device, it will not
   * get added to the NWK databufs. So it will not go OTA and it will not get
   * a MACCB_DATA_CONFIRM_CMD callback. Thus it is necessary to generate the
   * AF_DATA_CONFIRM_CMD here. Note that APSDE_DataConfirm() only generates one
   * message with the first in line TransSeqNumber, even on a multi message.
   */
  if ( (req.dstAddr.addrMode == Addr16Bit) &&
       (req.dstAddr.addr.shortAddr == NLME_GetShortAddr()) )
  {
    afDataConfirm( srcEP->endPoint, *transID, stat );
  }

  if ( stat == afStatus_SUCCESS )
  {
    (*transID)++;
  }

  return (afStatus_t)stat;
}

/*********************************************************************
 * @fn      afFindEndPointDescList
 *
 * @brief   Find the endpoint description entry from the endpoint
 *          number.
 *
 * @param   EndPoint - Application Endpoint to look for
 *
 * @return  the address to the endpoint/interface description entry
 */
static epList_t *afFindEndPointDescList( byte EndPoint )
{
  epList_t *epSearch;

  // Start at the beginning
  epSearch = epList;

  // Look through the list until the end
  while ( epSearch )
  {
    // Is there a match?
    if ( epSearch->epDesc->endPoint == EndPoint )
    {
      return ( epSearch );
    }
    else
      epSearch = epSearch->nextDesc;  // Next entry
  }

  return ( (epList_t *)NULL );
}

/*********************************************************************
 * @fn      afFindEndPointDesc
 *
 * @brief   Find the endpoint description entry from the endpoint
 *          number.
 *
 * @param   EndPoint - Application Endpoint to look for
 *
 * @return  the address to the endpoint/interface description entry
 */
endPointDesc_t *afFindEndPointDesc( byte EndPoint )
{
  epList_t *epSearch;

  // Look for the endpoint
  epSearch = afFindEndPointDescList( EndPoint );

  if ( epSearch )
    return ( epSearch->epDesc );
  else
    return ( (endPointDesc_t *)NULL );
}

/*********************************************************************
 * @fn      afFindSimpleDesc
 *
 * @brief   Find the Simple Descriptor from the endpoint number.
 *
 * @param   EP - Application Endpoint to look for.
 *
 * @return  Non-zero to indicate that the descriptor memory must be freed.
 */
byte afFindSimpleDesc( SimpleDescriptionFormat_t **ppDesc, byte EP )
{
  epList_t *epItem = afFindEndPointDescList( EP );
  byte rtrn = FALSE;

  if ( epItem )
  {
    if ( epItem->pfnDescCB )
    {
      *ppDesc = epItem->pfnDescCB( AF_DESCRIPTOR_SIMPLE, EP );
      rtrn = TRUE;
    }
    else
    {
      *ppDesc = epItem->epDesc->simpleDesc;
    }
  }
  else
  {
    *ppDesc = NULL;
  }

  return rtrn;
}

/*********************************************************************
 * @fn      afGetDescCB
 *
 * @brief   Get the Descriptor callback function.
 *
 * @param   epDesc - pointer to the endpoint descriptor
 *
 * @return  function pointer or NULL
 */
static pDescCB afGetDescCB( endPointDesc_t *epDesc )
{
  epList_t *epSearch;

  // Start at the beginning
  epSearch = epList;

  // Look through the list until the end
  while ( epSearch )
  {
    // Is there a match?
    if ( epSearch->epDesc == epDesc )
    {
      return ( epSearch->pfnDescCB );
    }
    else
      epSearch = epSearch->nextDesc;  // Next entry
  }

  return ( (pDescCB)NULL );
}

/*********************************************************************
 * @fn      afGetReflector
 *
 * @brief   Get the Reflector's address for an endpoint.
 *
 * @param   EndPoint - Application Endpoint to look for
 *
 * @return  shortAddr of the reflector for the endpoint
 *              0xFFFF if not found.
 */
uint16 afGetReflector( byte EndPoint )
{
  epList_t *epSearch;

  // Look for the endpoint
  epSearch = afFindEndPointDescList( EndPoint );

  if ( epSearch )
  {
    //-------------------------------------------------------------------------
    #if !defined ( REFLECTOR )
    //-------------------------------------------------------------------------
    return ( epSearch->reflectorAddr );
    //-------------------------------------------------------------------------
    #else
    //-------------------------------------------------------------------------
    if ( epSearch->reflectorAddr == INVALID_NODE_ADDR )
    {
      return ( NLME_GetShortAddr() );
    }
    else
    {
      return ( epSearch->reflectorAddr );
    }
    //-------------------------------------------------------------------------
    #endif
    //-------------------------------------------------------------------------
  }
  else
  {
    return ( INVALID_NODE_ADDR );
  }
}

/*********************************************************************
 * @fn      afSetReflector
 *
 * @brief   Set the reflector address for the endpoint.
 *
 * @param   EndPoint - Application Endpoint to look for
 * @param   reflectorAddr - new address
 *
 * @return  TRUE if success, FALSE if endpoint not fount
 */
uint8 afSetReflector( byte EndPoint, uint16 reflectorAddr )
{
  epList_t *epSearch;

  // Look for the endpoint
  epSearch = afFindEndPointDescList( EndPoint );

  if ( epSearch )
  {
    epSearch->reflectorAddr = reflectorAddr;
    return ( TRUE );
  }
  else
    return ( FALSE );
}

/*********************************************************************
 * @fn      afDataReqMTU
 *
 * @brief   Get the Data Request MTU(Max Transport Unit).
 *
 * @param   fields - afDataReqMTU_t
 *
 * @return  uint8(MTU)
 */
uint8 afDataReqMTU( afDataReqMTU_t* fields )
{
  uint8 len;
  uint8 hdr;
  uint8 version;

  if ( fields->kvp == TRUE )
  {
    hdr = AF_HDR_KVP_MAX_LEN;
  }
  else
  {
    version = NLME_GetProtocolVersion();

    if ( version == ZB_PROT_V1_0 )
    {
      hdr = AF_HDR_V1_0_MAX_LEN;
    }
    else
    {
      hdr = AF_HDR_V1_1_MAX_LEN;
    }
  }

  len = (uint8)(APSDE_DataReqMTU(&fields->aps) - hdr);

  return len;
}

/*********************************************************************
 * @fn      afGetMatch
 *
 * @brief   Set the allow response flag.
 *
 * @param   ep - Application Endpoint to look for
 * @param   action - true - allow response, false - no response
 *
 * @return  TRUE allow responses, FALSE no response
 */
uint8 afGetMatch( uint8 ep )
{
  epList_t *epSearch;

  // Look for the endpoint
  epSearch = afFindEndPointDescList( ep );

  if ( epSearch )
  {
    if ( epSearch->flags & eEP_AllowMatch )
      return ( TRUE );
    else
      return ( FALSE );
  }
  else
    return ( FALSE );
}

/*********************************************************************
 * @fn      afSetMatch
 *
 * @brief   Set the allow response flag.
 *
 * @param   ep - Application Endpoint to look for
 * @param   action - true - allow response, false - no response
 *
 * @return  TRUE if success, FALSE if endpoint not found
 */
uint8 afSetMatch( uint8 ep, uint8 action )
{
  epList_t *epSearch;

  // Look for the endpoint
  epSearch = afFindEndPointDescList( ep );

  if ( epSearch )
  {
    if ( action )
    {
      epSearch->flags |= eEP_AllowMatch;
    }
    else
    {
      epSearch->flags &= ~eEP_AllowMatch;
    }
    return ( TRUE );
  }
  else
    return ( FALSE );
}

/*********************************************************************
 * @fn      afNumEndPoints
 *
 * @brief   Returns the number of endpoints defined (including 0)
 *
 * @param   none
 *
 * @return  number of endpoints
 */
byte afNumEndPoints( void )
{
  epList_t *epSearch;
  byte endpoints;

  // Start at the beginning
  epSearch = epList;
  endpoints = 0;

  while ( epSearch )
  {
    endpoints++;
    epSearch = epSearch->nextDesc;
  }

  return ( endpoints );
}

/*********************************************************************
 * @fn      afEndPoints
 *
 * @brief   Fills in the passed in buffer with the endpoint (numbers).
 *          Use afNumEndPoints to find out how big a buffer to supply.
 *
 * @param   epBuf - pointer to mem used
 *
 * @return  void
 */
void afEndPoints( byte *epBuf, byte skipZDO )
{
  epList_t *epSearch;
  byte endPoint;

  // Start at the beginning
  epSearch = epList;

  while ( epSearch )
  {
    endPoint = epSearch->epDesc->endPoint;

    if ( !skipZDO || endPoint != 0 )
      *epBuf++ = endPoint;

    epSearch = epSearch->nextDesc;
  }
}

/*********************************************************************
 * Semi-Precision fuctions
 */

#if ( AF_FLOAT_SUPPORT )
/*********************************************************************
 * @fn      afCnvtSP_uint16
 *
 * @brief   Converts uint16 to semi-precision structure format
 *
 * @param   sp - semi-precision structure format
 *
 * @return  16 bit value for semiprecision.
 */
uint16 afCnvtSP_uint16( afSemiPrecision_t sp )
{
  return ( ((sp.sign & 0x0001) << 15)
              | ((sp.exponent & 0x001F) << 10)
              | (sp.mantissa & 0x03FF) );
}

/*********************************************************************
 * @fn      afCnvtuint16_SP
 *
 * @brief   Converts uint16 to semi-precision structure format
 *
 * @param   rawSP - Raw representation of SemiPrecision
 *
 * @return  SemiPrecision conversion.
 */
afSemiPrecision_t afCnvtuint16_SP( uint16 rawSP )
{
  afSemiPrecision_t sp = {0,0,0};

  sp.sign = ((rawSP >> 15) & 0x0001);
  sp.exponent = ((rawSP >> 10) & 0x001F);
  sp.mantissa = (rawSP & 0x03FF);
  return ( sp );
}

/*********************************************************************
 * @fn      afCnvtFloat_SP
 *
 * @brief   Converts float to semi-precision structure format
 *
 * @param   f - float value to convert from
 *
 * NOTE: This function will convert to the closest possible
 *       representation in a 16 bit format.  Meaning that
 *       the number may not be exact.  For example, 10.7 will
 *       convert to 10.69531, because .7 is a repeating binary
 *       number.  The mantissa for afSemiPrecision_t is 10 bits
 *       and .69531 is the 10 bit representative of .7.
 *
 * @return  SemiPrecision conversion.
 */
afSemiPrecision_t afCnvtFloat_SP( float f )
{
  afSemiPrecision_t sp = {0,0,0};
  unsigned long mantissa;
  unsigned int oldexp;
  int tempexp;
  float calcMant;
  unsigned long *uf;

  if ( f < 0 )
  {
    sp.sign = 1;
    f = f * -1;
  }
  else
    sp.sign = 0;

  if ( f == 0 )
  {
    sp.exponent = (unsigned int)0;
    sp.mantissa = (unsigned int)0;
  }
  else
  {
    uf = (void*)&f;

    mantissa = *uf & 0x7fffff;
    oldexp = (unsigned int)((*uf >> 23) & 0xff);
    tempexp = oldexp - 127;

    calcMant = (float)((float)(mantissa) / (float)(0x800000));
    mantissa = (unsigned long)(calcMant * 1024);

    sp.exponent = (unsigned int)(tempexp + 15);
    sp.mantissa = (unsigned int)(mantissa);
  }

  return ( sp );
}

/*********************************************************************
 * @fn      afCnvtSP_Float
 *
 * @brief   Converts semi-precision structure format to float
 *
 * @param   sp - afSemiPrecision format to convert from
 *
 * @return  float
 */
float afCnvtSP_Float( afSemiPrecision_t sp )
{
  float a, b, c;

  if ( sp.exponent == 0 && sp.mantissa == 0 )
  {
    a = 0;
    b = 0;
  }
  else
  {
    a = (float)((float)1 + (float)((float)(sp.mantissa)/1024));

#if defined( __MWERKS__ )
    b = powf( 2.0, (float)((float)sp.exponent - 15.0) );
#else
    b = (float)pow( 2.0, sp.exponent - 15 );
#endif
  }

  if ( sp.sign )
    c = a * b * -1;
  else
    c = a * b;

  return ( c );
}
#endif

#if ( AF_KVP_SUPPORT )
/*********************************************************************
 * @fn      GetDataTypeLength
 *
 * @brief   Return the length of the datatype in length
 *
 * @param   DataType
 *
 * @return  byte
 */
byte GetDataTypeLength( byte DataType )
{
  switch (DataType)
  {
    case DATATYPE_INT8:
    case DATATYPE_UINT8:
      return 1;

    case DATATYPE_INT16:
    case DATATYPE_UINT16:
    case DATATYPE_SEMIPREC:
      return 2;

    case DATATYPE_ABS_TIME:
    case DATATYPE_REL_TIME:
      return 4;

    default:
      return 0;
  }
}
#endif

void afCopyAddress ( afAddrType_t *afAddr, zAddrType_t *zAddr )
{
  afAddr->addrMode = (afAddrMode_t)zAddr->addrMode;
  afAddr->addr.shortAddr = zAddr->addr.shortAddr;
}

/*********************************************************************
*********************************************************************/

