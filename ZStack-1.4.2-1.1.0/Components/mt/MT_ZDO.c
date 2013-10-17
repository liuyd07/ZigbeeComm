/*********************************************************************
    Filename:       MT_ZDO.c
    Revised:        $Date: 2007-05-14 17:34:18 -0700 (Mon, 14 May 2007) $
    Revision:       $Revision: 14296 $

    Description:

        MonitorTest functions for the NWK layer.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

#ifdef MT_ZDO_FUNC


/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "MTEL.h"
#include "MT_ZDO.h"
#include "APSMEDE.h"
#include "ZDConfig.h"
#include "ZDProfile.h"
#include "ZDObject.h"
#include "ZDApp.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

#include "nwk_util.h"

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
uint32 _zdoCallbackSub;

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
byte *zdo_MT_MakeExtAddr( zAddrType_t *devAddr, byte *pData );
byte *zdo_MT_CopyRevExtAddr( byte *dstMsg, byte *addr );

/*********************************************************************
 * @fn      MT_ZdoCommandProcessing
 *
 * @brief
 *
 *   Process all the ZDO commands that are issued by test tool
 *
 * @param   cmd_id - Command ID
 * @param   len    - Length of received SPI data message
 * @param   pData  - pointer to received SPI data message
 *
 * @return  void
 */
void MT_ZdoCommandProcessing( uint16 cmd_id , byte len , byte *pData )
{
  byte i;
  byte x;
  byte ret;
  byte attr;
  byte attr1;
  uint16 cID;
  uint16 shortAddr;
  uint16 uAttr;
  byte *ptr;
  byte *ptr1;
  zAddrType_t devAddr;
  zAddrType_t dstAddr;
  byte respLen;
#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
  uint32 scanChans;
#endif
#if defined ( ZDO_USERDESCSET_REQUEST )
  UserDescriptorFormat_t userDesc;
#endif

  ret = UNSUPPORTED_COMMAND;
  len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_ZDO_DEFAULT;
  respLen = SPI_RESP_LEN_ZDO_DEFAULT;

  switch (cmd_id)
  {
    case SPI_CMD_ZDO_AUTO_ENDDEVICEBIND_REQ:
      i = *pData;    // Get the endpoint/interface
      ZDApp_SendEndDeviceBindReq( i );

      //Since function type is void, report a succesful operation to the test tool
      ret = ZSUCCESS;
      break;

    case SPI_CMD_ZDO_AUTO_FIND_DESTINATION_REQ:
      i = *pData;    // Get the endpoint/interface
      ZDApp_AutoFindDestination( i );
      //Since function type is void, report a succesful operation to the test tool
      ret = ZSUCCESS;
      break;

#if defined ( ZDO_NWKADDR_REQUEST )
    case SPI_CMD_ZDO_NWK_ADDR_REQ:
      // Copy and flip incoming 64-bit address
      pData = zdo_MT_MakeExtAddr( &devAddr, pData );

      ptr = (byte*)&devAddr.addr.extAddr;

      attr = *pData++;   // RequestType
      attr1 = *pData++;  // StartIndex
      x = *pData;
      ret = (byte)ZDP_NwkAddrReq( ptr, attr, attr1, x );
      break;
#endif

#if defined ( ZDO_IEEEADDR_REQUEST )
    case SPI_CMD_ZDO_IEEE_ADDR_REQ:
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += sizeof( shortAddr );
      attr = *pData++;   // RequestType
      attr1 = *pData++;  // StartIndex
      x = *pData;        // SecuritySuite
      ret = (byte)ZDP_IEEEAddrReq( shortAddr, attr, attr1, x );
      break;
#endif

#if defined ( ZDO_NODEDESC_REQUEST )
    case SPI_CMD_ZDO_NODE_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;
      ret = (byte)ZDP_NodeDescReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_POWERDESC_REQUEST )
    case SPI_CMD_ZDO_POWER_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;
      ret = (byte)ZDP_PowerDescReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_SIMPLEDESC_REQUEST )
    case SPI_CMD_ZDO_SIMPLE_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData++;  // endpoint/interface
      attr1 = *pData;   // SecuritySuite
      ret = (byte)ZDP_SimpleDescReq( &devAddr, shortAddr, attr, attr1 );
      break;
#endif

#if defined ( ZDO_ACTIVEEP_REQUEST )
    case SPI_CMD_ZDO_ACTIVE_EPINT_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;  // SecuritySuite
      ret = (byte)ZDP_ActiveEPReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_MATCH_REQUEST )
    case SPI_CMD_ZDO_MATCH_DESC_REQ:
      {
        uint16 inC[16], outC[16];

        // destination address
        devAddr.addrMode = Addr16Bit;
        devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
        pData += 2;

        // Network address of interest
        shortAddr = BUILD_UINT16( pData[1], pData[0] );
        pData += 2;

        uAttr = BUILD_UINT16( pData[1], pData[0] );   // Profile ID
        pData += 2;

        attr = *pData++;   // NumInClusters
        for (i=0; i<16; ++i)  {
          inC[i] = BUILD_UINT16(pData[1], pData[0]);
          pData += 2;
        }

        attr1 = *pData++;  // NumOutClusters
        for (i=0; i<16; ++i)  {
          outC[i] = BUILD_UINT16(pData[1], pData[0]);
          pData += 2;
        }

        i = *pData;        // SecuritySuite

        ret = (byte)ZDP_MatchDescReq( &devAddr, shortAddr, uAttr,
                                  attr, inC, attr1, outC, i );
      }
      break;
#endif

#if defined ( ZDO_COMPLEXDESC_REQUEST )
    case SPI_CMD_ZDO_COMPLEX_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;  // SecuritySuite
      ret = (byte)ZDP_ComplexDescReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_USERDESC_REQUEST )
    case SPI_CMD_ZDO_USER_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;  // SecuritySuite
      ret = (byte)ZDP_UserDescReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_ENDDEVICEBIND_REQUEST )
    case SPI_CMD_ZDO_END_DEV_BIND_REQ:
      //TODO: When ZTool supports 16 bits the code below will need to take it into account
      {
        uint16 inC[16], outC[16];

        // destination address
        devAddr.addrMode = Addr16Bit;
        devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
        pData += 2;

        // Network address of interest
        shortAddr = BUILD_UINT16( pData[1], pData[0] );
        pData += 2;

        x = *pData++;      // EPInt

        uAttr = BUILD_UINT16( pData[1], pData[0] );   // Profile ID
        pData += 2;

        attr = *pData++;   // NumInClusters
        for (i=0; i<16; ++i)  {
          inC[i] = BUILD_UINT16(pData[1], pData[0]);
          pData += 2;
        }

        attr1 = *pData++;  // NumOutClusters
        for (i=0; i<16; ++i)  {
          outC[i] = BUILD_UINT16(pData[1], pData[0]);
          pData += 2;
        }

        i = *pData;        // SecuritySuite

        ret = (byte)ZDP_EndDeviceBindReq( &devAddr, shortAddr, x, uAttr,
                                attr, inC, attr1, outC, i );
      }
      break;
#endif

#if defined ( ZDO_BIND_UNBIND_REQUEST )
    case SPI_CMD_ZDO_BIND_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      MT_ReverseBytes( pData, Z_EXTADDR_LEN );
      ptr = pData;       // SrcAddress
      pData += Z_EXTADDR_LEN;

      attr = *pData++;   // SrcEPInt

      cID = BUILD_UINT16( pData[1], pData[0]);      // ClusterID
      pData += 2;
      
      dstAddr.addrMode = *pData++;
      if ( NLME_GetProtocolVersion() == ZB_PROT_V1_0 )
        dstAddr.addrMode = Addr64Bit;
      
      MT_ReverseBytes( pData, Z_EXTADDR_LEN );
      if ( dstAddr.addrMode == Addr64Bit )
      {
        ptr1 = pData;      // DstAddress
        osal_cpyExtAddr( dstAddr.addr.extAddr, ptr1 );
      }
      else
      {
        dstAddr.addr.shortAddr = BUILD_UINT16( pData[0], pData[1] ); 
      }
      
      // The short address occupies lsb two bytes
      pData += Z_EXTADDR_LEN;

      
      attr1 = *pData++;  // DstEPInt

      x = *pData;        // SecuritySuite
     
#if defined ( REFLECTOR )
      if ( devAddr.addr.shortAddr == _NIB.nwkDevAddress )
      {
	ZDApp_BindReqCB( 0, &devAddr, ptr, attr, cID, &dstAddr, attr1, x );
        ret = ZSuccess;
      }
      else
#endif
      ret = (byte)ZDP_BindReq( &devAddr, ptr, attr, cID, &dstAddr, attr1, x );
      break;
#endif

#if defined ( ZDO_BIND_UNBIND_REQUEST )
    case SPI_CMD_ZDO_UNBIND_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      MT_ReverseBytes( pData, Z_EXTADDR_LEN );
      ptr = pData;       // SrcAddress
      pData += Z_EXTADDR_LEN;

      attr = *pData++;   // SrcEPInt

      cID = BUILD_UINT16( pData[1], pData[0]);      // ClusterID
      pData += 2;

      dstAddr.addrMode = *pData++;
      if ( NLME_GetProtocolVersion() == ZB_PROT_V1_0 )
        dstAddr.addrMode = Addr64Bit;
      MT_ReverseBytes( pData, Z_EXTADDR_LEN );
      if ( dstAddr.addrMode == Addr64Bit )
      {
        ptr1 = pData;      // DstAddress
        osal_cpyExtAddr( dstAddr.addr.extAddr, ptr1 );
      }
      else
      {
        dstAddr.addr.shortAddr = BUILD_UINT16( pData[0], pData[1] ); 
      }      
      pData += Z_EXTADDR_LEN;

      attr1 = *pData++;  // DstEPInt

      x = *pData;        // SecuritySuite

#if defined ( REFLECTOR )
      if ( devAddr.addr.shortAddr == _NIB.nwkDevAddress )
      {
        ZDApp_UnbindReqCB( 0, &devAddr, ptr, attr, cID, &dstAddr, attr1, x );
        ret = ZSuccess;
      }
      else
#endif
      {
        ret = (byte)ZDP_UnbindReq( &devAddr, ptr, attr, cID, &dstAddr, attr1, x );
      }
      break;
#endif

#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
    case SPI_CMD_ZDO_MGMT_NWKDISC_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;
      scanChans = BUILD_UINT32( pData[3], pData[2], pData[1], pData[0] );
      ret = (byte)ZDP_MgmtNwkDiscReq( &devAddr, scanChans, pData[4], pData[5], false );
      break;
#endif

#if defined ( ZDO_MGMT_LQI_REQUEST )
    case SPI_CMD_ZDO_MGMT_LQI_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      ret = (byte)ZDP_MgmtLqiReq( &devAddr, pData[2], false );
      break;
#endif

#if defined ( ZDO_MGMT_RTG_REQUEST )
    case SPI_CMD_ZDO_MGMT_RTG_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      ret = (byte)ZDP_MgmtRtgReq( &devAddr, pData[2], false );
      break;
#endif

#if defined ( ZDO_MGMT_BIND_REQUEST )
    case SPI_CMD_ZDO_MGMT_BIND_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      ret = (byte)ZDP_MgmtBindReq( &devAddr, pData[2], false );
      break;
#endif

#if defined ( ZDO_MGMT_JOINDIRECT_REQUEST )
    case SPI_CMD_ZDO_MGMT_DIRECT_JOIN_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      MT_ReverseBytes( &pData[2], Z_EXTADDR_LEN );
      ret = (byte)ZDP_MgmtDirectJoinReq( &devAddr,
                               &pData[2],
                               pData[2 + Z_EXTADDR_LEN],
                               false );
      break;
#endif

#if defined ( ZDO_MGMT_LEAVE_REQUEST )
    case SPI_CMD_ZDO_MGMT_LEAVE_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      MT_ReverseBytes( &pData[2], Z_EXTADDR_LEN );
      ret = (byte)ZDP_MgmtLeaveReq( &devAddr, &pData[2], false );
      break;
#endif

#if defined ( ZDO_MGMT_PERMIT_JOIN_REQUEST )
    case SPI_CMD_ZDO_MGMT_PERMIT_JOIN_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      ret = (byte)ZDP_MgmtPermitJoinReq( &devAddr, pData[2], pData[3], false );
      break;
#endif


#if defined ( ZDO_USERDESCSET_REQUEST )
    case SPI_CMD_ZDO_USER_DESC_SET:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // User descriptor
      userDesc.len = *pData++;
      osal_memcpy( userDesc.desc, pData, userDesc.len );
      pData += 16;  // len of user desc

      ret =(byte)ZDP_UserDescSet( &devAddr, shortAddr, &userDesc, pData[0] );
      break;
#endif

#if defined ( ZDO_ENDDEVICE_ANNCE_REQUEST )
    case SPI_CMD_ZDO_END_DEV_ANNCE:
      // network address
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // extended address
      ptr = pData;
      MT_ReverseBytes( ptr, Z_EXTADDR_LEN );
      pData += Z_EXTADDR_LEN;

      // security
      attr = *pData++;

      ret = (byte)ZDP_EndDeviceAnnce( shortAddr, ptr, *pData, attr );
      break;
#endif

#if defined (ZDO_SERVERDISC_REQUEST )
    case SPI_CMD_ZDO_SERVERDISC_REQ:
      
      // Service Mask
      uAttr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;
      attr = *pData++; // Security suite
      
      ret = (byte) ZDP_ServerDiscReq( uAttr, attr );
      break;
#endif
      
#if defined (ZDO_NETWORKSTART_REQUEST )
    case SPI_CMD_ZDO_NETWORK_START_REQ:
      ret = ZDApp_StartUpFromApp( ZDAPP_STARTUP_AUTO );
      break;
    
#endif
    
    default:
      break;
  }

  MT_SendSPIRespMsg( ret, cmd_id, len, respLen );
}

/*********************************************************************
 * Utility FUNCTIONS
 */

/*********************************************************************
 * @fn      zdo_MT_CopyRevExtAddr
 *
 */
byte *zdo_MT_CopyRevExtAddr( byte *dstMsg, byte *addr )
{
  // Copy the 64-bit address
  osal_cpyExtAddr( dstMsg, addr );
  // Reverse byte order
  MT_ReverseBytes( dstMsg, Z_EXTADDR_LEN );
  // Return ptr to next destination location
  return ( dstMsg + Z_EXTADDR_LEN );
}

/*********************************************************************
 * @fn      zdo_MT_MakeExtAddr
 *
 */
byte *zdo_MT_MakeExtAddr( zAddrType_t *devAddr, byte *pData )
{
  // Define a 64-bit address
  devAddr->addrMode = Addr64Bit;
  // Copy and reverse the 64-bit address
  zdo_MT_CopyRevExtAddr( devAddr->addr.extAddr, pData );
  // Return ptr to next destination location
  return ( pData + Z_EXTADDR_LEN );
}

/*********************************************************************
 * CALLBACK FUNCTIONS
 */

#if defined ( ZDO_NWKADDR_REQUEST ) || defined ( ZDO_IEEEADDR_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_NwkIEEEAddrRspCB
 *
 * @brief
 *
 *   Called by ZDO when a NWK_addr_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  IEEEAddr - 64 bit IEEE address of device
 * @param  nwkAddr - 16 bit network address of device
 * @param  NumAssocDev - number of associated devices to reporting device
 * @param  AssocDevList - array short addresses of associated devices
 *
 * @return  none
 */
void zdo_MTCB_NwkIEEEAddrRspCB( uint16 type, zAddrType_t *SrcAddr, byte Status,
                               byte *IEEEAddr, uint16 nwkAddr, byte NumAssocDev,
                               byte StartIndex, uint16 *AssocDevList )
{
  byte *pBuf;
  byte *msg;
  byte len;
  byte x;

  /*Allocate a message of size equivalent to the corresponding SPI message
  (plus a couple of bytes for MT use)so that the same buffer can be sent by
  MT to the test tool by simply setting the header bytes.*/

  /*In order to allocate the message , we need to know the length and this
  has to be calculated before we allocate the message*/

  if ( type == SPI_CB_ZDO_NWK_ADDR_RSP )
  {
    len = 1 + Z_EXTADDR_LEN +  1 + Z_EXTADDR_LEN + 2 + 1 + 1 + (2*8);
      // Addrmode + SrcAddr + Status + IEEEAddr + nwkAddr + NumAssocDev + StartIndex
  }
  else
  {
    len = 1 + Z_EXTADDR_LEN +  1 + Z_EXTADDR_LEN + 1 + 1 + (2*8);
      // Addrmode + SrcAddr + Status + IEEEAddr + NumAssocDev + StartIndex
  }

  pBuf = osal_mem_alloc( len );

  if ( pBuf )
  {
    msg = pBuf;

    //First fill in details
    if ( SrcAddr->addrMode == Addr16Bit )
    {
      *msg++ = Addr16Bit;
      for ( x = 0; x < (Z_EXTADDR_LEN - 2); x++ )
        *msg++ = 0;
      *msg++ = HI_UINT16( SrcAddr->addr.shortAddr );
      *msg++ = LO_UINT16( SrcAddr->addr.shortAddr );
    }
    else
    {
      *msg++ = Addr64Bit;
      msg = zdo_MT_CopyRevExtAddr( msg, SrcAddr->addr.extAddr );
    }

    *msg++ = Status;
    msg = zdo_MT_CopyRevExtAddr( msg, IEEEAddr );

    if ( type == SPI_CB_ZDO_NWK_ADDR_RSP )
    {
      *msg++ = HI_UINT16( nwkAddr );
      *msg++ = LO_UINT16( nwkAddr );
    }

    *msg++ = NumAssocDev;
    *msg++ = StartIndex;
    byte cnt = NumAssocDev - StartIndex;

    for ( x = 0; x < 8; x++ )
    {
      if ( x < cnt )
      {
        *msg++ = HI_UINT16( *AssocDevList );
        *msg++ = LO_UINT16( *AssocDevList );
        AssocDevList++;
      }
      else
      {
        *msg++ = 0;
        *msg++ = 0;
      }
    }

    MT_BuildAndSendZToolCB( type, len, pBuf );

    osal_mem_free( pBuf );
  }
}
#endif // ZDO_NWKADDR_REQUEST || ZDO_IEEEADDR_REQUEST

#if defined ( ZDO_NODEDESC_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_NodeDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Node_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddr - 16 bit network address of device
 * @param  pNodeDesc - pointer to the devices Node Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
void zdo_MTCB_NodeDescRspCB( zAddrType_t *SrcAddr, byte Status, uint16 nwkAddr,
                             NodeDescriptorFormat_t *pNodeDesc )
{
  byte buf[18];
  byte *msg;

  msg = buf;

  //Fill up the data bytes
  *msg++ = Status;
  *msg++ = HI_UINT16( SrcAddr->addr.shortAddr );
  *msg++ = LO_UINT16( SrcAddr->addr.shortAddr );

  *msg++ = HI_UINT16( nwkAddr );
  *msg++ = LO_UINT16( nwkAddr );

  *msg++ = (byte)(pNodeDesc->LogicalType);

  // Since Z-Tool can't treat V1.0 and V1.1 differently,
  // we just output these two byte in both cases, although
  // in V1.0, they are always zeros.
  *msg++ = (byte) pNodeDesc->ComplexDescAvail;
  *msg++ = (byte) pNodeDesc->UserDescAvail;

  *msg++ = pNodeDesc->APSFlags;
  *msg++ = pNodeDesc->FrequencyBand;
  *msg++ = pNodeDesc->CapabilityFlags;
  *msg++ = pNodeDesc->ManufacturerCode[1];
  *msg++ = pNodeDesc->ManufacturerCode[0];
  *msg++ = pNodeDesc->MaxBufferSize;
  *msg++ = pNodeDesc->MaxTransferSize[1];
  *msg++ = pNodeDesc->MaxTransferSize[0];
  *msg++ = HI_UINT16( pNodeDesc->ServerMask);
  *msg++ = LO_UINT16( pNodeDesc->ServerMask);

  MT_BuildAndSendZToolCB( SPI_CB_ZDO_NODE_DESC_RSP, 18, buf );
}
#endif // ZDO_NODEDESC_REQUEST

#if defined ( ZDO_POWERDESC_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_PowerDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Power_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddr - 16 bit network address of device
 * @param  pPwrDesc - pointer to the devices Power Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
void zdo_MTCB_PowerDescRspCB( zAddrType_t *SrcAddr, byte Status,
          uint16 nwkAddr, NodePowerDescriptorFormat_t *pPwrDesc )
{
  byte buf[9];
  byte *msg;

  msg = buf;

  //Fill up the data bytes
  *msg++ = Status;
  *msg++ = HI_UINT16( SrcAddr->addr.shortAddr );
  *msg++ = LO_UINT16( SrcAddr->addr.shortAddr );
  *msg++ = HI_UINT16( nwkAddr );
  *msg++ = LO_UINT16( nwkAddr );

  *msg++ = pPwrDesc->PowerMode;
  *msg++ = pPwrDesc->AvailablePowerSources;
  *msg++ = pPwrDesc->CurrentPowerSource;
  *msg   = pPwrDesc->CurrentPowerSourceLevel;

  MT_BuildAndSendZToolCB( SPI_CB_ZDO_POWER_DESC_RSP, 9, buf );
}
#endif // ZDO_POWERDESC_REQUEST

#if defined ( ZDO_SIMPLEDESC_REQUEST )
#define ZDO_SIMPLE_DESC_CB_LEN  78
/*********************************************************************
 * @fn      zdo_MTCB_SimpleDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Simple_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddr - 16 bit network address of device
 * @param  EPIntf - Endpoint/Interface for description
 * @param  pSimpleDesc - pointer to the devices Simple Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
void zdo_MTCB_SimpleDescRspCB( zAddrType_t *SrcAddr, byte Status,
          uint16 nwkAddr, byte EPIntf, SimpleDescriptionFormat_t *pSimpleDesc )
{
  byte *msgPtr;
  byte *msg;
  byte x;

  msgPtr = osal_mem_alloc( ZDO_SIMPLE_DESC_CB_LEN );
  if ( msgPtr )
  {
    msg = msgPtr;

    //Fill up the data bytes
    *msg++ = Status;
    *msg++ = HI_UINT16( SrcAddr->addr.shortAddr );
    *msg++ = LO_UINT16( SrcAddr->addr.shortAddr );
    *msg++ = HI_UINT16( nwkAddr );
    *msg++ = LO_UINT16( nwkAddr );

    *msg++ = EPIntf;

    *msg++ = HI_UINT16( pSimpleDesc->AppProfId );
    *msg++ = LO_UINT16( pSimpleDesc->AppProfId );
    *msg++ = HI_UINT16( pSimpleDesc->AppDeviceId );
    *msg++ = LO_UINT16( pSimpleDesc->AppDeviceId );

    *msg++ = pSimpleDesc->AppDevVer;
    *msg++ = pSimpleDesc->Reserved;

    *msg++ = pSimpleDesc->AppNumInClusters;
    // ZTool supports 16 bits the code has taken it into account      
    for ( x = 0; x < 16; x++ )
    {
      if ( x < pSimpleDesc->AppNumInClusters )
      {
        *msg++ = HI_UINT16( pSimpleDesc->pAppInClusterList[x]);
        *msg++ = LO_UINT16( pSimpleDesc->pAppInClusterList[x]);
      }
      else
      {
        *msg++ = 0;
        *msg++ = 0;
      }
    }
    *msg++ = pSimpleDesc->AppNumOutClusters;

    for ( x = 0; x < 16; x++ )
    {
      if ( x < pSimpleDesc->AppNumOutClusters )
      {
        *msg++ = HI_UINT16( pSimpleDesc->pAppOutClusterList[x]);
        *msg++ = LO_UINT16( pSimpleDesc->pAppOutClusterList[x]);
      }
      else
      {
        *msg++ = 0;
        *msg++ = 0;
      }
    }

    MT_BuildAndSendZToolCB( SPI_CB_ZDO_SIMPLE_DESC_RSP, ZDO_SIMPLE_DESC_CB_LEN, msgPtr );

    osal_mem_free( msgPtr );
  }
}
#endif // ZDO_SIMPLEDESC_REQUEST

#if defined ( ZDO_ACTIVEEP_REQUEST ) || defined ( ZDO_MATCH_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_ActiveEPRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Active_EP_rsp or Match_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddr - Device's short address that this response describes
 * @param  epIntfCnt - number of epIntfList items
 * @param  epIntfList - array of active endpoint/interfaces.
 *
 * @return  none
 */
void zdo_MTCB_MatchActiveEPRspCB( uint16 type, zAddrType_t *SrcAddr, byte Status,
                  uint16 nwkAddr, byte epIntfCnt, byte *epIntfList )
{
  byte buf[22];
  byte *msg;
  byte x;

  msg = buf;

  //Fill up the data bytes
  *msg++ = Status;
  *msg++ = HI_UINT16( SrcAddr->addr.shortAddr );
  *msg++ = LO_UINT16( SrcAddr->addr.shortAddr );
  *msg++ = HI_UINT16( nwkAddr );
  *msg++ = LO_UINT16( nwkAddr );

  *msg++ = epIntfCnt;

  for ( x = 0; x < 16; x++ )
  {
    if ( x < epIntfCnt )
      *msg++ = *epIntfList++;
    else
      *msg++ = 0;
  }

  MT_BuildAndSendZToolCB( type, 22, buf );
}
#endif // ZDO_ACTIVEEP_REQUEST || ZDO_MATCH_REQUEST

#if defined ( ZDO_BIND_UNBIND_REQUEST ) || defined ( ZDO_ENDDEVICEBIND_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_BindRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for binding responses
 *
 * @param  type - binding type (end device, bind, unbind)
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void zdo_MTCB_BindRspCB( uint16 type, zAddrType_t *SrcAddr, byte Status )
{
  byte buf[3];
  buf[0] = Status;
  buf[1] = HI_UINT16( SrcAddr->addr.shortAddr );
  buf[2] = LO_UINT16( SrcAddr->addr.shortAddr );
  MT_BuildAndSendZToolCB( type, 3, buf );
}
#endif // ZDO_BIND_UNBIND_REQUEST || ZDO_ENDDEVICEBIND_REQUEST

#if defined ( ZDO_MGMT_LQI_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_MgmtLqiRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management LQI response
 *
 * @param  type - binding type (end device, bind, unbind)
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void zdo_MTCB_MgmtLqiRspCB( uint16 SrcAddr, byte Status, byte NeighborLqiEntries,
                            byte StartIndex, byte NeighborLqiCount,
                            neighborLqiItem_t *pList )
{
  byte *msgPtr;
  byte *msg;
  byte len;
  byte x;
  byte proVer = NLME_GetProtocolVersion();  
  
  /*Allocate a message of size equivalent to the corresponding SPI message
  (plus a couple of bytes for MT use)so that the same buffer can be sent by
  MT to the test tool by simply setting the header bytes.*/

  /*In order to allocate the message , we need to know the length and this
  has to be calculated before we allocate the message*/

  len = 2 + 1 + 1 + 1 + 1 + (ZDP_NEIGHBORLQI_SIZE * ZDO_MAX_LQI_ITEMS );
    //  SrcAddr + Status + NeighborLqiEntries + StartIndex + NeighborLqiCount
    //     + (maximum entries * size of struct)
 
  msgPtr = osal_mem_alloc( len );

  if ( msgPtr )
  {
    msg = msgPtr;

    //Fill up the data bytes
    
    *msg++ = HI_UINT16( SrcAddr );
    *msg++ = LO_UINT16( SrcAddr );    
    *msg++ = Status;
    *msg++ = NeighborLqiEntries;
    *msg++ = StartIndex;
    *msg++ = NeighborLqiCount;

    osal_memset( msg, 0, (ZDP_NEIGHBORLQI_SIZE * ZDO_MAX_LQI_ITEMS) );

    for ( x = 0; x < ZDO_MAX_LQI_ITEMS; x++ )
    {
      if ( x < NeighborLqiCount )
      {
        if ( proVer == ZB_PROT_V1_0 )
        {
          *msg++ = HI_UINT16( pList->PANId );
          *msg++ = LO_UINT16( pList->PANId );
        }
        else 
        {
          osal_cpyExtAddr(msg, pList->extPANId);
          msg += Z_EXTADDR_LEN;
        }
        *msg++ = HI_UINT16( pList->nwkAddr );
        *msg++ = LO_UINT16( pList->nwkAddr );
        *msg++ = pList->rxLqi;
        *msg++ = pList->txQuality;
        pList++;
      }
    }

    MT_BuildAndSendZToolCB( SPI_CB_ZDO_MGMT_LQI_RSP, len, msgPtr );

    osal_mem_free( msgPtr );
  }
}
#endif // ZDO_MGMT_LQI_REQUEST

#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_MgmtNwkDiscRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Network
 *   Discover response
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void zdo_MTCB_MgmtNwkDiscRspCB( uint16 SrcAddr, byte Status,
                        byte NetworkCount, byte StartIndex,
                        byte networkListCount, mgmtNwkDiscItem_t *pList )
{
  byte *msgPtr;
  byte *msg;
  byte len;
  byte x;
  byte proVer = NLME_GetProtocolVersion();  
   

  /*Allocate a message of size equivalent to the corresponding SPI message
  (plus a couple of bytes for MT use)so that the same buffer can be sent by
  MT to the test tool by simply setting the header bytes.*/

  /*In order to allocate the message , we need to know the length and this
  has to be calculated before we allocate the message*/
  if ( proVer == ZB_PROT_V1_0 )
  {
    len = 2 + 1 + 1 + 1 + 1 + (ZDP_NETWORK_DISCRIPTOR_SIZE * ZDO_MAX_NWKDISC_ITEMS);
      //  SrcAddr + Status + NetworkCount + StartIndex + networkListCount
      //     + (maximum entries * size of struct)
  }
  else
  {
    len = 2 + 1 + 1 + 1 + 1 + (ZDP_NETWORK_EXTENDED_DISCRIPTOR_SIZE * ZDO_MAX_NWKDISC_ITEMS);
 
  }

  msgPtr = osal_mem_alloc( len );

  if ( msgPtr )
  {
    msg = msgPtr;

    //Fill up the data bytes
    *msg++ = HI_UINT16( SrcAddr );
    *msg++ = LO_UINT16( SrcAddr );
    *msg++ = Status;
    *msg++ = NetworkCount;
    *msg++ = StartIndex;
    *msg++ = networkListCount;

    osal_memset( msg, 0, (ZDP_NETWORK_DISCRIPTOR_SIZE * ZDO_MAX_NWKDISC_ITEMS) );

    for ( x = 0; x < ZDO_MAX_NWKDISC_ITEMS; x++ )
    {
      if ( x < networkListCount )
      {
        if ( proVer == ZB_PROT_V1_0 )
        {
          *msg++ = HI_UINT16( pList->PANId );
          *msg++ = LO_UINT16( pList->PANId );
        }
        else
        {
          osal_cpyExtAddr( msg, pList->extendedPANID );
          msg += Z_EXTADDR_LEN;
        }
        *msg++ = pList->logicalChannel;
        *msg++ = pList->stackProfile;
        *msg++ = pList->version;
        *msg++ = pList->beaconOrder;
        *msg++ = pList->superFrameOrder;
        *msg++ = pList->permitJoining;
        pList++;
      }
    }

    MT_BuildAndSendZToolCB( SPI_CB_ZDO_MGMT_NWKDISC_RSP, len, msgPtr );

    osal_mem_free( msgPtr );
  }
}
#endif // ZDO_MGMT_NWKDISC_REQUEST

#if defined ( ZDO_MGMT_RTG_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_MgmtRtgRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Network
 *   Discover response
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void zdo_MTCB_MgmtRtgRspCB( uint16 SrcAddr, byte Status,
                        byte RtgCount, byte StartIndex,
                        byte RtgListCount, rtgItem_t *pList )
{
  byte *msgPtr;
  byte *msg;
  byte len;
  byte x;

  /*Allocate a message of size equivalent to the corresponding SPI message
  (plus a couple of bytes for MT use)so that the same buffer can be sent by
  MT to the test tool by simply setting the header bytes.*/

  /*In order to allocate the message , we need to know the length and this
  has to be calculated before we allocate the message*/

  len = 2 + 1 + 1 + 1 + 1 + (ZDP_RTG_DISCRIPTOR_SIZE * ZDO_MAX_RTG_ITEMS);
      //  SrcAddr + Status + RtgCount + StartIndex + RtgListCount
      //     + (maximum entries * size of struct)

  msgPtr = osal_mem_alloc( len );

  if ( msgPtr )
  {
    msg = msgPtr;

    //Fill up the data bytes
    *msg++ = HI_UINT16( SrcAddr );
    *msg++ = LO_UINT16( SrcAddr );
    *msg++ = Status;
    *msg++ = RtgCount;
    *msg++ = StartIndex;
    *msg++ = RtgListCount;

    osal_memset( msg, 0, (ZDP_RTG_DISCRIPTOR_SIZE * ZDO_MAX_RTG_ITEMS) );

    for ( x = 0; x < ZDO_MAX_RTG_ITEMS; x++ )
    {
      if ( x < RtgListCount )
      {
        *msg++ = HI_UINT16( pList->dstAddress );
        *msg++ = LO_UINT16( pList->dstAddress );
        *msg++ = HI_UINT16( pList->nextHopAddress );
        *msg++ = LO_UINT16( pList->nextHopAddress );
        *msg++ = pList->status;
        pList++;
      }
    }

    MT_BuildAndSendZToolCB( SPI_CB_ZDO_MGMT_RTG_RSP, len, msgPtr );

    osal_mem_free( msgPtr );
  }
}
#endif // ZDO_MGMT_RTG_REQUEST

#if defined ( ZDO_MGMT_BIND_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_MgmtBindRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Network
 *   Discover response
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void zdo_MTCB_MgmtBindRspCB( uint16 SrcAddr, byte Status,
                        byte BindCount, byte StartIndex,
                        byte BindListCount, apsBindingItem_t *pList )
{
  byte *msgPtr;
  byte *msg;
  byte len;       
  byte x;
  uint8 protoVer = NLME_GetProtocolVersion();
  
  /*Allocate a message of size equivalent to the corresponding SPI message
  (plus a couple of bytes for MT use)so that the same buffer can be sent by
  MT to the test tool by simply setting the header bytes.*/

  /*In order to allocate the message , we need to know the length and this
  has to be calculated before we allocate the message*/
  

  // One more byte for clusterID and DstAddrMode 
  len = 2 + 1 + 1 + 1 + 1 + ( ( ZDP_BIND_DISCRIPTOR_SIZE + 1 + 1 ) * ZDO_MAX_BIND_ITEMS);
      //  SrcAddr + Status + BindCount + StartIndex + BindListCount
      //     + (maximum entries * size of struct)

  msgPtr = osal_mem_alloc( len );

  if ( msgPtr )
  {
    msg = msgPtr;

    //Fill up the data bytes
    *msg++ = HI_UINT16( SrcAddr );
    *msg++ = LO_UINT16( SrcAddr );
    *msg++ = Status;
    *msg++ = BindCount;
    *msg++ = StartIndex;
    *msg++ = BindListCount;

    osal_memset( msg, 0, ( ( ZDP_BIND_DISCRIPTOR_SIZE + 1 + 1)  * ZDO_MAX_BIND_ITEMS) );
    
    
    for ( x = 0; x < ZDO_MAX_BIND_ITEMS; x++ )
    {
      if ( x < BindListCount )
      {
        msg = zdo_MT_CopyRevExtAddr( msg, pList->srcAddr );
        *msg++ = pList->srcEP;
        
        if ( protoVer == ZB_PROT_V1_0 ) 
        {         
          *msg++ = LO_UINT16( pList->clusterID);
          msg = zdo_MT_CopyRevExtAddr( msg, pList->dstAddr.addr.extAddr );    
          *msg++ = pList->dstEP;
        }
        else
        {
          *msg++ = HI_UINT16( pList->clusterID);
          *msg++ = LO_UINT16( pList->clusterID);
          *msg++ = pList->dstAddr.addrMode;
        
          if ( pList->dstAddr.addrMode == Addr64Bit )
          {         
            msg = zdo_MT_CopyRevExtAddr( msg, pList->dstAddr.addr.extAddr );
            *msg++ = pList->dstEP;           
          }
          else
          {
            *msg++ = HI_UINT16( pList->dstAddr.addr.shortAddr );
            *msg++ = LO_UINT16( pList->dstAddr.addr.shortAddr );
            // DstEndpoint will not present if DstAddrMode is not 64-bit extAddr
          }
        }
        
        pList++;
      }
    }

    MT_BuildAndSendZToolCB( SPI_CB_ZDO_MGMT_BIND_RSP, len, msgPtr );

    osal_mem_free( msgPtr );
  }
}
#endif // ZDO_MGMT_RTG_REQUEST

#if defined ( ZDO_MGMT_JOINDIRECT_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_MgmtDirectJoinRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Direct Join
 *   responses
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void zdo_MTCB_MgmtDirectJoinRspCB( uint16 SrcAddr, byte Status, byte SecurityUse )
{
  byte buf[3];

  buf[0] = HI_UINT16( SrcAddr );
  buf[1] = LO_UINT16( SrcAddr );
  buf[2] = Status;

  MT_BuildAndSendZToolCB( SPI_CB_ZDO_MGMT_DIRECT_JOIN_RSP, 3, buf );
}
#endif // ZDO_MGMT_JOINDIRECT_REQUEST

#if defined ( ZDO_MGMT_LEAVE_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_MgmtLeaveRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Leave
 *   responses
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void zdo_MTCB_MgmtLeaveRspCB( uint16 SrcAddr, byte Status, byte SecurityUse )
{
  byte buf[3];

  buf[0] = Status;
  buf[1] = HI_UINT16( SrcAddr );
  buf[2] = LO_UINT16( SrcAddr );

  MT_BuildAndSendZToolCB( SPI_CB_ZDO_MGMT_LEAVE_RSP, 3, buf );
}
#endif // ZDO_MGMT_LEAVE_REQUEST

#if defined ( ZDO_MGMT_PERMIT_JOIN_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_MgmtPermitJoinRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Permit Join
 *   responses
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void zdo_MTCB_MgmtPermitJoinRspCB( uint16 SrcAddr, byte Status, byte SecurityUse )
{
  byte buf[3];

  buf[0] = Status;
  buf[1] = HI_UINT16( SrcAddr );
  buf[2] = LO_UINT16( SrcAddr );

  MT_BuildAndSendZToolCB( SPI_CB_ZDO_MGMT_PERMIT_JOIN_RSP, 3, buf );
}
#endif // ZDO_MGMT_PERMIT_JOIN_REQUEST

#if defined ( ZDO_USERDESC_REQUEST )
#define USER_DESC_CB_LEN  22
/*********************************************************************
 * @fn      zdo_MTCB_UserDescRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for User Descriptor
 *   responses
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddrOfInterest -
 * @param  userDescLen -
 * @param  userDesc -
 * @param  SecurityUse -
 *
 * @return  none
 */
void zdo_MTCB_UserDescRspCB( uint16 SrcAddr, byte status, uint16 nwkAddrOfInterest,
                          byte userDescLen, byte *userDesc, byte SecurityUse )
{
  byte *msgPtr;
  byte *msg;
  msgPtr = osal_mem_alloc( USER_DESC_CB_LEN );
  osal_memset( msgPtr, 0, USER_DESC_CB_LEN );
  
  msg = msgPtr;
  *msg++ = status;
  *msg++ = HI_UINT16( SrcAddr );
  *msg++ = LO_UINT16( SrcAddr );
  *msg++ = HI_UINT16( nwkAddrOfInterest );
  *msg++ = LO_UINT16( nwkAddrOfInterest );
  *msg++ = userDescLen;
  osal_memcpy( msg, userDesc, userDescLen ); 
  MT_BuildAndSendZToolCB( SPI_CB_ZDO_USER_DESC_RSP, USER_DESC_CB_LEN, msgPtr );
  
  osal_mem_free( msgPtr );
}
#endif // ZDO_USERDESC_REQUEST

#if defined ( ZDO_USERDESCSET_REQUEST )
/*********************************************************************
 * @fn      zdo_MTCB_UserDescConfCB()
 *
 * @brief
 *
 *   Called to send MT callback response for User Descriptor
 *   confirm
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  SecurityUse -
 *
 * @return  none
 */
void zdo_MTCB_UserDescConfCB( uint16 SrcAddr, byte status, byte SecurityUse )
{
  byte buf[3];

  buf[0] = status;
  buf[1] = HI_UINT16( SrcAddr );
  buf[2] = LO_UINT16( SrcAddr );

  MT_BuildAndSendZToolCB( SPI_CB_ZDO_USER_DESC_CNF, 3, buf );
}
#endif // ZDO_USERDESCSET_REQUEST

#if defined ( ZDO_SERVERDISC_REQUEST )
/*********************************************************************
 * @fn     zdo_MTCB_ServerDiscRspCB()
 *
 * @brief  Called to send MT callback response for Server_Discovery_rsp responses.
 *
 * @param  srcAddr - Source address.
 * @param  status - Response status.
 * @param  aoi - Network Address of Interest.
 * @param  serverMask - Bit mask of services that match request.
 * @param  SecurityUse -
 *
 * @return  none
 */
void zdo_MTCB_ServerDiscRspCB( uint16 srcAddr, byte status, 
                               uint16 serverMask, byte SecurityUse )
{
  byte buf[5];
  byte *pBuf = buf;

  *pBuf++ = status;
  *pBuf++ = HI_UINT16( srcAddr );
  *pBuf++ = LO_UINT16( srcAddr );
  *pBuf++ = HI_UINT16( serverMask );
  *pBuf++ = LO_UINT16( serverMask );

  MT_BuildAndSendZToolCB( SPI_CB_ZDO_SERVERDISC_RSP, 5, buf );
#define CB_ID_ZDO_SERVERDISC_RSP             0x00080000
}
#endif

/*********************************************************************
*********************************************************************/

#endif   /*ZDO Command Processing in MT*/
