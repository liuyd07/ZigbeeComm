/*********************************************************************
    Filename:       MTEL.c
    Revised:        $Date: 2007-05-16 11:21:09 -0700 (Wed, 16 May 2007) $
    Revision:       $Revision: 14313 $

    Description:

        MonitorTest Event Loop functions.  Everything in the
        MonitorTest Task (except the serial driver).

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

#if defined( MT_TASK )

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OnBoard.h"
#include "OSAL.h"
#include "OSAL_Memory.h"
#include "OSAL_Nv.h"
#include "MTEL.h"
#include "DebugTrace.h"
#include "ZMAC.h"

#if !defined ( NONWK )
  #include "NLMEDE.h"
  #include "nwk_bufs.h"
  #include "ZDObject.h"
  #include "ssp.h"
  #include "nwk_util.h"
#endif

#if defined( MT_MAC_FUNC ) || defined( MT_MAC_CB_FUNC )
  #include "MT_MAC.h"
#endif
#if defined( MT_NWK_FUNC ) || defined( MT_NWK_CB_FUNC )
  #include "MT_NWK.h"
  #include "nwk.h"
  #include "nwk_bufs.h"
#endif
#if defined( MT_AF_FUNC ) || defined( MT_AF_CB_FUNC )
  #include "MT_AF.h"
#endif
#if defined( MT_USER_TEST_FUNC )
  #include "AF.h"
#endif
#if defined( MT_ZDO_FUNC )
  #include "MT_ZDO.h"
#endif
#if defined (MT_SAPI_FUNC)
	#include "MT_SAPI.h"
#endif
#if defined( APP_TP )
 #include "TestProfile.h"
#endif
#if defined( APP_TP2 )
 #include "TestProfile2.h"
#endif

#if defined(APP_TGEN)
  #include "TrafficGenApp.h"
#endif
#if defined(APP_DEBUG)
	#include "DebugApp.h"
#endif
#if defined (NWK_TEST)
	#include "HWTTApp.h"
#endif

/* HAL */
#include "hal_uart.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_mailbox.h"
#include "SPIMgr.h"

/*********************************************************************
 * MACROS
 */
#define MTEL_DEBUG_INFO( nParams, p1, p2, p3 ) DEBUG_INFO( COMPID_MTEL, nParams, p1, p2, p3 )

#if defined( EXTERNAL_RAM )
  #define IS_MEM_VALID( Addr )  \
        /* Check for valid internal RAM address. */\
    ( ( (((Addr) >= MCU_RAM_BEG) && ((Addr) <= MCU_RAM_END)) ||  \
        /* Check for valid external RAM address. */\
        (((Addr) >= EXT_RAM_BEG) && ((Addr) <= EXT_RAM_END)) ) ? TRUE : FALSE )
#else
  #define IS_MEM_VALID( Addr )  \
        /* Check for valid internal RAM address. */\
    ( ( ((Addr) >= MCU_RAM_BEG) && ((Addr) <= MCU_RAM_END) ) ? TRUE : FALSE )
#endif

/*********************************************************************
 * CONSTANTS
 */

#ifdef ZPORT
const char *MTVersionString[] = {"1.00 (F8W1.4.2-ZP)", "1.10 (F8W1.4.2-ZP)"};
#else
const char *MTVersionString[] = {"1.00 (F8W1.4.2)", "1.10 (F8W1.4.2)"};
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte MT_TaskID;
byte debugThreshold;
byte debugCompId;

UINT16 save_cmd;

//DEBUG
uint32 longvar;
uint16 *temp_glob_ptr1;
uint16 *temp_glob_ptr2;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
extern unsigned int mac_sim_eventLoop( void );

#ifdef MACSIM
  // Used to pass Zignet message
  extern void MACSIM_TranslateMsg( byte *buf, byte bLen );
#endif

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void MT_MsgQueueInit( void );
void MT_ProcessCommand( mtOSALSerialData_t *msg );
void MT_ProcessSerialCommand( byte *msg );
byte MT_RAMRead( UINT16 addr, byte *pData );
byte MT_RAMWrite( UINT16 addr , byte val );
void MT_ProcessDebugMsg( mtDebugMsg_t *pData );
void MT_ProcessDebugStr( mtDebugStr_t *pData );
byte MT_SetDebugThreshold( byte comp_id, byte threshold );
void MT_SendErrorNotification( byte err );
void MT_ResetMsgQueue( void );
byte MT_QueueMsg( byte *msg , byte len );
void MT_ProcessQueue( void );
void MT_SendSPIRespMsg( byte ret, uint16 cmd_id, byte msgLen, byte respLen);
void MT_Reset(byte typID);
byte MT_ProcessSetNV( byte *pData );
void MT_ProcessGetNV( byte *pData );
void MT_ProcessGetNvInfo( void );
void MT_ProcessGetDeviceInfo( void );
byte MTProcessAppMsg( byte *pData, byte len );
void MTProcessAppRspMsg( byte *pData, byte len );

#if (defined HAL_LED) && (HAL_LED == TRUE)
byte MTProcessLedControl( byte *pData );
#endif

#if defined ( MT_USER_TEST_FUNC )
void MT_ProcessAppUserCmd( byte *pData );
#endif

/*********************************************************************
 * @fn      MT_TaskInit
 *
 * @brief
 *
 *   MonitorTest Task Initialization.  This function is put into the
 *   task table.
 *
 * @param   byte task_id - task ID of the MT Task
 *
 * @return  void
 *
 *********************************************************************/
void MT_TaskInit( byte task_id )
{
  MT_TaskID = task_id;

  debugThreshold = 0;
  debugCompId = 0;

  // Initialize the Serial port
  SPIMgr_Init();

} /* MT_TaskInit() */

#ifdef ZTOOL_PORT
/*********************************************************************
 * @fn      MT_IndReset()
 *
 * @brief   Sends a ZTOOL "reset response" message.
 *
 * @param   None
 *
 * @return  None
 *
 *********************************************************************/
void MT_IndReset( void )
{

  byte rsp = 0;  // Reset type==0 indicates Z-Stack reset

  // Send out Reset Response message
  MT_BuildAndSendZToolResponse( (SPI_0DATA_MSG_LEN + sizeof( rsp )),
                                (SPI_RESPONSE_BIT | SPI_CMD_SYS_RESET),
                                sizeof( rsp ), &rsp );
}
#endif

/*********************************************************************
 * @fn      MT_ProcessEvent
 *
 * @brief
 *
 *   MonitorTest Task Event Processor.  This task is put into the
 *   task table.
 *
 * @param   byte task_id - task ID of the MT Task
 * @param   UINT16 events - event(s) for the MT Task
 *
 * @return  void
 */
UINT16 MT_ProcessEvent( byte task_id, UINT16 events )
{
  uint8 *msg_ptr;

  // Could be multiple events, so switch won't work

  if ( events & SYS_EVENT_MSG )
  {
    while ( (msg_ptr = osal_msg_receive( MT_TaskID )) )
    {
      MT_ProcessCommand( (mtOSALSerialData_t *)msg_ptr );
    }

    // Return unproccessed events
    return (events ^ SYS_EVENT_MSG);
  }

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  if ( events & MT_ZTOOL_SERIAL_RCV_BUFFER_FULL )
  {
    // Do sometype of error processing
    MT_SendErrorNotification(RECEIVE_BUFFER_FULL);

    // Return unproccessed events
    return (events ^ MT_ZTOOL_SERIAL_RCV_BUFFER_FULL);
  }
#endif

  // Discard or make more handlers
  return 0;

} /* MT_ProcessEvent() */

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/*********************************************************************
 * @fn      MT_BuildSPIMsg
 *
 * @brief
 *
 *   Format an SPI message.
 *
 * @param   UINT16 cmd - command id
 * @param   byte *msg - pointer to message buffer
 * @param   byte dataLen - length of data field
 * @param   byte *pData - pointer to data field
 *
 * @return  void
 */
void MT_BuildSPIMsg( UINT16 cmd, byte *msg, byte dataLen, byte *pData )
{
  byte *msgPtr;

  *msg++ = SOP_VALUE;

  msgPtr = msg;

  *msg++ = (byte)(HI_UINT16( cmd ));
  *msg++ = (byte)(LO_UINT16( cmd ));

  if ( pData )
  {
    *msg++ = dataLen;

    msg = osal_memcpy( msg, pData, dataLen );
  }
  else
    *msg++ = 0;

  *msg = SPIMgr_CalcFCS( msgPtr, (byte)(3 + dataLen) );
}
#endif

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/*********************************************************************
 * @fn      MT_BuildAndSendZToolResponse
 *
 * @brief
 *
 *   Build and send a ZTOOL msg
 *
 * @param   byte err
 *
 * @return  void
 */
void MT_BuildAndSendZToolResponse( byte msgLen, uint16 cmd,
                                   byte dataLen, byte *pData )
{
  byte *msg_ptr;

  // Get a message buffer to build response message
  msg_ptr = osal_mem_alloc( msgLen );
  if ( msg_ptr )
  {
#ifdef SPI_MGR_DEFAULT_PORT
    MT_BuildSPIMsg( cmd, msg_ptr, dataLen, pData );
    HalUARTWrite ( SPI_MGR_DEFAULT_PORT, msg_ptr, msgLen );
#endif
    osal_mem_free( msg_ptr );
  }
}
#endif

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/*********************************************************************
 * @fn      MT_BuildAndSendZToolCB
 *
 * @brief
 *
 *   Build and send a ZTOOL Callback msg
 *
 * @param   len - length of data portion of the message
 *
 * @return  void
 */
void MT_BuildAndSendZToolCB( uint16 callbackID, byte len, byte *pData )
{
  byte msgLen;
  mtOSALSerialData_t *msgPtr;
  byte *msg;

  msgLen = sizeof ( mtOSALSerialData_t ) + SPI_0DATA_MSG_LEN + len;

  msgPtr = (mtOSALSerialData_t *)osal_msg_allocate( msgLen );
  if ( msgPtr )
  {
    msgPtr->hdr.event = CB_FUNC;
    msgPtr->msg = (uint8 *)(msgPtr+1);
    msg = msgPtr->msg;

    //First byte is used as the event type for MT
    *msg++ = SOP_VALUE;
    *msg++ = HI_UINT16( callbackID );
    *msg++ = LO_UINT16( callbackID );
    *msg++ = len;

    //Fill up the data bytes
    osal_memcpy( msg, pData, len );

    osal_msg_send( MT_TaskID, (uint8 *)msgPtr );
  }
}
#endif

/*********************************************************************
 * @fn      MT_ProcessCommand
 *
 * @brief
 *
 *   Process Event Messages.
 *
 * @param   byte *msg - pointer to event message
 *
 * @return
 */
void MT_ProcessCommand( mtOSALSerialData_t *msg )
{
  byte deallocate;
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  byte *msg_ptr;
  byte len;

  // A little setup for AF, CB_FUNC and MT_SYS_APP_RSP_MSG
  msg_ptr = msg->msg;
#endif // ZTOOL

  deallocate = true;

  // Use the first byte of the message as the command ID
  switch ( msg->hdr.event )
  {
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
    case CMD_SERIAL_MSG:
      MT_ProcessSerialCommand( msg->msg );
      break;

    case CMD_DEBUG_MSG:
      MT_ProcessDebugMsg( (mtDebugMsg_t *)msg );
      break;

    case CMD_DEBUG_STR:
      MT_ProcessDebugStr( (mtDebugStr_t *)msg );
      break;

    case CB_FUNC:
      /*
        Build SPI message here instead of redundantly calling MT_BuildSPIMsg
        because we have copied data already in the allocated message
      */

      /* msg_ptr is the beginning of the intended SPI message */
      len = SPI_0DATA_MSG_LEN + msg_ptr[DATALEN_FIELD];

      /*
        FCS goes to the last byte in the message and is calculated over all
        the bytes except FCS and SOP
      */
      msg_ptr[len-1] = SPIMgr_CalcFCS( msg_ptr + 1 , (byte)(len-2) );

#ifdef SPI_MGR_DEFAULT_PORT
      HalUARTWrite ( SPI_MGR_DEFAULT_PORT, msg_ptr, len );
#endif
      break;

#if !defined ( NONWK )
    case MT_SYS_APP_RSP_MSG:
      len = SPI_0DATA_MSG_LEN + msg_ptr[DATALEN_FIELD];
      MTProcessAppRspMsg( msg_ptr, len );
      break;
#endif  // NONWK
#endif  // ZTOOL

    default:
      break;
  }

  if ( deallocate )
  {
    osal_msg_deallocate( (uint8 *)msg );
  }
}

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/*********************************************************************
 * @fn      MT_ProcessDebugMsg
 *
 * @brief
 *
 *   Build and send a debug message.
 *
 * @param   byte *data - pointer to the data portion of the debug message
 *
 * @return  void
 */
void MT_ProcessDebugMsg( mtDebugMsg_t *msg )
{
  byte *msg_ptr;
  byte dataLen;
  uint8 buf[11];
  uint8 *pBuf;

  // Calculate the data length based
  dataLen = 5 + (msg->numParams * sizeof ( uint16 ));

  // Get a message buffer to build the debug message
  msg_ptr = osal_msg_allocate( (byte)(SPI_0DATA_MSG_LEN + dataLen + 1) );
  if ( msg_ptr )
  {
    // Build the message
    pBuf = buf;
    *pBuf++ = msg->compID;
    *pBuf++ = msg->severity;
    *pBuf++ = msg->numParams;

    if ( msg->numParams >= 1 )
    {
      *pBuf++ = HI_UINT16( msg->param1 );
      *pBuf++ = LO_UINT16( msg->param1 );
    }

    if ( msg->numParams >= 2 )
    {
      *pBuf++ = HI_UINT16( msg->param2 );
      *pBuf++ = LO_UINT16( msg->param2 );
    }

    if ( msg->numParams == 3 )
    {
      *pBuf++ = HI_UINT16( msg->param3 );
      *pBuf++ = LO_UINT16( msg->param3 );
    }

    *pBuf++ = HI_UINT16( msg->timestamp );
    *pBuf++ = LO_UINT16( msg->timestamp );

#ifdef SPI_MGR_DEFAULT_PORT
    MT_BuildSPIMsg( SPI_CMD_DEBUG_MSG, &msg_ptr[1], dataLen, buf );
    HalUARTWrite ( SPI_MGR_DEFAULT_PORT, &msg_ptr[1], SPI_0DATA_MSG_LEN + dataLen );
#endif
    osal_msg_deallocate( msg_ptr );
  }
}
#endif // ZTOOL

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/*********************************************************************
 * @fn      MT_ProcessDebugStr
 *
 * @brief
 *
 *   Build and send a debug string.
 *
 * @param   byte *dstr - pointer to the data portion of the debug message
 *
 * @return  void
 */
void MT_ProcessDebugStr( mtDebugStr_t *dstr )
{
  byte *msg_ptr;

  // Get a message buffer to build the debug message
  msg_ptr = osal_mem_alloc( (byte)(SPI_0DATA_MSG_LEN + dstr->sln) );
  if ( msg_ptr )
  {
#ifdef SPI_MGR_DEFAULT_PORT
    MT_BuildSPIMsg( SPI_RESPONSE_BIT | SPI_SYS_STRING_MSG, msg_ptr, dstr->sln, dstr->pString );
    HalUARTWrite ( SPI_MGR_DEFAULT_PORT, msg_ptr, SPI_0DATA_MSG_LEN + dstr->sln );
#endif
    osal_mem_free( msg_ptr );
  }
}
#endif // ZTOOL

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/*********************************************************************
 * @fn      MT_ProcessSetNV
 *
 * @brief
 *
 *   The Set NV serial message.
 *
 * @param   byte *msg - pointer to the data
 *
 * @return  ZSuccess if successful
 *
 * @MT SPI_CMD_SYS_SET_NV
 */
byte MT_ProcessSetNV( byte *pData )
{
  uint16  attrib;
  uint16  attlen;

  attrib = (uint16) *pData++;
  attlen = osal_nv_item_len( attrib );

  return osal_nv_write( attrib, 0, attlen, pData );
}
#endif

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/*********************************************************************
 * @fn      MT_ProcessGetNV
 *
 * @brief
 *
 *   The Get NV serial message.
 *
 * @param   byte *msg - pointer to the data
 *
 * @return  void
 *
 * @MT SPI_CMD_SYS_GET_NV
 *
 */
void MT_ProcessGetNV( byte *pData )
{
  uint16  attrib;
  uint16 attlen;
  uint16 buflen;
  uint8 *buf;

  attrib = (uint16)*pData;
  attlen = osal_nv_item_len( attrib );

  buflen = attlen + 2;
  buf = osal_mem_alloc( buflen );
  if ( buf != NULL )
  {
    osal_memset( buf, 0, buflen );

    buf[0] = osal_nv_read( attrib, 0, attlen, &buf[2] );
    buf[1] = (uint8)attrib;

    MT_BuildAndSendZToolResponse( (SPI_0DATA_MSG_LEN + buflen),
                                  (SPI_RESPONSE_BIT | SPI_CMD_SYS_GET_NV),
                                  buflen, buf );
    osal_mem_free( buf );
  }
}
#endif

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
#if !defined ( NONWK )
/***************************************************************************************************
 * @fn      MT_ProcessGetNvInfo
 *
 * @brief
 *
 *   The Get NV Info serial message.
 *
 * @param   byte *msg - pointer to the data
 *
 * @return  void
 *
 * @MT SPI_CMD_SYS_GET_NV_INFO
 *
 ***************************************************************************************************/
void MT_ProcessGetNvInfo( void )
{
  uint8 len;
  uint8 stat;
  uint8 *buf;
  uint8 *pBuf;
  uint16 tmp16;
  uint32 tmp32;

  // Get required length of buffer
  // Status + ExtAddr + ChanList + PanID  + SecLevel + PreCfgKey
  len = 1 + Z_EXTADDR_LEN + 4 + 2 + 1 + SEC_KEY_LEN;

  buf = osal_mem_alloc( len );
  if ( buf )
  {
    // Assume NV not available
    osal_memset( buf, 0xFF, len );

    // Skip over status
    pBuf = buf + 1;

    // Start with 64-bit extended address
    stat = osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, pBuf );
    if ( stat ) stat = 0x01;
    MT_ReverseBytes( pBuf, Z_EXTADDR_LEN );
    pBuf += Z_EXTADDR_LEN;

    // Scan channel list (bit mask)
    if (  osal_nv_read( ZCD_NV_CHANLIST, 0, sizeof( tmp32 ), &tmp32 ) )
      stat |= 0x02;
    else
    {
      pBuf[0] = BREAK_UINT32( tmp32, 3 );
      pBuf[1] = BREAK_UINT32( tmp32, 2 );
      pBuf[2] = BREAK_UINT32( tmp32, 1 );
      pBuf[3] = BREAK_UINT32( tmp32, 0 );
    }
    pBuf += sizeof( tmp32 );

    // ZigBee PanID
    if ( osal_nv_read( ZCD_NV_PANID, 0, sizeof( tmp16 ), &tmp16 ) )
      stat |= 0x04;
    else
    {
      pBuf[0] = HI_UINT16( tmp16 );
      pBuf[1] = LO_UINT16( tmp16 );
    }
    pBuf += sizeof( tmp16 );

    // Security level
    if ( osal_nv_read( ZCD_NV_SECURITY_LEVEL, 0, sizeof( uint8 ), pBuf++ ) )
      stat |= 0x08;

    // Pre-configured security key
    if ( osal_nv_read( ZCD_NV_PRECFGKEY, 0, SEC_KEY_LEN, pBuf ) )
      stat |= 0x10;

    // Status bit mask - bit=1 indicates failure
    *buf = stat;

    MT_BuildAndSendZToolResponse( (SPI_0DATA_MSG_LEN + len),
                                  (SPI_RESPONSE_BIT | SPI_CMD_SYS_GET_NV_INFO),
                                  len, buf );

    osal_mem_free( buf );
  }
}
#endif  // NONWK
#endif  // ZTOOL

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
#define DEVICE_INFO_RESPONSE_LEN 46
#define TYPE_COORDINATOR         1
#define TYPE_ROUTER              2
#define TYPE_ENDDEVICE           4
/***************************************************************************************************
 * @fn      MT_ProcessGetDeviceInfo
 *
 * @brief
 *
 *   The Get Device Info serial message.
 *
 * @param   byte *msg - pointer to the data
 *
 * @return  void
 ***************************************************************************************************/
void MT_ProcessGetDeviceInfo( void )
{
  byte *buf;
  byte *pBuf;
  uint8 deviceType = 0;
  uint16 shortAddr;
  uint16 *assocList;
  byte assocCnt;
  uint16 *puint16;
  byte x;

  buf = osal_mem_alloc( DEVICE_INFO_RESPONSE_LEN );
  if ( buf )
  {
    pBuf = buf;

    *pBuf++ = ZSUCCESS;

    osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, pBuf );
    // Outgoing extended address needs to be reversed
    MT_ReverseBytes( pBuf, Z_EXTADDR_LEN );
    pBuf += Z_EXTADDR_LEN;

#if !defined( NONWK )
    shortAddr = NLME_GetShortAddr();
#else
    shortAddr = 0;
#endif

    *pBuf++ = HI_UINT16( shortAddr );
    *pBuf++ = LO_UINT16( shortAddr );

    // Return device type
#if !defined( NONWK )
#if defined (ZDO_COORDINATOR)
    deviceType |= (uint8) TYPE_COORDINATOR;
  #if defined (SOFT_START)
    deviceType |= (uint8) TYPE_ROUTER;
  #endif
#endif
#if defined (RTR_NWK) && !defined (ZDO_COORDINATOR)
    deviceType |= (uint8) TYPE_ROUTER;
#elif !defined (RTR_NWK)
    deviceType |= (uint8) TYPE_ENDDEVICE;
#endif
#endif
    *pBuf++ = (byte) deviceType;

    //Return device state
#if !defined( NONWK )
    *pBuf++ = (byte)devState;
#else
    *pBuf++ = (byte)0;
#endif

#if defined(RTR_NWK) && !defined( NONWK )
    assocList = AssocMakeList( &assocCnt );
#else
    assocCnt = 0;
    assocList = NULL;
#endif

    *pBuf++ = assocCnt;

    // upto 16 devices
    osal_memset( pBuf, 0, (16 * sizeof(uint16)) );
    puint16 = assocList;
    for ( x = 0; x < assocCnt; x++ )
    {
      *pBuf++ = HI_UINT16( *puint16 );
      *pBuf++ = LO_UINT16( *puint16 );
      puint16++;
    }

    if ( assocList )
      osal_mem_free( assocList );

    MT_BuildAndSendZToolResponse( (SPI_0DATA_MSG_LEN + DEVICE_INFO_RESPONSE_LEN),
                                  (SPI_RESPONSE_BIT | SPI_CMD_SYS_GET_DEVICE_INFO),
                                  DEVICE_INFO_RESPONSE_LEN, buf );

    osal_mem_free( buf );
  }
}
#endif

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/***************************************************************************************************
 * @fn      MT_ProcessSerialCommand
 *
 * @brief
 *
 *   Process Serial Message.
 *
 * @param   byte *msg - pointer to event message
 *
 * @return  void
 ***************************************************************************************************/
void MT_ProcessSerialCommand( byte *msg )
{
  UINT16 cmd;
  UINT16 callbackID;
  byte len;
  byte ret;
  byte *pData;
  uint16 tmp16;
  uint32 tmp32;
  byte extAddr[Z_EXTADDR_LEN];
  byte *retValue;
  byte x = 0;
#if !defined ( NONWK )
  uint16 attLen;
#endif // NONWK

  // dig out header info
  cmd = BUILD_UINT16( msg[1], msg[0] );
  save_cmd = cmd;
  len = msg[2];
  pData = &msg[3];

    // Setup for return;
    len = 0;
    retValue = &ret;

    //Process the contents of the message
    switch ( cmd )
    {
#ifdef MACSIM
      case SPI_CMD_ZIGNET_DATA:
        MACSIM_TranslateMsg( pData, len );
        break;
#endif

      case SPI_CMD_SYS_RAM_READ:
        extAddr[0] = MT_RAMRead( (UINT16)BUILD_UINT16( pData[1], pData[0] ), &extAddr[1] );
        len = MT_RAM_READ_RESP_LEN;
        retValue = extAddr;
        break;

      case SPI_CMD_SYS_RAM_WRITE:
        ret = MT_RAMWrite( (UINT16)BUILD_UINT16( pData[1], pData[0] ), pData[2] );
        len = MT_RAM_WRITE_RESP_LEN;
        break;

      case SPI_CMD_SYS_SET_DEBUG_THRESHOLD:
        ret = MT_SetDebugThreshold( pData[0], pData[1] );
        len = 1;
        break;

      case SPI_CMD_TRACE_SUB:
        break;

      case SPI_CMD_SYS_RESET:
        MT_Reset( pData[0] );
        break;

      case SPI_CMD_SYS_CALLBACK_SUB_CMD:
        // a callback value of 0xFFFF turns on all available callbacks
        callbackID = BUILD_UINT16( pData[1] , pData[0] );
        if ( callbackID == 0xFFFF )
        {
          // What is the action
          if ( pData[2] )
          {
            // Turn on
#if defined( MT_MAC_CB_FUNC )
            _macCallbackSub = 0xFFFF;
#endif
#if defined( MT_NWK_CB_FUNC )
            _nwkCallbackSub = 0xFFFF;
#endif

#if defined( MT_ZDO_FUNC )
            _zdoCallbackSub = 0xFFFFFFFF;
#endif
#if defined( MT_AF_CB_FUNC )
            _afCallbackSub = 0xFFFF;
#endif
#if defined( MT_SAPI_CB_FUNC )
            _sapiCallbackSub = 0xFFFF;
#endif
          }
          else
          {
            // Turn off
#if defined( MT_MAC_CB_FUNC )
            _macCallbackSub = 0x0000;
#endif
#if defined( MT_NWK_CB_FUNC )
            _nwkCallbackSub = 0x0000;
#endif

#if defined( MT_ZDO_FUNC )
            _zdoCallbackSub = 0x00000000;
#endif
#if defined( MT_AF_CB_FUNC )
            _afCallbackSub = 0x0000;
#endif
#if defined( MT_SAPI_CB_FUNC )
            _sapiCallbackSub = 0x0000;
#endif
          }
        }
        else
        {
          //First check which layer callbacks are desired and then set the preference

#if defined( MT_MAC_CB_FUNC )
          //If it is a MAC callback, set the corresponding callback subscription bit
          if (( callbackID & 0xFFF0 ) == SPI_MAC_CB_TYPE )
          {
            //Based on the action field, either enable or disable subscription
            if ( pData[2] )
              _macCallbackSub |=  ( 1 << ( pData[1] & 0x0F ) );
            else
              _macCallbackSub &= ~( 1 << ( pData[1] & 0x0F ) );
          }
#endif

#if defined( MT_NWK_CB_FUNC )
          //If it is a NWK callback, set the corresponding callback subscription bit
          if (( callbackID & 0xFFF0 ) == SPI_NWK_CB_TYPE )
          {

            //Based on the action field, either enable or disable subscription
            if ( pData[2] )
              _nwkCallbackSub |=  ( 1 << ( pData[1] & 0x0F ) ) ;
            else
              _nwkCallbackSub &= ~( 1 << ( pData[1] & 0x0F ) );
          }
#endif

#if defined( MT_ZDO_FUNC )
          //If it is a APS callback, set the corresponding callback subscription bit
          if ( ((callbackID & 0xFFF0) == SPI_ZDO_CB_TYPE) ||
               ((callbackID & 0xFFF0) == SPI_ZDO_CB2_TYPE) )
          {
            //Based on the action field, either enable or disable subscription
            if ( pData[2] )
              _zdoCallbackSub |=  ( 1L << ( pData[1] & 0x1F ) );
            else
              _zdoCallbackSub &= ~( 1L << ( pData[1] & 0x1F ) );
          }
#endif

#if defined( MT_AF_CB_FUNC )
          // Set the corresponding callback subscription bit for an AF callback.
          if (( callbackID & 0xFFF0 ) == SPI_AF_CB_TYPE )
          {
            // Based on the action field, either enable or disable subscription.
            if ( pData[2] )
              _afCallbackSub |=  ( 1 << ( pData[1] & 0x0F ) );
            else
              _afCallbackSub &= ~( 1 << ( pData[1] & 0x0F ) );
          }
#endif
#if defined( MT_SAPI_CB_FUNC )
          // Set the corresponding callback subscription bit for an SAPI callback.
          if (( callbackID & 0xFFF0 ) == SPI_SAPI_CB_TYPE )
          {
            // Based on the action field, either enable or disable subscription.
            if ( pData[2] )
              _sapiCallbackSub |=  ( 1 << ( pData[1] & 0x0F ) );
            else
              _sapiCallbackSub &= ~( 1 << ( pData[1] & 0x0F ) );
          }
#endif
        }
        len = 1;
        ret = ZSUCCESS;
        break;

      case SPI_CMD_SYS_PING:
        // Get a message buffer to build response message
        // The Ping response now has capabilities included

        // Build Capabilities
        tmp16 = MT_CAP_MAC | MT_CAP_NWK | MT_CAP_AF |
              MT_CAP_ZDO | MT_CAP_USER_TEST | MT_CAP_SAPI_FUNC;

        // Convert to high byte first into temp buffer
        extAddr[0] = HI_UINT16( tmp16 );
        extAddr[1] = LO_UINT16( tmp16 );
        len = sizeof ( tmp16 );
        retValue = extAddr;
        break;

      case SPI_CMD_SYS_VERSION:
        {
#if !defined ( NONWK )
          uint8 i = NLME_GetProtocolVersion() - 1;
#else
          uint8 i = 1;   // just say '1.1' -- irrelevant if stack isn't there anyway
#endif

          // Get a message buffer to build response message
          len      = (byte)(osal_strlen( (char *)MTVersionString[i] ));
          retValue = (byte *)MTVersionString[i];
        }
          break;

      case SPI_CMD_SYS_SET_EXTADDR:
        // Incoming extended address is reversed
        MT_ReverseBytes( pData, Z_EXTADDR_LEN );

        if ( ZMacSetReq( ZMacExtAddr, pData ) == ZMacSuccess )
          ret = osal_nv_write( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, pData );
        else
          ret = 1;
        len = 1;
        break;

      case SPI_CMD_SYS_GET_EXTADDR:
        ZMacGetReq( ZMacExtAddr, extAddr );

        // Outgoing extended address needs to be reversed
        MT_ReverseBytes( extAddr, Z_EXTADDR_LEN );

        len = Z_EXTADDR_LEN;
        retValue = extAddr;
        break;

#if !defined ( NONWK )
      case SPI_CMD_SYS_SET_PANID:
        tmp16 = BUILD_UINT16( pData[1], pData[0] );
        attLen = osal_nv_item_len( ZCD_NV_PANID );
        ret = osal_nv_write( ZCD_NV_PANID, 0, attLen, &tmp16 );
        len = 1;
        break;

      case SPI_CMD_SYS_SET_CHANNELS:
        tmp32 = BUILD_UINT32( pData[3], pData[2], pData[1], pData[0] );
        attLen = osal_nv_item_len( ZCD_NV_CHANLIST );
        ret = osal_nv_write( ZCD_NV_CHANLIST, 0, attLen, &tmp32 );
        len = 1;
        break;

      case SPI_CMD_SYS_SET_SECLEVEL:
        attLen = osal_nv_item_len( ZCD_NV_SECURITY_LEVEL );
        ret = osal_nv_write( ZCD_NV_SECURITY_LEVEL, 0, attLen, pData );
        len = 1;
        break;

      case SPI_CMD_SYS_SET_PRECFGKEY:
        attLen = osal_nv_item_len( ZCD_NV_PRECFGKEY );
        ret = osal_nv_write( ZCD_NV_PRECFGKEY, 0, attLen, pData );
        len = 1;
        break;

      case SPI_CMD_SYS_GET_NV_INFO:
        MT_ProcessGetNvInfo();
        break;
#endif // NONWK

      case SPI_CMD_SYS_GET_DEVICE_INFO:
        MT_ProcessGetDeviceInfo();
        break;

      case SPI_CMD_SYS_SET_NV:
        ret = MT_ProcessSetNV( pData );
        len = 1;
        break;

      case SPI_CMD_SYS_GET_NV:
        MT_ProcessGetNV( pData );
        break;

      case SPI_CMD_SYS_TIME_ALIVE:
        // Time since last reset (seconds)
        tmp32 = osal_GetSystemClock() / 1000;
        // Convert to high byte first into temp buffer
        extAddr[0] = BREAK_UINT32( tmp32, 3 );
        extAddr[1] = BREAK_UINT32( tmp32, 2 );
        extAddr[2] = BREAK_UINT32( tmp32, 1 );
        extAddr[3] = BREAK_UINT32( tmp32, 0 );
        len = sizeof ( tmp32 );
        retValue = extAddr;
        break;

      case SPI_CMD_SYS_KEY_EVENT:
        // Translate between SPI values to device values
        if ( pData[1] & 0x01 )
          x |= HAL_KEY_SW_1;
        if ( pData[1] & 0x02 )
          x |= HAL_KEY_SW_2;
        if ( pData[1] & 0x04 )
          x |= HAL_KEY_SW_3;
        if ( pData[1] & 0x08 )
          x |= HAL_KEY_SW_4;
#if defined ( HAL_KEY_SW_5 )
        if ( pData[1] & 0x10 )
          x |= HAL_KEY_SW_5;
#endif
#if defined ( HAL_KEY_SW_6 )
        if ( pData[1] & 0x20 )
          x |= HAL_KEY_SW_6;
#endif
#if defined ( HAL_KEY_SW_7 )
        if ( pData[1] & 0x40 )
          x |= HAL_KEY_SW_7;
#endif
#if defined ( HAL_KEY_SW_8 )
        if ( pData[1] & 0x80 )
          x |= HAL_KEY_SW_8;
#endif
        ret = OnBoard_SendKeys( x, pData[0]  );
        len = 1;
        break;

      case SPI_CMD_SYS_HEARTBEAT:
        ret = ZSUCCESS;
        len = 1;
        break;

#if !defined ( NONWK )
      case SPI_CMD_SYS_APP_MSG:
        ret = MTProcessAppMsg( pData, msg[2] );
        len = 0;
        break;
#endif // NONWK

      case SPI_CMD_SYS_LED_CONTROL:
#if (defined HAL_LED) && (HAL_LED == TRUE)
        ret = MTProcessLedControl( pData );
        len = 1;
#endif
        break;

#ifdef MT_MAC_FUNC
      case SPI_CMD_MAC_INIT:
      case SPI_CMD_MAC_ASSOCIATE_REQ:
      case SPI_CMD_MAC_ASSOCIATE_RSP:
      case SPI_CMD_MAC_DISASSOCIATE_REQ:
      case SPI_CMD_MAC_DATA_REQ:
      case SPI_CMD_MAC_GET_REQ:
      case SPI_CMD_MAC_SET_REQ:
      case SPI_CMD_MAC_START_REQ:
      case SPI_CMD_MAC_SCAN_REQ:
      case SPI_CMD_MAC_RESET_REQ:
      case SPI_CMD_MAC_GTS_REQ:
      case SPI_CMD_MAC_ORPHAN_RSP:
      case SPI_CMD_MAC_RX_ENABLE_REQ:
      case SPI_CMD_MAC_SYNC_REQ:
      case SPI_CMD_MAC_POLL_REQ:
      case SPI_CMD_MAC_PURGE_REQ:
        MT_MacCommandProcessing( cmd , len , pData );
        break;
#endif

#ifdef MT_NWK_FUNC
      case SPI_CMD_NWK_INIT:
      case SPI_CMD_NLDE_DATA_REQ:
      case SPI_CMD_NLME_INIT_COORD_REQ:
      case SPI_CMD_NLME_PERMIT_JOINING_REQ:
      case SPI_CMD_NLME_JOIN_REQ:
      case SPI_CMD_NLME_LEAVE_REQ:
      case SPI_CMD_NLME_RESET_REQ:
      case SPI_CMD_NLME_RX_STATE_REQ:
      case SPI_CMD_NLME_GET_REQ:
      case SPI_CMD_NLME_SET_REQ:
      case SPI_CMD_NLME_NWK_DISC_REQ:
      case SPI_CMD_NLME_ROUTE_DISC_REQ:
      case SPI_CMD_NLME_DIRECT_JOIN_REQ:
      case SPI_CMD_NLME_ORPHAN_JOIN_REQ:
      case SPI_CMD_NLME_START_ROUTER_REQ:
        MT_NwkCommandProcessing( cmd , len , pData );
        break;
#endif

#ifdef MT_ZDO_FUNC
      case SPI_CMD_ZDO_AUTO_ENDDEVICEBIND_REQ:
      case SPI_CMD_ZDO_AUTO_FIND_DESTINATION_REQ:
      case SPI_CMD_ZDO_NWK_ADDR_REQ:
      case SPI_CMD_ZDO_IEEE_ADDR_REQ:
      case SPI_CMD_ZDO_NODE_DESC_REQ:
      case SPI_CMD_ZDO_POWER_DESC_REQ:
      case SPI_CMD_ZDO_SIMPLE_DESC_REQ:
      case SPI_CMD_ZDO_ACTIVE_EPINT_REQ:
      case SPI_CMD_ZDO_MATCH_DESC_REQ:
      case SPI_CMD_ZDO_COMPLEX_DESC_REQ:
      case SPI_CMD_ZDO_USER_DESC_REQ:
      case SPI_CMD_ZDO_END_DEV_BIND_REQ:
      case SPI_CMD_ZDO_BIND_REQ:
      case SPI_CMD_ZDO_UNBIND_REQ:
      case SPI_CMD_ZDO_MGMT_NWKDISC_REQ:
      case SPI_CMD_ZDO_MGMT_LQI_REQ:
      case SPI_CMD_ZDO_MGMT_RTG_REQ:
      case SPI_CMD_ZDO_MGMT_BIND_REQ:
      case SPI_CMD_ZDO_MGMT_DIRECT_JOIN_REQ:
      case SPI_CMD_ZDO_USER_DESC_SET:
      case SPI_CMD_ZDO_END_DEV_ANNCE:
      case SPI_CMD_ZDO_MGMT_LEAVE_REQ:
      case SPI_CMD_ZDO_MGMT_PERMIT_JOIN_REQ:
      case SPI_CMD_ZDO_SERVERDISC_REQ:
      case SPI_CMD_ZDO_NETWORK_START_REQ:
        MT_ZdoCommandProcessing( cmd , len , pData );
        break;
#endif

#if defined ( MT_AF_FUNC )
      case SPI_CMD_AF_INIT:
      case SPI_CMD_AF_REGISTER:
      case SPI_CMD_AF_SENDMSG:
        MT_afCommandProcessing( cmd , len , pData );
        break;
#endif

#if defined ( MT_SAPI_FUNC )
      case SPI_CMD_SAPI_SYS_RESET:
      case SPI_CMD_SAPI_START_REQ:
      case SPI_CMD_SAPI_BIND_DEVICE:
      case SPI_CMD_SAPI_ALLOW_BIND:
      case SPI_CMD_SAPI_SEND_DATA:
      case SPI_CMD_SAPI_READ_CFG:
      case SPI_CMD_SAPI_WRITE_CFG:
      case SPI_CMD_SAPI_GET_DEV_INFO:
      case SPI_CMD_SAPI_FIND_DEV:
      case SPI_CMD_SAPI_PMT_JOIN:
        ret = MT_sapiCommandProcessing( cmd , len , pData );
        if ( ret == 0xff )
          len = 0;
        else
          len = 1;
        break;
#endif

#if defined ( MT_USER_TEST_FUNC )
      case SPI_CMD_USER_TEST:
        MT_ProcessAppUserCmd( pData );
        break;
#endif

      default:
        break;
    }

    if ( len )
    {
      MT_BuildAndSendZToolResponse( (SPI_0DATA_MSG_LEN + len),
                                    (SPI_RESPONSE_BIT | cmd),
                                    len, retValue );
    }
  }
#endif // ZTOOL

#if (defined HAL_LED) && (HAL_LED == TRUE)
/***************************************************************************************************
 * @fn      MTProcessLedControl
 *
 * @brief
 *
 *   Process the LED Control Message
 *
 * @param   data - input serial buffer
 *
 * @return  status
 ***************************************************************************************************/
byte MTProcessLedControl( byte *pData )
{
  byte iLed;
  byte Led;
  byte iMode;
  byte Mode;

  iLed = *pData++;
  iMode = *pData;

  if ( iLed == 1 )
    Led = HAL_LED_1;
  else if ( iLed == 2 )
    Led = HAL_LED_2;
  else if ( iLed == 3 )
    Led = HAL_LED_3;
  else if ( iLed == 4 )
    Led = HAL_LED_4;
  else if ( iLed == 0xFF )
    Led = HAL_LED_ALL;
  else
    Led = 0;

  if ( iMode == 0 )
    Mode = HAL_LED_MODE_OFF;
  else if ( iMode == 1 )
    Mode = HAL_LED_MODE_ON;
  else if ( iMode == 2 )
    Mode = HAL_LED_MODE_BLINK;
  else if ( iMode == 3 )
    Mode = HAL_LED_MODE_FLASH;
  else if ( iMode == 4 )
    Mode = HAL_LED_MODE_TOGGLE;
  else
    Led = 0;

  if ( Led != 0 )
  {
    HalLedSet (Led, Mode );
    return ( ZSuccess );
  }
  else
    return ( ZFailure );
}
#endif // HAL_LED

#if !defined ( NONWK )
/*********************************************************************
 * @fn      MTProcessAppMsg
 *
 * @brief
 *
 *   Process the User App Message
 *
 * @param   data - input serial buffer
 * @param   len - data length
 *
 * @return  status
 */
byte MTProcessAppMsg( byte *pData, byte len )
{
  byte ret = ZFailure;
  byte endpoint;
  endPointDesc_t *epDesc;
  mtSysAppMsg_t *msg;

  // Get the endpoint and skip past it.
  endpoint = *pData++;
  len--;

  // Look up the endpoint
  epDesc = afFindEndPointDesc( endpoint );

  if ( epDesc )
  {
    // Build and send the message to the APP
    msg = (mtSysAppMsg_t *)osal_msg_allocate( sizeof( mtSysAppMsg_t ) + len );
    if ( msg )
    {
      msg->hdr.event = MT_SYS_APP_MSG;
      msg->endpoint = endpoint;
      msg->appDataLen = len;
      msg->appData = (uint8*)(msg+1);

      osal_memcpy( msg->appData, pData, len );

      osal_msg_send( *(epDesc->task_id), (uint8 *)msg );

      ret = ZSuccess;
    }
  }

  return ret;
}
#endif // NONWK

#if defined ( ZTOOL_PORT )
/*********************************************************************
 * @fn      MTProcessAppRspMsg
 *
 * @brief
 *
 *   Process the User App Response Message
 *
 * @param   data - output serial buffer.  The first byte must be the
 *          endpoint that send this message.
 * @param   len - data length
 *
 * @return  none
 */
void MTProcessAppRspMsg( byte *pData, byte len )
{
  // Send out Reset Response message
  MT_BuildAndSendZToolResponse( (SPI_0DATA_MSG_LEN + len),
                                (SPI_RESPONSE_BIT | SPI_CMD_SYS_APP_MSG),
                                len, pData );
}
#endif // ZTOOL_PORT


#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
#if defined ( MT_USER_TEST_FUNC )
/*********************************************************************
 * @fn      MT_ProcessAppUserCmd
 *
 * @brief
 *
 *   Temp function for testing
 *
 * @param   data - received message
 *
 * @return  void
 */
void MT_ProcessAppUserCmd( byte *pData) 						
{
  uint16 app_cmd;
  byte srcEp;
  uint16 param1;
  uint16 param2;
  byte len;
  uint16 ret;

  ret = INVALID_TASK;     //should be changed later

  srcEp = *pData++;

  app_cmd = BUILD_UINT16( pData[1] , pData[0] );
  pData = pData + sizeof( uint16 );

  param1 = BUILD_UINT16( pData[1] , pData[0] );
  pData = pData + sizeof( uint16 );

  param2 = BUILD_UINT16( pData[1] , pData[0] );

  len = SPI_RESP_MSG_LEN_DEFAULT;


  switch ( app_cmd )
  {

#if defined (APP_TGEN)
    case TGEN_START:
      TrafficGenApp_SendCmdMSG( param1, param2, TRAFFICGENAPP_CMD_START );
      ret = ZSUCCESS;
      break;

    case TGEN_STOP:
      TrafficGenApp_SendCmdMSG( param1, param2, TRAFFICGENAPP_CMD_STOP );
      ret = ZSUCCESS;
      break;

    case TGEN_COUNT:
      ret = TrafficGenApp_CountPkt( param1, param2 );
      return;		// so that spi_resp is not sent...
      //ret = ZSUCCESS;
      break;				
#endif

#if defined (NWK_TEST)
    case HW_TEST:
      HwApp_Start( HI_UINT16(param1), LO_UINT16(param1), HI_UINT16(param2),
                    1000, LO_UINT16(param2), 3, 0 );
      break;

    case HW_DISPLAY_RESULT:
      HwApp_TestInfo();
      break;

    case HW_SEND_STATUS:
      HwApp_SendStats();
      break;
#endif

#if defined( APP_TP ) || defined ( APP_TP2 )
  #if defined( APP_TP )
    case TP_SEND_NODATA:
      ret = TestProfileApp_SendNoData( srcEp, (byte)param1 );
      break;
  #endif // APP_TP
			
    case TP_SEND_BUFFERTEST:
      ret = TestProfileApp_SendBufferReq( srcEp, (byte)param1 );
      break;
			
  #if defined( APP_TP )
    case TP_SEND_UINT8:
      ret = TestProfileApp_SendUint8( srcEp, (byte)param1 );
      break;

    case TP_SEND_INT8:
      ret = TestProfileApp_SendInt8( srcEp, (byte)param1 );
      break;

    case TP_SEND_UINT16:
      ret = TestProfileApp_SendUint16( srcEp, (byte)param1 );
      break;

    case TP_SEND_INT16:
      ret = TestProfileApp_SendInt16( srcEp, (byte)param1 );
      break;

    case TP_SEND_SEMIPREC:
      ret = TestProfileApp_SendSemiPrec( srcEp, (byte)param1 );
      break;

    case TP_SEND_FREEFORM:
      ret = TestProfileApp_SendFreeFormReq( srcEp, (byte)param1 );
      break;
			
  #else // APP_TP
    case TP_SEND_FREEFORM:
      ret = TestProfileApp_SendFreeFormReq(srcEp, (byte)param1, (byte)param2);
      break;
  #endif
			
  #if defined( APP_TP )
    case TP_SEND_ABS_TIME:
      ret = TestProfileApp_SendAbsTime( srcEp, (byte)param1 );
      break;

    case TP_SEND_REL_TIME:
      ret = TestProfileApp_SendRelativeTime( srcEp, (byte)param1 );
      break;

    case TP_SEND_CHAR_STRING:
      ret = TestProfileApp_SendCharString( srcEp, (byte)param1 );
      break;

    case TP_SEND_OCTET_STRING:
      ret = TestProfileApp_SendOctetString( srcEp, (byte)param1 );
      break;		
  #endif // APP_TP
				
    case TP_SET_DSTADDRESS:			
      ret = TestProfileApp_SetDestAddress(HI_UINT16(param1), LO_UINT16(param1), param2);
      break;	

  #if defined( APP_TP2 )
    case TP_SEND_BUFFER_GROUP:
      ret = TestProfileApp_SendBufferGroup( srcEp, (byte)param1 );
      break;
  #endif // APP_TP

    case TP_SEND_BUFFER:
      ret = TestProfileApp_SendBuffer( srcEp, (byte)param1 );
      break;
				
  #if defined( APP_TP )
    case TP_SEND_MULT_KVP_8BIT:
      TestProfileApp_SendMultiKVP_8bit( srcEp, (byte)param1 );
      ret = ZSuccess;
      break;

    case TP_SEND_MULT_KVP_16BIT:
      TestProfileApp_SendMultiKVP_16bit( srcEp, (byte)param1 );
      ret = ZSuccess;
      break;

    case TP_SEND_MULT_KVP_TIME:
      TestProfileApp_SendMultiKVP_Time( srcEp, (byte)param1 );
      ret = ZSuccess;
      break;

    case TP_SEND_MULT_KVP_STRING:
      TestProfileApp_SendMultiKVP_String( srcEp, (byte)param1 );
      ret = ZSuccess;
      break;

    case TP_SEND_MULTI_KVP_STR_TIME:
      ret = ZSuccess;
      TestProfileApp_SendMultiKVP_String_Time( srcEp, (byte)param1 );
      break;
  #endif // APP_TP
				
    case TP_SEND_COUNTED_PKTS:
      TestProfileApp_SendCountedPktsReq(HI_UINT16(param1), LO_UINT16(param1), param2);
      ret = ZSuccess;
      break;

    case TP_SEND_RESET_COUNTER:
      TestProfileApp_CountedPakts_ResetCounterReq( (byte)param1 );
      ret = ZSuccess;
      break;

    case TP_SEND_GET_COUNTER:
      TestProfileApp_CountedPakts_GetCounterReq( srcEp, (byte)param1 );
      ret = ZSuccess;
      break;
				
    case TP_SET_PERMIT_JOIN:
  #if defined ( RTR_NWK )
      NLME_PermitJoiningRequest( (byte)param1 );
      ret = ZSuccess;
  #else
      ret = ZFailure;
  #endif
      break;

  #if defined ( APP_TP2 )
    case TP_ADD_GROUP:
      ret = TestProfileApp_SetGroup( srcEp, param1 );
      break;

    case TP_REMOVE_GROUP:
      ret = TestProfileApp_RemoveGroup( srcEp, param1 );
      break;

    case TP_SEND_UPDATE_KEY:
      ret = TestProfileApp_UpdateKey( srcEp, (uint8)param1 );
      break;

    case TP_SEND_SWITCH_KEY:
      ret = TestProfileApp_SwitchKey(  srcEp, (uint8)param1 );
      break;
			
    case TP_SEND_BUFFERTEST_GROUP:
      ret = TestProfileApp_SendBufferGroupReq( srcEp, (byte)param1 );
      break;

    case TP_SEND_ROUTE_DISC_REQ:
      ret = TestProfileApp_SendRouteDiscReq( srcEp, param1,
                                  HI_UINT16( param2 ), LO_UINT16( param2 ) );
      break;

    case TP_SEND_ROUTE_DISCOVERY:
#if defined ( RTR_NWK )
      ret = TestProfileApp_SendRouteDiscovery( param1,
                                  HI_UINT16( param2 ), LO_UINT16( param2 ) );
#endif
      break;

  #endif // APP_TP2

#endif  // APP_TP || APP_TP2

#if defined ( OSAL_TOTAL_MEM )
    case OSAL_MEM_STACK_HIGH_WATER:
    case OSAL_MEM_HEAP_HIGH_WATER:
      if ( app_cmd == OSAL_MEM_STACK_HIGH_WATER)
        param1 = osal_stack_used();
      else
        param1 = osal_heap_high_water();
      pData[0] = HI_UINT16( param1 );
      pData[1] = LO_UINT16( param1 );

      // The pData for this response will only send one byte,
      // so we are going to call it twice.
      //        MT_BuildAndSendZToolResponse( (SPI_0DATA_MSG_LEN + sizeof( uint16 )),
      //                                    (SPI_CMD_USER_TEST | SPI_RESPONSE_BIT),
      //                                    sizeof( uint16 ), pData );
      MT_SendSPIRespMsg( pData[0], SPI_CMD_USER_TEST, SPI_RESP_MSG_LEN_DEFAULT, 1);
      MT_SendSPIRespMsg( pData[1], SPI_CMD_USER_TEST, SPI_RESP_MSG_LEN_DEFAULT, 1);
      return;
#endif

#if defined ( APP_DEBUG )
    case DEBUG_GET:
      DebugApp_SendQuery( param1 );
      ret = ZSUCCESS;
      break;
#endif

#if defined ( APP_TP2 )
    case TP_SEND_BCAST_RSP:
      ret = TestProfileApp_SendBcastRsp( srcEp, (byte)param1 );
      break;
#endif
			
    default:
      break;
  }

  MT_SendSPIRespMsg( ( byte )ret, SPI_CMD_USER_TEST, len, 1);

}
#endif // MT_USER_TEST_FUNC
#endif // ZTOOL

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/***************************************************************************************************
 * @fn      MT_RAMRead
 *
 * @brief
 *
 *   Process Serial Message.
 *
 * @param   UINT16 addr - address to read from
 * @param   pData - pointer to buffer to put read data
 *
 * @return  ZSuccess or ZFailure
 *
 * @MT SPI_CMD_SYS_RAM_READ
 *
 ***************************************************************************************************/
byte MT_RAMRead( UINT16 addr, byte *pData  )
{
  byte *pAddr;

  if ( IS_MEM_VALID( addr ) )
  {
    pAddr = (byte *)addr;
    *pData = *pAddr;
    return ( (byte)ZSuccess );
  }
  else
  {
    *pData = 0;
    return ( (byte)ZFailure );
  }
}
#endif

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/***************************************************************************************************
 * @fn      MT_RAMWrite
 *
 * @brief
 *
 *   Process Serial Message.
 *
 * @param   UINT16 addr - address to write at
 * @param   byte val  - values to fill in the above address and the next
 *
 * @return  ZSuccess or ZFailure
 *
 * @MT SPI_CMD_SYS_RAM_WRITE
 *
 ***************************************************************************************************/
byte MT_RAMWrite( UINT16 addr, byte val )
{
  if ( IS_MEM_VALID( addr ) )
  {
    *((byte*)(addr)) = val;
    return ( (byte)ZSuccess );
  }
  else
    return ( (byte)ZFailure );
}
#endif

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/***************************************************************************************************
 * @fn      MT_SetDebugThreshold
 *
 * @brief
 *
 *   Set Debug Threshold for software components.
 *
 * @param   comp_id   - software component ID.
 * @param   threshold - threshold value for reporting debug messages.
 *
 * @return  ZSuccess
 *
 * @MT SPI_CMD_SYS_SET_DEBUG_THRESHOLD
 *
 ***************************************************************************************************/
byte MT_SetDebugThreshold( byte compID, byte threshold )
{
  // *** RKJ - for now if we get any threshold message, set it on
  debugThreshold = threshold;
  debugCompId = compID;

  return ( (byte)ZSuccess );
}
#endif

/***************************************************************************************************
 * @fn      MT_Reset
 *
 * @brief
 *
 *   Reset/reprogram the device.
 *
 * @param   typID: 0=reset, 1=serial bootloader
 *
 * @return  void
 *
 * @MT SPI_CMD_SYS_RESET
 ***************************************************************************************************/
void MT_Reset( byte typID )
{
  if ( typID )
  {
    // Jump to bootloader
    BootLoader();
  }
  else
  {
    // Restart this program
    SystemReset();
  }
}

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
/***************************************************************************************************
 * @fn      MT_SendErrorNotification
 *
 * @brief
 *
 *   Send Error Notofication Message to Test Tool.
 *
 * @param   byte err
 *
 * @return  void
 ***************************************************************************************************/
void MT_SendErrorNotification( byte err )
{
  MT_BuildAndSendZToolResponse( (SPI_0DATA_MSG_LEN + 1),
                              (SPI_RESPONSE_BIT | SPI_CMD_SYS_RAM_WRITE),
                              1, &err );
}
#endif

/***************************************************************************************************
 * @fn      MT_ReverseBytes
 *
 * @brief
 *
 *   Reverses bytes within an array
 *
 * @param   data - ptr to data buffer to reverse
 * @param    len - number of bytes in buffer
 *
 * @return  void
 ***************************************************************************************************/
void MT_ReverseBytes( byte *pData, byte len )
{
  byte i,j;
  byte temp;

  for ( i = 0, j = len-1; len > 1; len-=2 ) {
    temp = pData[i];
    pData[i++] = pData[j];
    pData[j--] = temp;
  }
}

/***************************************************************************************************
 * @fn      MT_SendSPIRespMsg
 *
 * @brief
 *
 *   This function is used to process messages in the queue
 *
 * @param   none
 *
 * @return  void
 ***************************************************************************************************/
void MT_SendSPIRespMsg( byte ret, uint16 cmd_id, byte msgLen, byte respLen)
{
  byte *msgPtr;

  msgPtr = osal_mem_alloc( msgLen );
  if ( msgPtr )
  {
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
    MT_BuildSPIMsg( (SPI_RESPONSE_BIT | cmd_id), msgPtr, respLen, &ret );
    HalUARTWrite ( SPI_MGR_DEFAULT_PORT, msgPtr, msgLen );
#endif

    osal_mem_free( msgPtr );
  }
}

#endif  // MT_TASK

/***************************************************************************************************
***************************************************************************************************/
