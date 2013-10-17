/*********************************************************************
  Filename:       TransmitApp.c
  Revised:        $Date: 2007-05-21 14:13:59 -0700 (Mon, 21 May 2007) $
  Revision:       $Revision: 14372 $

  Description:
				  - Transmit Application (no Profile).
				
          This application will send a data packet to another
          tranmitApp device as fast as it can.  The receiving
          transmitApp device will calculate the following transmit
          rate statistics:
            -	Number bytes in the last second
            -	Number of seconds running
            -	Average number of bytes per second
            -	Number of packets received.

          The application will send one message and as soon as it
          receives the confirmation for that message it will send
          the next message.

          If you would like a delay between messages
          define TRANSMITAPP_DELAY_SEND and set the delay amount
          in TRANSMITAPP_SEND_DELAY.

          TransmitApp_MaxDataLength defines the message size

          Set TRANSMITAPP_TX_OPTIONS to AF_MSG_ACK_REQUEST to send
          the message expecting an APS ACK, this will decrease your
          throughput.  Set TRANSMITAPP_TX_OPTIONS to 0 for no
          APS ACK.

          This applications doesn't have a profile, so it handles
          everything directly - itself.

          Key control:
            SW1:  Starts and stops the transmitting
            SW2:  initiates end device binding
            SW3:
            SW4:  initiates a match description request

  Notes:

    This application was intended to be used to test the maximum
    throughput between 2 devices in a network - between routers
    coordinators.

    Although not recommended, it can be used between
    an end device and a router (or coordinator), but you must
    enable the delay feature (TRANSMITAPP_DELAY_SEND and
    TRANSMITAPP_SEND_DELAY).  If you don't include the delay, the
    end device can't receive messages because it will stop polling.
    Also, the delay must be greater than RESPONSE_POLL_RATE (default 100 MSec).

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
#include "NLMEDE.h"
#include "aps_groups.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDProfile.h"

#include "TransmitApp.h"
#include "nwk_util.h"
#include "OnBoard.h"

#include "DebugTrace.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "SPIMgr.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define TRANSMITAPP_STATE_WAITING 0
#define TRANSMITAPP_STATE_SENDING 1

#if !defined ( RTR_NWK )
  // Use these 2 lines to add a delay between each packet sent
  //  - default for end devices
  #define TRANSMITAPP_DELAY_SEND
  #define TRANSMITAPP_SEND_DELAY    (RESPONSE_POLL_RATE * 2)  // in MSecs
#endif

// Send with or without APS ACKs
//#define TRANSMITAPP_TX_OPTIONS    (AF_DISCV_ROUTE | AF_ACK_REQUEST)
#define TRANSMITAPP_TX_OPTIONS    AF_DISCV_ROUTE

#define TRANSMITAPP_INITIAL_MSG_COUNT  2

#define TRANSMITAPP_TRANSMIT_TIME   4  // 4 MS
#define TRANSMITAPP_DISPLAY_TIMER   (2 * 1000)

#if defined ( TRANSMITAPP_FRAGMENTED )
#define TRANSMITAPP_MAX_DATA_LEN    225
#else
#define TRANSMITAPP_MAX_DATA_LEN    102
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// This is the buffer that is sent out as data.
byte TransmitApp_Msg[ TRANSMITAPP_MAX_DATA_LEN ];

// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t TransmitApp_ClusterList[TRANSMITAPP_MAX_CLUSTERS] =
{
  TRANSMITAPP_CLUSTERID_TESTMSG  // MSG Cluster ID
};

const SimpleDescriptionFormat_t TransmitApp_SimpleDesc =
{
  TRANSMITAPP_ENDPOINT,              //  int    Endpoint;
  TRANSMITAPP_PROFID,                //  uint16 AppProfId[2];
  TRANSMITAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  TRANSMITAPP_DEVICE_VERSION,        //  int    AppDevVer:4;
  TRANSMITAPP_FLAGS,                 //  int    AppFlags:4;
  TRANSMITAPP_MAX_CLUSTERS,          //  byte   AppNumInClusters;
  (cId_t *)TransmitApp_ClusterList,  //  byte   *pAppInClusterList;
  TRANSMITAPP_MAX_CLUSTERS,          //  byte   AppNumInClusters;
  (cId_t *)TransmitApp_ClusterList   //  byte   *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in TransmitApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t TransmitApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// Task ID for event processing - received when TransmitApp_Init() is called.
byte TransmitApp_TaskID;

devStates_t TransmitApp_NwkState;

static byte TransmitApp_TransID;  // This is the unique message ID (counter)

afAddrType_t TransmitApp_DstAddr;

byte TransmitApp_State;

// Shadow of the OSAL system clock used for calculating actual time expired.
static uint32 clkShdw;
// Running total count of test messages recv/sent since beginning current run.
static uint32 rxTotal, txTotal;
// Running count of test messages recv/sent since last display / update - 1 Hz.
static uint32 rxAccum, txAccum;

static byte timerOn;

static byte timesToSend;

uint16 pktCounter;

// Max Data Request Length
uint16 TransmitApp_MaxDataLength;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void TransmitApp_HandleKeys( byte shift, byte keys );
void TransmitApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void TransmitApp_SendTheMessage( void );
void TransmitApp_ChangeState( void );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
void TransmitApp_DisplayResults( void );

/*********************************************************************
 * @fn      TransmitApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void TransmitApp_Init( byte task_id )
{
#if !defined ( TRANSMITAPP_FRAGMENTED )
  afDataReqMTU_t mtu;
#endif
  uint16 i;

  TransmitApp_TaskID = task_id;
  TransmitApp_NwkState = DEV_INIT;
  TransmitApp_TransID = 0;

  pktCounter = 0;

  TransmitApp_State = TRANSMITAPP_STATE_WAITING;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  TransmitApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  TransmitApp_DstAddr.endPoint = 0;
  TransmitApp_DstAddr.addr.shortAddr = 0;

  // Fill out the endpoint description.
  TransmitApp_epDesc.endPoint = TRANSMITAPP_ENDPOINT;
  TransmitApp_epDesc.task_id = &TransmitApp_TaskID;
  TransmitApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&TransmitApp_SimpleDesc;
  TransmitApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint/interface description with the AF
  afRegister( &TransmitApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( TransmitApp_TaskID );

  // Update the display
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "TransmitApp", HAL_LCD_LINE_2 );
#endif

  // Set the data length
#if defined ( TRANSMITAPP_FRAGMENTED )
  TransmitApp_MaxDataLength = TRANSMITAPP_MAX_DATA_LEN;
#else
  mtu.kvp        = FALSE;
  mtu.aps.secure = FALSE;
  TransmitApp_MaxDataLength = afDataReqMTU( &mtu );
#endif

  // Generate the data
  for (i=0; i<TransmitApp_MaxDataLength; i++)
  {
    TransmitApp_Msg[i] = (uint8) i;
  }
}

/*********************************************************************
 * @fn      TransmitApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
UINT16 TransmitApp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  byte dstEP;
  zAddrType_t *dstAddr;
  afDataConfirm_t *afDataConfirm;
  ZDO_NewDstAddr_t *ZDO_NewDstAddr;

  // Data Confirmation message fields
  ZStatus_t sentStatus;
  byte sentEP;

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( TransmitApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case KEY_CHANGE:
          TransmitApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case AF_DATA_CONFIRM_CMD:
          // This message is received as a confirmation of a data packet sent.
          // The status is of ZStatus_t type [defined in ZComDef.h]
          // The message fields are defined in AF.h
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;

          if ( (ZSuccess == sentStatus) &&
               (TransmitApp_epDesc.endPoint == sentEP) )
          {
            txAccum += TransmitApp_MaxDataLength;
            if ( !timerOn )
            {
              osal_start_timerEx( TransmitApp_TaskID,TRANSMITAPP_RCVTIMER_EVT,
                                                     TRANSMITAPP_DISPLAY_TIMER);
              clkShdw = osal_GetSystemClock();
              timerOn = TRUE;
            }
          }

          // Action taken when confirmation is received: Send the next message.
          TransmitApp_SetSendEvt();
          break;

        case AF_INCOMING_MSG_CMD:
          TransmitApp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_NEW_DSTADDR:
          ZDO_NewDstAddr = (ZDO_NewDstAddr_t *)MSGpkt;

          dstEP = ZDO_NewDstAddr->dstAddrDstEP;
          dstAddr = &ZDO_NewDstAddr->dstAddr;
          TransmitApp_DstAddr.addrMode = (afAddrMode_t)dstAddr->addrMode;
          TransmitApp_DstAddr.endPoint = dstEP;
          if ( dstAddr->addrMode == Addr16Bit )
          {
            TransmitApp_DstAddr.addr.shortAddr = dstAddr->addr.shortAddr;
          }
          break;

        case ZDO_STATE_CHANGE:
          TransmitApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( TransmitApp_TaskID );
    }

    // Squash compiler warnings until values are used.
    (void)sentStatus;
    (void)sentEP;

    // Return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out
  if ( events & TRANSMITAPP_SEND_MSG_EVT )
  {
    if ( TransmitApp_State == TRANSMITAPP_STATE_SENDING )
    {
      TransmitApp_SendTheMessage();
    }

    // Return unprocessed events
    return (events ^ TRANSMITAPP_SEND_MSG_EVT);
  }

  // Timed wait from error
  if ( events & TRANSMITAPP_SEND_ERR_EVT )
  {
    TransmitApp_SetSendEvt();

    // Return unprocessed events
    return (events ^ TRANSMITAPP_SEND_ERR_EVT);
  }

  // Receive timer
  if ( events & TRANSMITAPP_RCVTIMER_EVT )
  {
    // Setup to display the next result
    osal_start_timerEx( TransmitApp_TaskID, TRANSMITAPP_RCVTIMER_EVT,
                                            TRANSMITAPP_DISPLAY_TIMER );
    TransmitApp_DisplayResults();

    return (events ^ TRANSMITAPP_RCVTIMER_EVT);
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * @fn      TransmitApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 EVAL_SW4
 *                 EVAL_SW3
 *                 EVAL_SW2
 *                 EVAL_SW1
 *
 * @return  none
 */
void TransmitApp_HandleKeys( byte shift, byte keys )
{
  aps_Group_t group;
  // Shift is used to make each button/switch dual purpose.
  if ( shift )
  {
    if ( keys & HAL_KEY_SW_1 )
    {
      // Assign yourself to group 1
      group.ID = 0x0001;
      group.name[0] = 0;
      aps_AddGroup( TRANSMITAPP_ENDPOINT, &group );
    }
    if ( keys & HAL_KEY_SW_2 )
    {
      // Change destination address to group 1
      TransmitApp_DstAddr.addrMode = afAddrGroup;
      TransmitApp_DstAddr.endPoint = 0;
      TransmitApp_DstAddr.addr.shortAddr = 0x001; // group address
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
      TransmitApp_ChangeState();
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      // Initiate an End Device Bind Request for the mandatory endpoint
      ZDApp_SendEndDeviceBindReq( TransmitApp_epDesc.endPoint );
    }

    if ( keys & HAL_KEY_SW_3 )
    {
      rxTotal = txTotal = 0;
      rxAccum = txAccum = 0;
      TransmitApp_DisplayResults();
    }

    if ( keys & HAL_KEY_SW_4 )
    {
      // Initiate a Match Description Request (Service Discovery)
      //  for the mandatory endpoint
      ZDApp_AutoFindDestination( TransmitApp_epDesc.endPoint );
    }
  }
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      TransmitApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void TransmitApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  int16 i;
  uint8 error = FALSE;

  switch ( pkt->clusterId )
  {
    case TRANSMITAPP_CLUSTERID_TESTMSG:
      if (pkt->cmd.DataLength != TransmitApp_MaxDataLength)
      {
        error = TRUE;
      }

      for (i=4; i<pkt->cmd.DataLength; i++)
      {
        if (pkt->cmd.Data[i] != i%256)
          error = TRUE;
      }

      if (error)
      {
        // Display error LED
        HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
      }
      else
      {
        if ( !timerOn )
        {
          osal_start_timerEx( TransmitApp_TaskID, TRANSMITAPP_RCVTIMER_EVT,
                                                  TRANSMITAPP_DISPLAY_TIMER );
          clkShdw = osal_GetSystemClock();
          timerOn = TRUE;
        }
        rxAccum += pkt->cmd.DataLength;
      }
      break;

    // Could receive control messages in the future.
    default:
      break;
  }
}

/*********************************************************************
 * @fn      TransmitApp_SendTheMessage
 *
 * @brief   Send "the" message.
 *
 * @param   none
 *
 * @return  none
 */
void TransmitApp_SendTheMessage( void )
{
  uint16 len;
  uint8 tmp;

  // put the sequence number in the message
  tmp = HI_UINT8( TransmitApp_TransID );
  tmp += (tmp <= 9) ? ('0') : ('A' - 0x0A);
  TransmitApp_Msg[2] = tmp;
  tmp = LO_UINT8( TransmitApp_TransID );
  tmp += (tmp <= 9) ? ('0') : ('A' - 0x0A);
  TransmitApp_Msg[3] = tmp;

  len = TransmitApp_MaxDataLength;

#if defined ( TRANSMITAPP_RANDOM_LEN )
  len = (uint8)(osal_rand() & 0x7F);
  if( len > TransmitApp_MaxDataLength || len == 0 )
    len = TransmitApp_MaxDataLength;
#endif
	
  do {
    tmp = AF_DataRequest( &TransmitApp_DstAddr, &TransmitApp_epDesc,
                           TRANSMITAPP_CLUSTERID_TESTMSG,
                           len, TransmitApp_Msg,
                          &TransmitApp_TransID,
                           TRANSMITAPP_TX_OPTIONS,
                           AF_DEFAULT_RADIUS );

    if ( timesToSend )
    {
      timesToSend--;
    }
  } while ( (timesToSend != 0) && (afStatus_SUCCESS == tmp) );

  if ( afStatus_SUCCESS == tmp )
  {
    pktCounter++;
  }
  else
  {
    // Error, so wait (10 mSec) and try again.
    osal_start_timerEx( TransmitApp_TaskID, TRANSMITAPP_SEND_ERR_EVT, 10 );
  }
}

/*********************************************************************
 * @fn      TransmitApp_ChangeState
 *
 * @brief   Toggle the Sending/Waiting state flag
 *
 * @param   none
 *
 * @return  none
 */
void TransmitApp_ChangeState( void )
{
  if ( TransmitApp_State == TRANSMITAPP_STATE_WAITING )
  {
    TransmitApp_State = TRANSMITAPP_STATE_SENDING;
    TransmitApp_SetSendEvt();
    timesToSend = TRANSMITAPP_INITIAL_MSG_COUNT;
  }
  else
  {
    TransmitApp_State = TRANSMITAPP_STATE_WAITING;
  }
}

/*********************************************************************
 * @fn      TransmitApp_SetSendEvt
 *
 * @brief   Set the event flag
 *
 * @param   none
 *
 * @return  none
 */
void TransmitApp_SetSendEvt( void )
{
#if defined( TRANSMITAPP_DELAY_SEND )
  // Adds a delay to sending the data
  osal_start_timerEx( TransmitApp_TaskID,
                    TRANSMITAPP_SEND_MSG_EVT, TRANSMITAPP_SEND_DELAY );
#else
  // No Delay - just send the data
  osal_set_event( TransmitApp_TaskID, TRANSMITAPP_SEND_MSG_EVT );
#endif
}

/*********************************************************************
 * @fn      TransmitApp_DisplayResults
 *
 * @brief   Display the results and clear the accumulators
 *
 * @param   none
 *
 * @return  none
 */
void TransmitApp_DisplayResults( void )
{
#ifdef LCD_SUPPORTED
  #define LCD_W  16
  uint32 rxShdw, txShdw, tmp;
  byte lcd_buf[LCD_W+1];
  byte idx;
#endif
  // The OSAL timers are not real-time, so calculate the actual time expired.
  uint32 msecs = osal_GetSystemClock() - clkShdw;
  clkShdw = osal_GetSystemClock();

  rxTotal += rxAccum;
  txTotal += txAccum;

#if defined ( LCD_SUPPORTED )
  rxShdw = (rxAccum * 1000 + msecs/2) / msecs;
  txShdw = (txAccum * 1000 + msecs/2) / msecs;

  osal_memset( lcd_buf, ' ', LCD_W );
  lcd_buf[LCD_W] = NULL;

  idx = 4;
  tmp = (rxShdw >= 100000) ? 99999 : rxShdw;
  do
  {
    lcd_buf[idx--] = (uint8) ('0' + (tmp % 10));
    tmp /= 10;
  } while ( tmp );

  idx = LCD_W-1;
  tmp = rxTotal;
  do
  {
    lcd_buf[idx--] = (uint8) ('0' + (tmp % 10));
    tmp /= 10;
  } while ( tmp );

  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_1 );
  osal_memset( lcd_buf, ' ', LCD_W );

  idx = 4;
  tmp = (txShdw >= 100000) ? 99999 : txShdw;
  do
  {
    lcd_buf[idx--] = (uint8) ('0' + (tmp % 10));
    tmp /= 10;
  } while ( tmp );

  idx = LCD_W-1;
  tmp = txTotal;
  do
  {
    lcd_buf[idx--] = (uint8) ('0' + (tmp % 10));
    tmp /= 10;
  } while ( tmp );

  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_2 );
#else
  DEBUG_INFO( COMPID_APP, SEVERITY_INFORMATION, 3, rxAccum,
                                              (uint16)msecs, (uint16)rxTotal );
#endif

  if ( (rxAccum == 0) && (txAccum == 0) )
  {
    osal_stop_timerEx( TransmitApp_TaskID, TRANSMITAPP_RCVTIMER_EVT );
    timerOn = FALSE;
  }

  rxAccum = txAccum = 0;
}

/*********************************************************************
*********************************************************************/
