/*********************************************************************
    Filename:       MT_NWK.c
    Revised:        $Date: 2007-05-03 14:52:32 -0700 (Thu, 03 May 2007) $
    Revision:       $Revision: 14191 $

    Description:

        MonitorTest functions for the NWK layer.

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
#include "NLMEDE.h"
#include "nwk.h"
#include "ZDApp.h"
#include "nwk_globals.h"
#include "MT_NWK.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

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
uint16 _nwkCallbackSub;

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
#if defined ( MT_NWK_FUNC )  //NWK commands
uint8 MT_Nwk_DataRequest( uint16 dstAddr, uint8 nsduLen, uint8* nsdu,
                         uint8 nsduHandle, uint16 nsduHandleOptions,
                         uint8 secure, uint8 discoverRoute,
                         uint8 radius);

uint8 MT_Nwk_DataRequest( uint16 dstAddr, uint8 nsduLen, uint8* nsdu,
                         uint8 nsduHandle, uint16 nsduHandleOptions,
                         uint8 secure, uint8 discoverRoute,
                         uint8 radius)
{
    uint8               status;
    NLDE_DataReqAlloc_t dra;
    NLDE_DataReq_t*     req;


    dra.overhead = sizeof(NLDE_DataReq_t);
    dra.nsduLen  = nsduLen;
    dra.secure   = secure;

    req = NLDE_DataReqAlloc(&dra);

    if ( req != NULL )
    {
      osal_memcpy(req->nfd.nsdu, nsdu, nsduLen);

      req->nfd.dstAddr           = dstAddr;
      req->nfd.nsduHandleOptions = nsduHandleOptions;
      req->nfd.discoverRoute     = discoverRoute;
      req->nfd.radius            = radius;

      status = NLDE_DataReq( req );
    }
    else
    {
      status = ZMemError;
    }

    return status;
}
#endif // defined ( MT_NWK_FUNC )

/*********************************************************************
 * @fn      MT_NwkCommandProcessing
 *
 * @brief
 *
 *   Process all the NWK commands that are issued by test tool
 *
 * @param   cmd_id - Command ID
 * @param   len    - Length of received SPI data message
 * @param   pData  - pointer to received SPI data message
 *
 * @return  void
 */
void MT_NwkCommandProcessing( uint16 cmd_id , byte len , byte *pData )
{
  byte ret;
#if defined ( MT_NWK_FUNC )
  uint8 dummyExPANID[Z_EXTADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  uint16 dstAddr;
#endif
#if defined ( MT_NWK_FUNC )  //NWK commands
  byte attr;
  byte index;
  byte dataLen;
  byte *dataPtr;
  uint32 channelList;
  byte databuf[SPI_RESP_LEN_NWK_DEFAULT + NWK_DEFAULT_GET_RESPONSE_LEN];
#if defined( ZDO_COORDINATOR )
	uint16 panId;
#else
  byte i,j;
#endif
#endif // MT_NWK_FUNC

  len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_NWK_DEFAULT;
	ret = (byte)ZSuccess;

  switch (cmd_id)
  {
#if defined( RTR_NWK )
    case SPI_CMD_NLME_PERMIT_JOINING_REQ:
      //The only information is PermitDuration
      ret = (byte)NLME_PermitJoiningRequest( *pData );
      break;
#endif

#if defined ( MT_NWK_FUNC )  //NWK commands
    case SPI_CMD_NWK_INIT:
      nwk_init( NWK_TaskID );
      break;

    case SPI_CMD_NLDE_DATA_REQ:
      //First read the DstAddr
      dstAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += sizeof( dstAddr );

      //Get the NSDU details
      dataLen = *pData++;
      dataPtr = pData;

      /* For now, skip a length of ZTEST_DEFAULT_DATA_LEN, instead of dataLen.
         In future ZTOOL releases the data buffer will be only as long as dataLen */

      //pData += dataLen;
      pData += ZTEST_DEFAULT_DATA_LEN;

      /* pData[0] = NSDUHandlde
         pData[1] = NSDUHandleOptions
         pData[3] = SecurityEnable
         pData[4] = DiscoverRoute
         pData[5] = RadiusCounter */

      ret = (byte)MT_Nwk_DataRequest( dstAddr, dataLen, dataPtr, pData[0],
                                      BUILD_UINT16( pData[2], pData[1] ),
                                      pData[3], pData[4], pData[5]);
      break;

#if defined( ZDO_COORDINATOR )
    case SPI_CMD_NLME_INIT_COORD_REQ:
			panId = BUILD_UINT16( pData[1], pData[0] );
			
			MT_ReverseBytes( &pData[2], 4 );
			channelList = osal_build_uint32( &pData[2], 4 );

			ret = (byte)NLME_NetworkFormationRequest( panId, channelList, pData[6], pData[7],
                                                      pData[8], pData[9] );
      break;
#endif  // ZDO

#if defined( RTR_NWK )
    case SPI_CMD_NLME_START_ROUTER_REQ:
      // NOTE: first two parameters are not used, see NLMEDE.h for details
      ret = (byte)NLME_StartRouterRequest( pData[0], pData[1], pData[2] );
    break;
#endif  // RTR

    case SPI_CMD_NLME_JOIN_REQ:
		  ret = (byte)NLME_JoinRequest( dummyExPANID, BUILD_UINT16( pData[1], pData[0] ), pData[2], pData[3] );
      if ( pData[3] & CAPINFO_RCVR_ON_IDLE )
      {
        // The receiver is on, turn network layer polling off.
        NLME_SetPollRate( 0 );
        NLME_SetQueuedPollRate( 0 );
        NLME_SetResponseRate( 0 );
      }
      break;

    case SPI_CMD_NLME_LEAVE_REQ:
      {
        NLME_LeaveReq_t req;
        // if extAddr is all zeros, it means null pointer..
        for( index = 0; ( ( index < Z_EXTADDR_LEN ) &&
                        ( pData[index] == 0 ) ) ; index++ );
        if ( index == Z_EXTADDR_LEN )
        {
          req.extAddr = NULL;
        }
        else
        {
          MT_ReverseBytes( pData, Z_EXTADDR_LEN );
          req.extAddr = pData;
        }
        pData += Z_EXTADDR_LEN;

        req.removeChildren = FALSE;
        req.rejoin         = FALSE;
        req.silent         = FALSE;
        ret = (byte)NLME_LeaveReq( &req );
      }
      break;

    case SPI_CMD_NLME_RESET_REQ:
      //Its a direct call to reset NWK
      ret = (byte)NLME_ResetRequest();
      break;

    case SPI_CMD_NLME_GET_REQ:
      attr = *pData++;
      index = *pData;
			databuf[0] = (byte)NLME_GetRequest( (ZNwkAttributes_t )attr, index, &databuf[1] );
      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_NWK_DEFAULT + NWK_DEFAULT_GET_RESPONSE_LEN;
      MT_BuildAndSendZToolResponse( len, (SPI_RESPONSE_BIT | SPI_CMD_NLME_GET_REQ),
            (SPI_RESP_LEN_NWK_DEFAULT + NWK_DEFAULT_GET_RESPONSE_LEN), databuf );
      return;   // Don't return to this function

    case SPI_CMD_NLME_SET_REQ:
      ret = (byte)NLME_SetRequest( (ZNwkAttributes_t)pData[0], pData[1], &pData[2] );
      osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 1000 );
      break;

    case SPI_CMD_NLME_NWK_DISC_REQ:
      MT_ReverseBytes( pData, 4 );
      ret = (byte)NLME_NetworkDiscoveryRequest( osal_build_uint32( pData, 4 ), pData[4] );
      break;

#if !defined( ZDO_COORDINATOR )
    case SPI_CMD_NLME_ORPHAN_JOIN_REQ:
      // Channel list bit mask
      MT_ReverseBytes( pData, 4 );
      channelList = osal_build_uint32( pData, 4 );

      // Count number of channels
      j = attr = 0;
      for ( i = 0; i < ED_SCAN_MAXCHANNELS; i++ )
      {
        if ( channelList & (1 << i) )
        {
           j++;
           attr = i;
        }
      }

      // If only one channel specified...
      if ( j == 1 )
      {
        _NIB.scanDuration = pData[4];
        _NIB.nwkLogicalChannel = attr;
        _NIB.channelList = channelList;
        if ( !_NIB.CapabilityInfo )
          _NIB.CapabilityInfo = ZDO_Config_Node_Descriptor.CapabilityFlags;

        devState = DEV_NWK_ORPHAN;
        ret = (byte)NLME_OrphanJoinRequest( channelList, pData[4] );
      }
      else
        ret = ZNwkInvalidParam;
      break;
#endif  // !ZDO
	
#if defined (RTR_NWK)
    case SPI_CMD_NLME_ROUTE_DISC_REQ:
      ret = (byte)NLME_RouteDiscoveryRequest( BUILD_UINT16( pData[1], pData[0] ), pData[2] );
      break;
			
    case SPI_CMD_NLME_DIRECT_JOIN_REQ:
      MT_ReverseBytes( pData, 8 );
      ret = (byte)NLME_DirectJoinRequest( pData, pData[8] );
    break;
#endif	// RTR

#endif // MT_NWK_FUNC

    default:
      ret = (byte)ZUnsupportedMode;
      break;
  }

#if defined ( MT_NWK_FUNC )
	MT_SendSPIRespMsg( ret, cmd_id, len, SPI_RESP_LEN_NWK_DEFAULT );
#endif	
  (void)len;
  (void)ret;
}

#if defined ( MT_NWK_CB_FUNC )             //NWK callback commands
/*********************************************************************
 * @fn          nwk_MTCallbackSubDataConfirm
 *
 * @brief       Process the callback subscription for NLDE-DATA.confirm
 *
 * @param       nsduHandle  - APS handle
 * @param       Status      - result of data request
 *
 * @return      none
 */
void nwk_MTCallbackSubDataConfirm(byte nsduHandle, ZStatus_t status )
{
  byte buf[2];

  buf[0] = nsduHandle;
  buf[1] = (byte)status;

  MT_BuildAndSendZToolCB( SPI_CB_NLDE_DATA_CNF, 2, buf );
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubDataIndication
 *
 * @brief       Process the callback subscription for NLDE-DATA.indication
 *
 * @param       SrcAddress      - 16 bit address
 * @param       nsduLength      - Length of incoming data
 * @param       nsdu            - Pointer to incoming data
 * @param       LinkQuality     - Link quality measured during
 *                                reception.
 * @param       SecuritySuite   - Security Suite Applied
 * @param       SecurityStatus  - MLDE_SUCCESS if security process
 *                                successfull, MLDE_FAILURE if not.
 *
 * @return      none
 */
void nwk_MTCallbackSubDataIndication( uint16 SrcAddress, int16 nsduLength,
                                      byte *nsdu, byte LinkQuality )
{
  byte *msgPtr;
  byte *msg;
  byte msgLen;

  msgLen = sizeof( uint16 ) + sizeof( uint8 ) + ZTEST_DEFAULT_DATA_LEN
            + sizeof( byte);

  msgPtr = osal_mem_alloc( msgLen );
  if ( msgPtr )
  {
    //Fill up the data bytes
    msg = msgPtr;

    //First fill in details
    *msg++ = HI_UINT16( SrcAddress );
    *msg++ = LO_UINT16( SrcAddress );

    //Since the max packet size is less than 255 bytes, a byte is enough
    //to represent nsdu length
    *msg++ = ( uint8 ) nsduLength;

    osal_memset( msg, NULL, ZTEST_DEFAULT_DATA_LEN ); // Clear the mem
    osal_memcpy( msg, nsdu, nsduLength );
    msg += ZTEST_DEFAULT_DATA_LEN;

    *msg++ = LinkQuality;

    MT_BuildAndSendZToolCB( SPI_CB_NLDE_DATA_IND, msgLen, msgPtr );

    osal_mem_free( msgPtr );
  }
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubInitCoordConfirm
 *
 * @brief       Process the callback subscription for NLME-INIT-COORD.confirm
 *
 * @param       Status - Result of NLME_InitCoordinatorRequest()
 *
 * @return      none
 */
void nwk_MTCallbackSubInitCoordConfirm( ZStatus_t Status )
{
#if defined( ZDO_COORDINATOR )
  MT_BuildAndSendZToolCB( SPI_CB_NLME_INITCOORD_CNF,
                                    sizeof( byte ), (byte*)&Status );
#endif  // ZDO_COORDINATOR
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubStartRouterConfirm
 *
 * @brief       Process the callback subscription for NLME-START-ROUTER.confirm
 *
 * @param       Status - Result of NLME_StartRouterRequest()
 *
 * @return      none
 */
void nwk_MTCallbackSubStartRouterConfirm( ZStatus_t Status )
{
  MT_BuildAndSendZToolCB( SPI_CB_NLME_START_ROUTER_CNF,
                                        sizeof( byte ), (byte*)&Status );
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubJoinConfirm
 *
 * @brief       Process the callback subscription for NLME-JOIN.confirm
 *
 * @param       Status - Result of NLME_JoinRequest()
 *
 * @return      none
 */
void nwk_MTCallbackSubJoinConfirm(  uint16 PanId, ZStatus_t Status )
{
  byte msg[Z_EXTADDR_LEN + 3];

  // This device's 64-bit address
  ZMacGetReq( ZMacExtAddr, msg );
  MT_ReverseBytes( msg, Z_EXTADDR_LEN );

  msg[Z_EXTADDR_LEN + 0] = HI_UINT16( PanId );
  msg[Z_EXTADDR_LEN + 1] = LO_UINT16( PanId );
  msg[Z_EXTADDR_LEN + 2] = (byte)Status;

  MT_BuildAndSendZToolCB( SPI_CB_NLME_JOIN_CNF, Z_EXTADDR_LEN + 3, msg );
}
/*********************************************************************
 * @fn          nwk_MTCallbackSubNetworkDiscoveryConfirm
 *
 * @brief       Process the callback subscription for NLME-NWK_DISC.confirm
 *
 * @param       ResultCount			- number of networks discovered
 * @param				NetworkList			- pointer to list of network descriptors
 *
 * @return      void
 */
void nwk_MTCallbackSubNetworkDiscoveryConfirm( byte ResultCount, networkDesc_t *NetworkList )
{
	byte len;
	byte *msgPtr;
	byte *msg;
	byte i;

        // The message cannot be bigger then SPI_TX_BUFF_MAX.  Reduce resultCount if necessary
        if (ResultCount * sizeof(networkDesc_t) > SPI_TX_BUFF_MAX - (1 + SPI_0DATA_MSG_LEN))
        {
          ResultCount = (SPI_TX_BUFF_MAX - (1 + SPI_0DATA_MSG_LEN)) / sizeof(networkDesc_t);
        }

	len = 1 + ResultCount * sizeof(networkDesc_t);
  msgPtr = osal_mem_alloc( len );
	if ( msgPtr )
	{
	 //Fill up the data bytes
    msg = msgPtr;

		*msg++ = ResultCount;

		for ( i = 0; i < ResultCount; i++ )
		{
		  *msg++ = HI_UINT16( NetworkList->panId );
		  *msg++ = LO_UINT16( NetworkList->panId );
		  *msg++ = NetworkList->logicalChannel;
		  *msg++ = NetworkList->beaconOrder;
		  *msg++ = NetworkList->superFrameOrder;
		  *msg++ = NetworkList->routerCapacity;
		  *msg++ = NetworkList->deviceCapacity;
		  *msg++ = NetworkList->version;
		  *msg++ = NetworkList->stackProfile;
		  //*msg++ = NetworkList->securityLevel;
		
			NetworkList = (networkDesc_t*)NetworkList->nextDesc;
		}

    MT_BuildAndSendZToolCB( SPI_CB_NLME_NWK_DISC_CNF, len, msgPtr );

    osal_mem_free( msgPtr );
	}
}
/*********************************************************************
 * @fn          nwk_MTCallbackSubJoinIndication
 *
 * @brief       Process the callback subscription for NLME-INIT-COORD.indication
 *
 * @param       ShortAddress - 16-bit address
 * @param       ExtendedAddress - IEEE (64-bit) address
 * @param       CapabilityInformation - Association Capability Information
 *
 * @return      ZStatus_t
 */
void nwk_MTCallbackSubJoinIndication( uint16 ShortAddress, byte *ExtendedAddress,
                                      byte CapabilityInformation )
{
  byte *msgPtr;
  byte *msg;
  byte len;

  len = sizeof( uint16 ) + Z_EXTADDR_LEN + sizeof( byte );
  msgPtr = osal_mem_alloc( len );

  if ( msgPtr )
  {
    //Fill up the data bytes
    msg = msgPtr;

    //First fill in details
    *msg++ = HI_UINT16( ShortAddress );
    *msg++ = LO_UINT16( ShortAddress );

    osal_cpyExtAddr( msg, ExtendedAddress );
    MT_ReverseBytes( msg, Z_EXTADDR_LEN );
    msg += Z_EXTADDR_LEN;

    *msg = CapabilityInformation;

    MT_BuildAndSendZToolCB( SPI_CB_NLME_JOIN_IND, len, msgPtr );

    osal_mem_free( msgPtr );
  }
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubLeaveConfirm
 *
 * @brief       Process the callback subscription for NLME-LEAVE.confirm
 *
 * @param       DeviceAddress - IEEE (64-bit) address
 * @param       Status - Result of NLME_LeaveRequest()
 *
 * @return      none
 */
void nwk_MTCallbackSubLeaveConfirm( byte *DeviceAddress, ZStatus_t Status )
{
  byte *msgPtr;
  byte *msg;

  msgPtr = osal_mem_alloc( Z_EXTADDR_LEN + sizeof( byte ) );
  if ( msgPtr )
  {
    //Fill up the data bytes
    msg = msgPtr;

    //First fill in details
    osal_cpyExtAddr( msg, DeviceAddress );
    MT_ReverseBytes( msg, Z_EXTADDR_LEN );
    msg += Z_EXTADDR_LEN;

    *msg = (byte)Status;

    MT_BuildAndSendZToolCB( SPI_CB_NLME_LEAVE_CNF,
                                  Z_EXTADDR_LEN + sizeof( byte ), msgPtr );

    osal_mem_free( msgPtr );
  }
}
/*********************************************************************
 * @fn          nwk_MTCallbackSubLeaveIndication
 *
 * @brief       Process the callback subscription for NLME-LEAVE.indication
 *
 * @param       DeviceAddress - IEEE (64-bit) address
 *
 * @return      NULL
 */
void nwk_MTCallbackSubLeaveIndication( byte *DeviceAddress )
{
  byte msg[Z_EXTADDR_LEN+1];

  //First fill in details
  if ( DeviceAddress )
  {
    osal_cpyExtAddr( msg, DeviceAddress );
    MT_ReverseBytes( msg, Z_EXTADDR_LEN );
  }
  else
    osal_memset( msg, 0, Z_EXTADDR_LEN );
  msg[Z_EXTADDR_LEN] = 0;  // Status, assume good if we get this far

  MT_BuildAndSendZToolCB( SPI_CB_NLME_LEAVE_IND, Z_EXTADDR_LEN+1, msg );
}
/*********************************************************************
 * @fn          nwk_MTCallbackSubSyncIndication
 *
 * @brief       Process the callback subscription for NLME-SYNC.indication
 *
 * @param       none
 *
 * @return      none
 */
void nwk_MTCallbackSubSyncIndication( void )
{
  MT_BuildAndSendZToolCB( SPI_CB_NLME_SYNC_IND, 0, NULL );
}

/*********************************************************************
 * @fn          nwk_MTCallbackSubPollConfirm
 *
 * @brief       Process the callback subscription for NLME-POLL.confirm
 *
 * @param       status - status of the poll operation
 *
 * @return      none
 */
void nwk_MTCallbackSubPollConfirm( byte status )
{
  byte msg = status;
  MT_BuildAndSendZToolCB( SPI_CB_NLME_POLL_CNF, 1, &msg );
}

#endif /*NWK Callback commands*/

/*********************************************************************
*********************************************************************/

