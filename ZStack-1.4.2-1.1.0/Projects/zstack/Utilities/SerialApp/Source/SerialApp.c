/*********************************************************************
    Filename:       SerialApp.c
    Revised:        $Date: 2007-03-13 14:24:09 -0700 (Tue, 13 Mar 2007) $
    Revision:       $Revision: 13747 $

    Description -   Serial Transfer Application (no Profile).
				
    This sample application is basically a cable replacement
    and it should be customized for your application. A PC
    (or other device) sends data to the serial port on this
    application's device.  This device transmits the message
    to another device with the same application running. The
    other device receives the over-the-air message and sends
    it to a PC (or other device) connected to its serial port.
				
    This application doesn't have a profile, so it handles everything directly.

    Key control:
      SW1:
      SW2:  initiates end device binding
      SW3:
      SW4:  initiates a match description request

    Copyright (c) 2007 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "SerialApp.h"
#include "AF.h"
#include "OSAL_Tasks.h"
#include "ZDApp.h"

#include "hal_drivers.h"
#include "hal_key.h"
#if defined ( LCD_SUPPORTED )
  #include "hal_lcd.h"
#endif
#include "hal_led.h"
#include "hal_uart.h"
#include "OnBoard.h"

/*********************************************************************
 * MACROS
 */

/* See the important comment by the otaBuf declaration below concerning the
 * mandatory use of this macro vice directly invoking osal_mem_free().
 */
#define FREE_OTABUF() { \
  if ( otaBuf ) \
  { \
    osal_mem_free( otaBuf ); \
  } \
  if ( otaBuf2 ) \
  { \
    SerialApp_SendData( otaBuf2, otaLen2 ); \
    otaBuf2 = NULL; \
  } \
  else \
  { \
    otaBuf = NULL; \
  } \
}

/*********************************************************************
 * CONSTANTS
 */

#if !defined( SERIAL_APP_PORT )
  #define SERIAL_APP_PORT  0
#endif

#if !defined( SERIAL_APP_BAUD )
  // CC2430 only allows 38.4k or 115.2k.
  #define SERIAL_APP_BAUD  HAL_UART_BR_38400
  //#define SERIAL_APP_BAUD  HAL_UART_BR_115200
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( SERIAL_APP_THRESH )
  #define SERIAL_APP_THRESH  48
#endif

#if !defined( SERIAL_APP_RX_MAX )
  #if (defined( HAL_UART_DMA )) && HAL_UART_DMA
    #define SERIAL_APP_RX_MAX  128
  #else
    /* The generic safe Rx minimum is 48, but if you know your PC App will not
     * continue to send more than a byte after receiving the ~CTS, lower max
     * here and safe min in _hal_uart.c to just 8.
     */
    #define SERIAL_APP_RX_MAX  64
  #endif
#endif

#if !defined( SERIAL_APP_TX_MAX )
  #if (defined( HAL_UART_DMA )) && HAL_UART_DMA
  #define SERIAL_APP_TX_MAX  128
  #else
    #define SERIAL_APP_TX_MAX  64
  #endif
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( SERIAL_APP_IDLE )
  #define SERIAL_APP_IDLE  6
#endif

// This is the desired byte count per OTA message.
#if !defined( SERIAL_APP_RX_CNT )
  #if (defined( HAL_UART_DMA )) && HAL_UART_DMA
    #define SERIAL_APP_RX_CNT  80
  #else
    #define SERIAL_APP_RX_CNT  6
  #endif
#endif

// Loopback Rx bytes to Tx for thruput testing.
#if !defined( SERIAL_APP_LOOPBACK )
  #define SERIAL_APP_LOOPBACK  FALSE
#endif

#if SERIAL_APP_LOOPBACK
  #define SERIALAPP_TX_RTRY_EVT      0x0010
  #define SERIALAPP_TX_RTRY_TIMEOUT  250
#endif

#define SERIAL_APP_RSP_CNT  4

// This list should be filled with Application specific Cluster IDs.
const cId_t SerialApp_ClusterList[SERIALAPP_MAX_CLUSTERS] =
{
  SERIALAPP_CLUSTERID1,
  SERIALAPP_CLUSTERID2
};

const SimpleDescriptionFormat_t SerialApp_SimpleDesc =
{
  SERIALAPP_ENDPOINT,              //  int   Endpoint;
  SERIALAPP_PROFID,                //  uint16 AppProfId[2];
  SERIALAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SERIALAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SERIALAPP_FLAGS,                 //  int   AppFlags:4;
  SERIALAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)SerialApp_ClusterList,  //  byte *pAppInClusterList;
  SERIALAPP_MAX_CLUSTERS,          //  byte  AppNumOutClusters;
  (cId_t *)SerialApp_ClusterList   //  byte *pAppOutClusterList;
};

const endPointDesc_t SerialApp_epDesc =
{
  SERIALAPP_ENDPOINT,
 &SerialApp_TaskID,
  (SimpleDescriptionFormat_t *)&SerialApp_SimpleDesc,
  noLatencyReqs
};

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

uint8 SerialApp_TaskID;    // Task ID for internal task/event processing.

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static uint8 SerialApp_MsgID;

static afAddrType_t SerialApp_DstAddr;
static uint8 SerialApp_SeqTx;
/* This local global is used to hold the last outgoing OTA data for retries
 * until the process expires or the handshake response is received. Do not
 * free with osal_mem_free() - only use the above FREE_OTABUF() macro.
 */
static uint8 *otaBuf;
static uint8 otaLen;
static uint8 *otaBuf2;
static uint8 otaLen2;
static uint8 rtryCnt;

static uint8 SerialApp_SeqRx;
static afAddrType_t SerialApp_RspDstAddr;
static uint8 rspBuf[ SERIAL_APP_RSP_CNT ];

#if SERIAL_APP_LOOPBACK
static uint8 rxLen;
static uint8 rxBuf[SERIAL_APP_RX_CNT];
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void SerialApp_HandleKeys( uint8 shift, uint8 keys );
static void SerialApp_ProcessMSGCmd( afIncomingMSGPacket_t *pkt );
static void SerialApp_SendData( uint8 *buf, uint8 len );
#if SERIAL_APP_LOOPBACK
static void rxCB_Loopback( uint8 port, uint8 event );
#else
static void rxCB( uint8 port, uint8 event );
#endif

/*********************************************************************
 * @fn      SerialApp_Init
 *
 * @brief   This is called during OSAL tasks' initialization.
 *
 * @param   task_id - the Task ID assigned by OSAL.
 *
 * @return  none
 */
void SerialApp_Init( uint8 task_id )
{
  halUARTCfg_t uartConfig;

  SerialApp_MsgID = 0x00;
  SerialApp_SeqRx = 0xC3;
  SerialApp_TaskID = task_id;

  SerialApp_DstAddr.endPoint = 0;
  SerialApp_DstAddr.addr.shortAddr = 0;
  SerialApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;

  SerialApp_RspDstAddr.endPoint = 0;
  SerialApp_RspDstAddr.addr.shortAddr = 0;
  SerialApp_RspDstAddr.addrMode = (afAddrMode_t)AddrNotPresent;

  afRegister( (endPointDesc_t *)&SerialApp_epDesc );

  RegisterForKeys( task_id );

  uartConfig.configured           = TRUE;              // 2430 don't care.
  uartConfig.baudRate             = SERIAL_APP_BAUD;
  uartConfig.flowControl          = TRUE;
  uartConfig.flowControlThreshold = SERIAL_APP_THRESH;
  uartConfig.rx.maxBufSize        = SERIAL_APP_RX_MAX;
  uartConfig.tx.maxBufSize        = SERIAL_APP_TX_MAX;
  uartConfig.idleTimeout          = SERIAL_APP_IDLE;   // 2430 don't care.
  uartConfig.intEnable            = TRUE;              // 2430 don't care.
#if SERIAL_APP_LOOPBACK
  uartConfig.callBackFunc         = rxCB_Loopback;
#else
  uartConfig.callBackFunc         = rxCB;
#endif
  HalUARTOpen (SERIAL_APP_PORT, &uartConfig);

#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "SerialApp2", HAL_LCD_LINE_2 );
#endif
}

/*********************************************************************
 * @fn      SerialApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events   - Bit map of events to process.
 *
 * @return  none
 */
UINT16 SerialApp_ProcessEvent( uint8 task_id, UINT16 events )
{
  if ( events & SYS_EVENT_MSG )
  {
    zAddrType_t *dstAddr;
    ZDO_NewDstAddr_t *ZDO_NewDstAddr;
    afIncomingMSGPacket_t *MSGpkt;

    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(
                                                          SerialApp_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
      case KEY_CHANGE:
        SerialApp_HandleKeys( ((keyChange_t *)MSGpkt)->state,
                              ((keyChange_t *)MSGpkt)->keys );
        break;

      case AF_INCOMING_MSG_CMD:
        SerialApp_ProcessMSGCmd( MSGpkt );
        break;

      case ZDO_NEW_DSTADDR:
        ZDO_NewDstAddr = (ZDO_NewDstAddr_t *)MSGpkt;
        SerialApp_DstAddr.endPoint = ZDO_NewDstAddr->dstAddrDstEP;
        dstAddr = &ZDO_NewDstAddr->dstAddr;
        SerialApp_DstAddr.addrMode = (afAddrMode_t)dstAddr->addrMode;
        SerialApp_DstAddr.addr.shortAddr = dstAddr->addr.shortAddr;
        HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
        break;

      default:
        break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );  // Release the memory.
    }

    // Return unprocessed events
    return ( events ^ SYS_EVENT_MSG );
  }

  if ( events & SERIALAPP_MSG_SEND_EVT )
  {
    SerialApp_SendData( otaBuf, otaLen );

    return ( events ^ SERIALAPP_MSG_SEND_EVT );
  }

  if ( events & SERIALAPP_MSG_RTRY_EVT )
  {
    if ( --rtryCnt )
    {
      AF_DataRequest( &SerialApp_DstAddr,
                      (endPointDesc_t *)&SerialApp_epDesc,
                       SERIALAPP_CLUSTERID1, otaLen, otaBuf,
                      &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS );
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_MSG_RTRY_EVT,
                                            SERIALAPP_MSG_RTRY_TIMEOUT );
    }
    else
    {
      FREE_OTABUF();
    }

    return ( events ^ SERIALAPP_MSG_RTRY_EVT );
  }

  if ( events & SERIALAPP_RSP_RTRY_EVT )
  {
    afStatus_t stat = AF_DataRequest( &SerialApp_RspDstAddr,
                                      (endPointDesc_t *)&SerialApp_epDesc,
                                       SERIALAPP_CLUSTERID2,
                                       SERIAL_APP_RSP_CNT, rspBuf,
                                      &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS );

    if ( stat != afStatus_SUCCESS )
    {
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_RSP_RTRY_EVT,
                                            SERIALAPP_RSP_RTRY_TIMEOUT );
    }

    return ( events ^ SERIALAPP_RSP_RTRY_EVT );
  }

#if SERIAL_APP_LOOPBACK
  if ( events & SERIALAPP_TX_RTRY_EVT )
  {
    if ( rxLen )
    {
      if ( !HalUARTWrite( SERIAL_APP_PORT, rxBuf, rxLen ) )
      {
        osal_start_timerEx( SerialApp_TaskID, SERIALAPP_TX_RTRY_EVT,
                                              SERIALAPP_TX_RTRY_TIMEOUT );
      }
      else
      {
        rxLen = 0;
      }
    }

    return ( events ^ SERIALAPP_TX_RTRY_EVT );
  }
#endif

  return ( 0 );  // Discard unknown events.
}

/*********************************************************************
 * @fn      SerialApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys  - bit field for key events.
 *
 * @return  none
 */
void SerialApp_HandleKeys( uint8 shift, uint8 keys )
{
  if ( shift )
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }
    if ( keys & HAL_KEY_SW_2 )
    {
    }
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      // Initiate an End Device Bind Request for the mandatory endpoint
      ZDApp_SendEndDeviceBindReq( SerialApp_epDesc.endPoint );
    }

    if ( keys & HAL_KEY_SW_3 )
    {
    }

    if ( keys & HAL_KEY_SW_4 )
    {
      // Initiate a Match Description Request (Service Discovery)
      //  for the mandatory endpoint
      ZDApp_AutoFindDestination( SerialApp_epDesc.endPoint );
    }
  }
}

/*********************************************************************
 * @fn      SerialApp_ProcessMSGCmd
 *
 * @brief   Data message processor callback. This function processes
 *          any incoming data - probably from other devices. Based
 *          on the cluster ID, perform the intended action.
 *
 * @param   pkt - pointer to the incoming message packet
 *
 * @return  TRUE if the 'pkt' parameter is being used and will be freed later,
 *          FALSE otherwise.
 */
void SerialApp_ProcessMSGCmd( afIncomingMSGPacket_t *pkt )
{
  uint8 stat;
  uint8 seqnb;
  uint8 delay;

  switch ( pkt->clusterId )
  {
  // A message with a serial data block to be transmitted on the serial port.
  case SERIALAPP_CLUSTERID1:
    seqnb = pkt->cmd.Data[0];

    // Keep message if not a repeat packet
    if ( (seqnb > SerialApp_SeqRx) ||                    // Normal
        ((seqnb < 0x80 ) && ( SerialApp_SeqRx > 0x80)) ) // Wrap-around
    {
      // Transmit the data on the serial port.
      if ( HalUARTWrite( SERIAL_APP_PORT, pkt->cmd.Data+1,
                                         (pkt->cmd.DataLength-1) ) )
      {
        // Save for next incoming message
        SerialApp_SeqRx = seqnb;

        stat = OTA_SUCCESS;
      }
      else
      {
        stat = OTA_SER_BUSY;
      }
    }
    else
    {
      stat = OTA_DUP_MSG;
    }

    // Select approproiate OTA flow-control delay.
    delay = (stat == OTA_SER_BUSY) ? SERIALAPP_NAK_DELAY : SERIALAPP_ACK_DELAY;

    // Build & send OTA response message.
    rspBuf[0] = stat;
    rspBuf[1] = seqnb;
    rspBuf[2] = LO_UINT16( delay );
    rspBuf[3] = HI_UINT16( delay );
    stat = AF_DataRequest( &(pkt->srcAddr), (endPointDesc_t*)&SerialApp_epDesc,
                            SERIALAPP_CLUSTERID2, SERIAL_APP_RSP_CNT , rspBuf,
                           &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS );

    if ( stat != afStatus_SUCCESS )
    {
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_RSP_RTRY_EVT,
                                            SERIALAPP_RSP_RTRY_TIMEOUT );

      // Store the address for the timeout retry.
      osal_memcpy(&SerialApp_RspDstAddr, &(pkt->srcAddr), sizeof( afAddrType_t ));
    }
    break;

  // A response to a received serial data block.
  case SERIALAPP_CLUSTERID2:
    if ( (pkt->cmd.Data[1] == SerialApp_SeqTx) &&
        ((pkt->cmd.Data[0] == OTA_SUCCESS) ||
         (pkt->cmd.Data[0] == OTA_DUP_MSG)) )
    {
      // Remove timeout waiting for response from other device.
      osal_stop_timerEx( SerialApp_TaskID, SERIALAPP_MSG_RTRY_EVT );
      FREE_OTABUF();
    }
    else
    {
      delay = BUILD_UINT16( pkt->cmd.Data[2], pkt->cmd.Data[3] );
      // Re-start timeout according to delay sent from other device.
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_MSG_RTRY_EVT, delay );
    }
    break;

    default:
      break;
  }
}

/*********************************************************************
 * @fn      SerialApp_SendData
 *
 * @brief   Send data OTA.
 *
 * @param   none
 *
 * @return  none
 */
static void SerialApp_SendData( uint8 *buf, uint8 len )
{
  afStatus_t stat;

  // Pre-pend sequence number to the start of the Rx buffer.
  *buf = ++SerialApp_SeqTx;

  otaBuf = buf;
  otaLen = len+1;

  stat = AF_DataRequest( &SerialApp_DstAddr,
                         (endPointDesc_t *)&SerialApp_epDesc,
                          SERIALAPP_CLUSTERID1,
                          otaLen, otaBuf,
                          &SerialApp_MsgID, 0, AF_DEFAULT_RADIUS );

  if ( (stat == afStatus_SUCCESS) || (stat == afStatus_MEM_FAIL) )
  {
    osal_start_timerEx( SerialApp_TaskID, SERIALAPP_MSG_RTRY_EVT,
                      SERIALAPP_MSG_RTRY_TIMEOUT );
    rtryCnt = SERIALAPP_MAX_RETRIES;
  }
  else
  {
    FREE_OTABUF();
  }
}

#if SERIAL_APP_LOOPBACK
/*********************************************************************
 * @fn      rxCB_Loopback
 *
 * @brief   Process UART Rx event handling.
 *          May be triggered by an Rx timer expiration - less than max
 *          Rx bytes have arrived within the Rx max age time.
 *          May be set by failure to alloc max Rx byte-buffer for the DMA Rx -
 *          system resources are too low, so set flow control?
 *
 * @param   none
 *
 * @return  none
 */
static void rxCB_Loopback( uint8 port, uint8 event )
{

  if ( rxLen )
  {
    if ( !HalUARTWrite( SERIAL_APP_PORT, rxBuf, rxLen ) )
    {
      osal_start_timerEx( SerialApp_TaskID, SERIALAPP_TX_RTRY_EVT,
                                            SERIALAPP_TX_RTRY_TIMEOUT );
      return;
    }
    else
    {
      osal_stop_timerEx( SerialApp_TaskID, SERIALAPP_TX_RTRY_EVT );
    }
  }

  // HAL UART Manager will turn flow control back on if it can after read.
  if ( !(rxLen = HalUARTRead( port, rxBuf, SERIAL_APP_RX_CNT )) )
  {
    return;
  }

  if ( HalUARTWrite( SERIAL_APP_PORT, rxBuf, rxLen ) )
  {
    rxLen = 0;
  }
  else
  {
    osal_start_timerEx( SerialApp_TaskID, SERIALAPP_TX_RTRY_EVT,
                                          SERIALAPP_TX_RTRY_TIMEOUT );
  }
}

#else

/*********************************************************************
 * @fn      rxCB
 *
 * @brief   Process UART Rx event handling.
 *          May be triggered by an Rx timer expiration - less than max
 *          Rx bytes have arrived within the Rx max age time.
 *          May be set by failure to alloc max Rx byte-buffer for the DMA Rx -
 *          system resources are too low, so set flow control?
 *
 * @param   none
 *
 * @return  none
 */
static void rxCB( uint8 port, uint8 event )
{
  uint8 *buf, len;

  /* While awaiting retries/response, only buffer 1 next buffer: otaBuf2.
   * If allow the DMA Rx to continue to run, allocating Rx buffers, the heap
   * will become so depleted that an incoming OTA response cannot be received.
   * When the Rx data available is not read, the DMA Rx Machine automatically
   * sets flow control off - it is automatically re-enabled upon Rx data read.
   * When the back-logged otaBuf2 is sent OTA, an Rx data read is scheduled.
   */
  if ( otaBuf2 )
  {
    return;
  }

  if ( !(buf = osal_mem_alloc( SERIAL_APP_RX_CNT )) )
  {
    return;
  }

  /* HAL UART Manager will turn flow control back on if it can after read.
   * Reserve 1 byte for the 'sequence number'.
   */
  len = HalUARTRead( port, buf+1, SERIAL_APP_RX_CNT-1 );

  if ( !len )  // Length is not expected to ever be zero.
  {
    osal_mem_free( buf );
    return;
  }

  /* If the local global otaBuf is in use, then either the response handshake
   * is being awaited or retries are being attempted. When the wait/retries
   * process has been exhausted, the next OTA msg will be attempted from
   * otaBuf2, if it is not NULL.
   */
  if ( otaBuf )
  {
    otaBuf2 = buf;
    otaLen2 = len;
  }
  else
  {
    otaBuf = buf;
    otaLen = len;
    /* Don't call SerialApp_SendData() from here in the callback function.
     * Set the event so SerialApp_SendData() runs during this task's time slot.
     */
    osal_set_event( SerialApp_TaskID, SERIALAPP_MSG_SEND_EVT );
  }
}
#endif

/*********************************************************************
*********************************************************************/
