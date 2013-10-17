/*********************************************************************
    Filename:       ZDApp.c
    Revised:        $Date: 2007-05-31 15:56:04 -0700 (Thu, 31 May 2007) $
    Revision:       $Revision: 14490 $

    Description:

      This file contains the interface to the Zigbee Device Application.
      This is the Application part that the use can change. This also
      contains the Task functions.

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
#include "ZMac.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"
#include "OSAL_Nv.h"
#include "AF.h"
#include "APSMEDE.h"
#include "NLMEDE.h"
#include "AddrMgr.h"
#include "ZDCache.h"
#include "ZDProfile.h"
#include "ZDObject.h"
#include "ZDConfig.h"
#include "ZDSecMgr.h"
#include "ZDApp.h"
#include "DebugTrace.h"
#include "nwk_util.h"
#include "OnBoard.h"
#include "ZGlobals.h"

#if   ( SECURE != 0  )
  #include "ssp.h"
#endif

#if defined( MT_ZDO_FUNC )
  #include "MT_ZDO.h"
#endif

/* HAL */
#include "hal_led.h"
#include "hal_lcd.h"
#include "hal_key.h"

#if defined( MT_MAC_FUNC ) || defined( MT_MAC_CB_FUNC )
  #error "ERROR! MT_MAC functionalities should be disabled on ZDO devices"
#endif
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#if !defined( NWK_START_DELAY )
  #define NWK_START_DELAY             100   // in milliseconds
#endif

#if !defined( EXTENDED_JOINING_RANDOM_MASK )
  #define EXTENDED_JOINING_RANDOM_MASK 0x007F
#endif

#if !defined( BEACON_REQUEST_DELAY )
  #define BEACON_REQUEST_DELAY        100   // in milliseconds
#endif

#if !defined( BEACON_REQ_DELAY_MASK )
  #define BEACON_REQ_DELAY_MASK       0x007F
#endif

#if defined (AUTO_SOFT_START)
  #define MAX_RESUME_RETRY            3
  #define NUM_DISC_ATTEMPTS           3
#else
  #define MAX_RESUME_RETRY            1
#endif

#define MAX_DEVICE_UNAUTH_TIMEOUT   5000  // 5 seconds

// Beacon Order Settings (see NLMEDE.h)
#define DEFAULT_BEACON_ORDER        BEACON_ORDER_NO_BEACONS
#define DEFAULT_SUPERFRAME_ORDER    DEFAULT_BEACON_ORDER

#if ( SECURE != 0 )
  #if !defined( MAX_NWK_FRAMECOUNTER_CHANGES )
    // The number of times the frame counter can change before
    // saving to NV
    #define MAX_NWK_FRAMECOUNTER_CHANGES    1000
  #endif
#endif

// Leave control bits
#define ZDAPP_LEAVE_CTRL_INIT 0
#define ZDAPP_LEAVE_CTRL_SET  1
#define ZDAPP_LEAVE_CTRL_RA   2

// Standard time to update NWK NV data
#define ZDAPP_UPDATE_NWK_NV_TIME 100

// Address Manager Stub Implementation
#define ZDApp_NwkWriteNVRequest AddrMgrWriteNVRequest

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

byte zdoDiscCounter = 1;

zAddrType_t ZDAppNwkAddr;

#if defined ( ZDO_MGMT_NWKDISC_RESPONSE )
  byte zdappMgmtNwkDiscRspTransSeq;
  byte zdappMgmtNwkDiscReqInProgress = FALSE;
  zAddrType_t zdappMgmtNwkDiscRspAddr;
  byte zdappMgmtNwkDiscStartIndex;
  byte zdappMgmtSavedNwkState;
#endif

#if ( SECURE != 0 )
  uint16 nwkFrameCounterChanges = 0;
#endif

#if defined ( SOFT_START )
  static uint8 softStartAllowCoord = TRUE;
#endif

uint8 continueJoining = TRUE;

byte  _tmpRejoinState;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

void ZDApp_NetworkStartEvt( void );
void ZDApp_DeviceAuthEvt( void );
void ZDApp_SaveNetworkStateEvt( void );

uint8 ZDApp_ReadNetworkRestoreState( void );
uint8 ZDApp_RestoreNetworkState( void );
void ZDAppDetermineDeviceType( void );
void ZDAppSetupProtoVersion( void );
void ZDApp_InitUserDesc( void );
void ZDAppCheckForHoldKey( void );
void ZDApp_ProcessOSALMsg( osal_event_hdr_t *msgPtr );
void ZDApp_ProcessNetworkJoin( void );
void ZDApp_SetCoordAddress( byte endPoint, byte dstEP );
void ZDApp_SendNewDstAddr( byte dstEP, zAddrType_t *dstAddr,
               cId_t clusterID, byte removeFlag, byte task_id, byte endpoint );

#if ( SECURE != 0 )
  void ZDApp_SaveNwkKey( void );
  byte ZDApp_RestoreNwkKey( void );
#endif

void ZDApp_SendMsg( byte taskID, byte cmd, byte len, byte *buf );

#if defined ( ZDO_BIND_UNBIND_RESPONSE ) && !defined ( REFLECTOR )
  extern void ZDApp_AppBindReq( byte TransSeq, zAddrType_t *SrcAddr, byte *SrcAddress,
                      byte SrcEndPoint, cId_t ClusterID, byte *DstAddress,
                      byte DstEndPoint, byte SecurityUse, uint8 Type );
#endif

void ZDApp_ResetTimerStart( uint16 delay );
void ZDApp_ResetTimerCancel( void );
void ZDApp_LeaveCtrlInit( void );
void ZDApp_LeaveCtrlSet( uint8 ra );
uint8 ZDApp_LeaveCtrlBypass( void );
void ZDApp_LeaveCtrlStartup( devStates_t* state, uint16* startDelay );
void ZDApp_LeaveReset( uint8 ra );
void ZDApp_LeaveUpdate( uint16 nwkAddr, uint8* extAddr,
                        uint8 removeChildren );
void ZDApp_NodeProfileSync( ZDO_NetworkDiscoveryCfm_t* cfm );

/*********************************************************************
 * LOCAL VARIABLES
 */

byte ZDAppTaskID;
byte nwkStatus;
endPointDesc_t *ZDApp_AutoFindMode_epDesc = (endPointDesc_t *)NULL;
uint8 ZDApp_LeaveCtrl;

#if defined( HOLD_AUTO_START )
  devStates_t devState = DEV_HOLD;
#else
  devStates_t devState = DEV_INIT;
#endif

#if defined( ZDO_COORDINATOR ) && !defined( SOFT_START )
  // Set the default to coodinator
  devStartModes_t devStartMode = MODE_HARD;
#else
  devStartModes_t devStartMode = MODE_JOIN;     // Assume joining
  //devStartModes_t devStartMode = MODE_RESUME; // if already "directly joined"
                        // to parent. Set to make the device do an Orphan scan.
#endif

#if defined ( ZDO_IEEEADDR_REQUEST )
  static byte ZDApp_IEEEAddrRsp_TaskID = 0;  // Initialized to NO TASK
#endif

#if defined ( ZDO_NWKADDR_REQUEST )
  static byte ZDApp_NwkAddrRsp_TaskID = 0;  // Initialized to NO TASK
#endif

static byte ZDApp_MatchDescRsp_TaskID = 0;  // Initialized to NO TASK
static byte ZDApp_EndDeviceAnnounce_TaskID = 0;  // Initialized to NO TASK

#if defined ( ZDO_BIND_UNBIND_REQUEST )
  static byte ZDApp_BindUnbindRsp_TaskID = TASK_NO_TASK;
#endif

#if defined ( ZDO_BIND_UNBIND_RESPONSE ) && !defined ( REFLECTOR )
  static byte ZDApp_BindReq_TaskID = 0;  // Initialized to NO TASK
#endif

#if defined ( ZDO_MGMT_BIND_RESPONSE ) && !defined ( REFLECTOR )
  static byte ZDApp_MgmtBindReq_TaskID = 0;  // Initialized to NO TASK
#endif

#if !defined( ZDO_COORDINATOR ) || defined( SOFT_START )
  static uint8 retryCnt;
#endif

// a little awkward -- this is will hold the list of versions that are legal given other
// constraints such as NV value, macro values etc. list used in ZDO_NetworkDiscoveryConfirmCB()
// when a joining device is deciding which network to join.
static byte sPVerList[] = {ZB_PROT_V1_1, ZB_PROT_V1_0};

endPointDesc_t ZDApp_epDesc =
{
  ZDO_EP,
  &ZDAppTaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for ZDO
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

/*********************************************************************
 * @fn      ZDApp_Init
 *
 * @brief   ZDApp Initialization function.
 *
 * @param   task_id - ZDApp Task ID
 *
 * @return  None
 */
void ZDApp_Init( byte task_id )
{
  uint8 capabilities;

  // Save the task ID
  ZDAppTaskID = task_id;

  // Initialize the ZDO global device short address storage
  ZDAppNwkAddr.addrMode = Addr16Bit;
  ZDAppNwkAddr.addr.shortAddr = INVALID_NODE_ADDR;
  (void)NLME_GetExtAddr();  // Load the saveExtAddr pointer.

  // Check for manual "Hold Auto Start"
  ZDAppCheckForHoldKey();

  // Initialize ZDO items and setup the device - type of device to create.
  ZDO_Init();

  // Register the endpoint description with the AF
  // This task doesn't have a Simple description, but we still need
  // to register the endpoint.
  afRegister( (endPointDesc_t *)&ZDApp_epDesc );

#if defined( ZDO_USERDESC_RESPONSE )
  ZDApp_InitUserDesc();
#endif // ZDO_USERDESC_RESPONSE

#if defined( ZDO_CACHE )
  ZDCacheInit();
#endif

  // Setup the Zigbee Network Protocol Version
  ZDAppSetupProtoVersion();

  // set broadcast address mask to support broadcast filtering
  NLME_GetRequest(nwkCapabilityInfo, 0, &capabilities);
  NLME_SetBroadcastFilter( capabilities );

  // Start the device?
  if ( devState != DEV_HOLD )
  {
    ZDOInitDevice( 0 );
  }
  else
  {
    // Blink LED to indicate HOLD_START
    HalLedBlink ( HAL_LED_4, 0, 50, 500 );
  }
} /* ZDO_Init() */

/*********************************************************************
 * @fn      ZDApp_event_loop()
 *
 * @brief   Main event loop for Zigbee device objects task. This function
 *          should be called at periodic intervals.
 *
 * @param   task_id - Task ID
 * @param   events  - Bitmap of events
 *
 * @return  none
 */
UINT16 ZDApp_event_loop( byte task_id, UINT16 events )
{
  uint8 *msg_ptr;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (msg_ptr = osal_msg_receive( ZDAppTaskID )) )
    {
      ZDApp_ProcessOSALMsg( (osal_event_hdr_t *)msg_ptr );

      // Release the memory
      osal_msg_deallocate( msg_ptr );
    }

    // Return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & ZDO_NETWORK_INIT )
  {
    // Initialize apps and start the network
    devState = DEV_INIT;
    ZDO_StartDevice( (uint8)ZDO_Config_Node_Descriptor.LogicalType, devStartMode,
                     DEFAULT_BEACON_ORDER, DEFAULT_SUPERFRAME_ORDER );

    // Return unprocessed events
    return (events ^ ZDO_NETWORK_INIT);
  }

#if defined (RTR_NWK)
  if ( events & ZDO_NETWORK_START )
  {
    ZDApp_NetworkStartEvt();

    // Return unprocessed events
    return (events ^ ZDO_NETWORK_START);
  }
#endif  //RTR_NWK

#if defined ( RTR_NWK )
  if ( events & ZDO_ROUTER_START )
  {
    if ( nwkStatus == ZSuccess )
    {
      if ( devState == DEV_END_DEVICE )
        devState = DEV_ROUTER;

      osal_pwrmgr_device( PWRMGR_ALWAYS_ON );
    }
    else
    {
      // remain as end device!!
    }
    osal_set_event( ZDAppTaskID, ZDO_STATE_CHANGE_EVT );

    // Return unprocessed events
    return (events ^ ZDO_ROUTER_START);
  }
#endif  // RTR

  if ( events & ZDO_STATE_CHANGE_EVT )
  {
    ZDO_UpdateNwkStatus( devState );

    // Return unprocessed events
    return (events ^ ZDO_STATE_CHANGE_EVT);
  }

  if ( events & ZDO_COMMAND_CNF )
  {
    // User defined logic

    // Return unprocessed events
    return (events ^ ZDO_COMMAND_CNF);
  }

#if defined( ZDSECMGR_SECURE ) && defined( RTR_NWK )
  if ( events & ZDO_NEW_DEVICE )
  {
    // process the new device event
    if ( ZDSecMgrNewDeviceEvent() == TRUE )
    {
      osal_start_timerEx( ZDAppTaskID, ZDO_NEW_DEVICE, 1000 );
    }

    // Return unprocessed events
    return (events ^ ZDO_NEW_DEVICE);
  }
#endif  // ZDSECMGR_SECURE && RTR

#if defined ( ZDSECMGR_COMMERCIAL )
  if ( events & ZDO_SECMGR_EVENT )
  {
    ZDSecMgrEvent();

    // Return unprocessed events
    return (events ^ ZDO_SECMGR_EVENT);
  }
#endif // defined( ZDSECMGR_COMMERCIAL )

#if   ( SECURE != 0  )
  if ( events & ZDO_DEVICE_AUTH )
  {
    ZDApp_DeviceAuthEvt();

    // Return unprocessed events
    return (events ^ ZDO_DEVICE_AUTH);
  }
#endif  // SECURE

  if ( events & ZDO_NWK_UPDATE_NV )
  {
    ZDApp_SaveNetworkStateEvt();

    // Return unprocessed events
    return (events ^ ZDO_NWK_UPDATE_NV);
  }

#if ( SECURE != 0  )
  if ( events & ZDO_FRAMECOUNTER_CHANGE )
  {
    if ( nwkFrameCounterChanges++ > MAX_NWK_FRAMECOUNTER_CHANGES )
      ZDApp_SaveNwkKey();

    // Return unprocessed events
    return (events ^ ZDO_FRAMECOUNTER_CHANGE);
  }
#endif

  if ( events & ZDO_DEVICE_RESET )
  {
    // The device has been in the UNAUTH state, so reset
    // Note: there will be no return from this call
    SystemReset();
  }

  // Discard or make more handlers
  return 0;
}

/*********************************************************************
 * Application Functions
 */

/*********************************************************************
 * @fn      ZDOInitDevice
 *
 * @brief   Start the device in the network.  This function will read
 *   ZCD_NV_STARTUP_OPTION (NV item) to determine whether or not to
 *   restore the network state of the device.
 *
 * @param   startDelay - timeDelay to start device (in milliseconds).
 *      There is a jitter added to this delay:
 *              ((NWK_START_DELAY + startDelay)
 *              + (osal_rand() & EXTENDED_JOINING_RANDOM_MASK))
 *
 * NOTE:    If the application would like to force a "new" join, the
 *          application should set the ZCD_STARTOPT_DEFAULT_NETWORK_STATE
 *          bit in the ZCD_NV_STARTUP_OPTION NV item before calling
 *          this function. "new" join means to not restore the network
 *          state of the device. Use zgWriteStartupOptions() to set these
 *          options.
 *
 * @return
 *    ZDO_INITDEV_RESTORED_NETWORK_STATE  - The device's network state was
 *          restored.
 *    ZDO_INITDEV_NEW_NETWORK_STATE - The network state was initialized.
 *          This could mean that ZCD_NV_STARTUP_OPTION said to not restore, or
 *          it could mean that there was no network state to restore.
 *    ZDO_INITDEV_LEAVE_NOT_STARTED - Before the reset, a network leave was issued
 *          with the rejoin option set to TRUE.  So, the device was not
 *          started in the network (one time only).  The next time this
 *          function is called it will start.
 */
uint8 ZDOInitDevice( uint16 startDelay )
{
  uint8 networkStateNV = ZDO_INITDEV_NEW_NETWORK_STATE;
  uint16 extendedDelay = 0;

  devState = DEV_INIT;    // Remove the Hold state

  // Initialize leave control logic
  ZDApp_LeaveCtrlInit();

  // Check leave control reset settings
  ZDApp_LeaveCtrlStartup( &devState, &startDelay );

  // Leave may make the hold state come back
  if ( devState == DEV_HOLD )
    return ( ZDO_INITDEV_LEAVE_NOT_STARTED );   // Don't join - (one time).

#if defined ( NV_RESTORE )
  // Get Keypad directly to see if a reset nv is needed.
  // Hold down the SW_BYPASS_NV key (defined in OnBoard.h)
  // while booting to skip past NV Restore.
  if ( HalKeyRead() == SW_BYPASS_NV )
    networkStateNV = ZDO_INITDEV_NEW_NETWORK_STATE;
  else
  {
    // Determine if NV should be restored
    networkStateNV = ZDApp_ReadNetworkRestoreState();
  }

  if ( networkStateNV == ZDO_INITDEV_RESTORED_NETWORK_STATE )
  {
    networkStateNV = ZDApp_RestoreNetworkState();
  }
  else
  {
    // Wipe out the network state in NV
    NLME_InitNV();
    NLME_SetDefaultNV();
    ZDAppSetupProtoVersion();
  }
#endif

  if ( networkStateNV == ZDO_INITDEV_NEW_NETWORK_STATE )
  {
    ZDAppDetermineDeviceType();

    // Only delay if joining network - not restoring network state
    extendedDelay = (uint16)((NWK_START_DELAY + startDelay)
              + (osal_rand() & EXTENDED_JOINING_RANDOM_MASK));
  }

  // Trigger the network start
  ZDApp_NetworkInit( extendedDelay );

  return ( networkStateNV );
}

/*********************************************************************
 * @fn      ZDApp_ReadNetworkRestoreState
 *
 * @brief   Read the ZCD_NV_STARTUP_OPTION NV Item to state whether
 *          or not to restore the network state.
 *          If the read value has the ZCD_STARTOPT_DEFAULT_NETWORK_STATE
 *          bit set return the ZDO_INITDEV_NEW_NETWORK_STATE.
 *
 * @param   none
 *
 * @return  ZDO_INITDEV_NEW_NETWORK_STATE
 *          or ZDO_INITDEV_RESTORED_NETWORK_STATE based on whether or
 *          not ZCD_STARTOPT_DEFAULT_NETWORK_STATE bit is set in
 *          ZCD_NV_STARTUP_OPTION
 */
uint8 ZDApp_ReadNetworkRestoreState( void )
{
  uint8 networkStateNV = ZDO_INITDEV_RESTORED_NETWORK_STATE;

  // Look for the New Network State option.
  if ( zgReadStartupOptions() & ZCD_STARTOPT_DEFAULT_NETWORK_STATE )
  {
    networkStateNV = ZDO_INITDEV_NEW_NETWORK_STATE;
  }

  return ( networkStateNV );
}

/*********************************************************************
 * @fn      ZDAppDetermineDeviceType()
 *
 * @brief   Determines the type of device to start.  Right now
 *          this only works with the SOFT_START feature.  So it doesn't
 *          support the end device type.
 *
 *          Looks at zgDeviceLogicalType and determines what type of
 *          device to start.  The types are:
 *            ZG_DEVICETYPE_COORDINATOR
 *            ZG_DEVICETYPE_ROUTER
 *            ZG_DEVICETYPE_ENDDEVICE - not supported yet.
 *            ZG_DEVICETYPE_SOFT - looks for coordinator, if one doesn't
 *               exist, becomes one.  This option is should only be used
 *               if the system is manually configured and you are insured
 *               that the first device is started before all the other
 *               devices are started.
 *
 * @param   none
 *
 * @return  none
 */
void ZDAppDetermineDeviceType( void )
{
  if ( zgDeviceLogicalType == ZG_DEVICETYPE_ENDDEVICE )
    return;

#if defined ( SOFT_START )
  if ( zgDeviceLogicalType == ZG_DEVICETYPE_COORDINATOR )
  {
    devStartMode = MODE_HARD;     // Start as a coordinator
    ZDO_Config_Node_Descriptor.LogicalType = NODETYPE_COORDINATOR;
  }
  else
  {
    if ( zgDeviceLogicalType == ZG_DEVICETYPE_ROUTER )
    {
      softStartAllowCoord = FALSE;  // Don't allow coord to start
      continueJoining = TRUE;
    }
    devStartMode = MODE_JOIN;     // Assume joining
  }
#endif // SOFT_START
}

/*********************************************************************
 * @fn      ZDApp_NetworkStartEvt()
 *
 * @brief   Process the Network Start Event
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_NetworkStartEvt( void )
{
  if ( nwkStatus == ZSuccess )
  {
    // Successfully started a ZigBee network
    if ( devState == DEV_COORD_STARTING )
    {
      devState = DEV_ZB_COORD;

#if ( SECURE != 0 )
      // Initialize keys
      SSP_UpdateNwkKey( (byte*)zgPreConfigKey, 0 );
      SSP_SwitchNwkKey( 0 );
#endif
    }

    osal_pwrmgr_device( PWRMGR_ALWAYS_ON );
    osal_set_event( ZDAppTaskID, ZDO_STATE_CHANGE_EVT );
  }
  else
  {
    // Try again with a higher energy threshold !!
    if ( ( NLME_GetEnergyThreshold() + ENERGY_SCAN_INCREMENT ) < 0xff )
    {
      NLME_SetEnergyThreshold( (uint8)(NLME_GetEnergyThreshold() + ENERGY_SCAN_INCREMENT) );
      osal_set_event( ZDAppTaskID, ZDO_NETWORK_INIT );
    }
    else
    {
      // Failed to start network. Enter a dormant state (until user intervenes)
      devState = DEV_INIT;
      osal_set_event( ZDAppTaskID, ZDO_STATE_CHANGE_EVT );
    }
  }
}

#if ( SECURE != 0 )
/*********************************************************************
 * @fn      ZDApp_DeviceAuthEvt()
 *
 * @brief   Process the Device Authentic Event
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_DeviceAuthEvt( void )
{
  // received authentication from trust center
  if ( devState == DEV_END_DEVICE_UNAUTH )
  {
    // Stop the reset timer so it doesn't reset
    ZDApp_ResetTimerCancel();

    devState = DEV_END_DEVICE;
    osal_set_event( ZDAppTaskID, ZDO_STATE_CHANGE_EVT );

    // Set the Power Manager Device
#if defined ( POWER_SAVING )
    osal_pwrmgr_device( PWRMGR_BATTERY );
#endif

#if defined ( RTR_NWK )
    if ( ZDO_Config_Node_Descriptor.LogicalType != NODETYPE_DEVICE )
    {
      // NOTE: first two parameters are not used, see NLMEDE.h for details
      NLME_StartRouterRequest( 0, 0, false );
    }
#endif  // RTR

      // Notify to save info into NV
    osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 100 );

    // Save off the security
    ZDApp_SaveNwkKey();

#if defined ( ZDO_ENDDEVICE_ANNCE_GENERATE )
    ZDP_EndDeviceAnnce( ZDAppNwkAddr.addr.shortAddr, saveExtAddr,
                       ZDO_Config_Node_Descriptor.CapabilityFlags, 0 );
#endif
  }
  else
  {
    osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );
  }
}
#endif

/*********************************************************************
 * @fn      ZDApp_SaveNetworkStateEvt()
 *
 * @brief   Process the Save the Network State Event
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_SaveNetworkStateEvt( void )
{
#if defined ( NV_RESTORE )
 #if defined ( NV_TURN_OFF_RADIO )
  // Turn off the radio's receiver during an NV update
  byte RxOnIdle;
  byte x = false;
  ZMacGetReq( ZMacRxOnIdle, &RxOnIdle );
  ZMacSetReq( ZMacRxOnIdle, &x );
 #endif

  // Update the Network State in NV
  NLME_UpdateNV( NWK_NV_NIB_ENABLE        |
                 NWK_NV_DEVICELIST_ENABLE |
                 NWK_NV_BINDING_ENABLE    |
                 NWK_NV_ADDRMGR_ENABLE );

  // Reset the NV startup option to resume from NV by
  // clearing the "New" join option.
  zgWriteStartupOptions( FALSE, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );

 #if defined ( NV_TURN_OFF_RADIO )
  ZMacSetReq( ZMacRxOnIdle, &RxOnIdle );
 #endif
#endif  // NV_RESTORE
}

/*********************************************************************
 * @fn      ZDApp_RestoreNetworkState()
 *
 * @brief   This function will restore the network state of the
 *          device if the network state is stored in NV.
 *
 * @param   none
 *
 * @return
 *    ZDO_INITDEV_RESTORED_NETWORK_STATE  - The device's network state was
 *          restored.
 *    ZDO_INITDEV_NEW_NETWORK_STATE - The network state was not used.
 *          This could mean that zgStartupOption said to not restore, or
 *          it could mean that there was no network state to restore.
 *
 */
uint8 ZDApp_RestoreNetworkState( void )
{
  byte nvStat;
#if ( SECURE != 0 )
  nwkActiveKeyItems keyItems;
#endif

  // Initialize NWK NV items
  nvStat = NLME_InitNV();

  if ( nvStat != NV_OPER_FAILED )
  {
    if ( NLME_RestoreFromNV() )
    {
      // Are we a coordinator
      ZDAppNwkAddr.addr.shortAddr = NLME_GetShortAddr();
      if ( ZDAppNwkAddr.addr.shortAddr == 0 )
      {
        ZDO_Config_Node_Descriptor.LogicalType = NODETYPE_COORDINATOR;
      }
      devStartMode = MODE_RESUME;
    }
    else
      nvStat = NV_ITEM_UNINIT;

#if   ( SECURE != 0  )
    nwkFrameCounterChanges = 0;
    osal_memset( &keyItems, 0, sizeof( nwkActiveKeyItems ) );
    osal_nv_item_init( ZCD_NV_NWKKEY, sizeof(nwkActiveKeyItems), (void *)&keyItems );

  #if defined ( ZDO_COORDINATOR )
    ZDApp_RestoreNwkKey();
  #endif // ZDO_COORDINATOR
#endif // SECURE

    // The default for RxOnWhenIdle is true for RTR_NWK and false for end devices
    // [setup in the NLME_RestoreFromNV()].  Change it here if you want something
    // other than default.
  }

  if ( nvStat == ZSUCCESS )
    return ( ZDO_INITDEV_RESTORED_NETWORK_STATE );
  else
    return ( ZDO_INITDEV_NEW_NETWORK_STATE );
}

/*********************************************************************
 * @fn      ZDAppSetupProtoVersion()
 *
 * @brief   Setup the Network Protocol version
 *
 * NOTES:
 *   Take care of setting initial protocol value if we're possibly a
 *   Coordinator.
 *
 *   If DEF_PROTO_VERS macro is not defined get version
 *   from NV. if the NV version isn't valid default to Version 1.1.
 *
 *   if DEF_PROTO_VERS macro is defined respect it.
 *
 *   This initialization section works for End Devices as well.
 *
 *   There are two chores: make sure that if we're the Coordinator we start
 *   the correct network version, and set things up so that if we're a
 *   joining device we join the correct network. In both cases this init
 *   function runs so take care of both cases here.
 *
 * @param   none
 *
 * @return  none
 */
void ZDAppSetupProtoVersion( void )
{
  uint8 restore = 1;  // update NV or not: could save a flash erase cycle

#if !defined ( DEF_PROTO_VERS )
  uint8 protoVer = NLME_GetProtocolVersion(); // get  protocol version from NV

  // it is possible that it is uninitialized.
  if ((protoVer != ZB_PROT_V1_0) && (protoVer != ZB_PROT_V1_1))
  {
    // NV value not valid. 'protoVer' must be set.

    // For Coordinator behavior
    // CUSTOMER NOTE: change the following to default the started network
    // to a protocol version other than 1.1
    protoVer = ZB_PROT_V1_1;
  }
  else
  {
    // NV valid. 'protoVer' is valid.
    restore = 0;  // no need to update NV

    // For joining device behavior. Respect the NV version by making
    // all entries in the version array the same as the NV value.
    osal_memset(sPVerList, protoVer, sizeof(sPVerList));
  }
#else
  // macro defined. respect it.

  // don't compile if defined to an illegal value.
  #if (DEF_PROTO_VERS != ZB_PROT_V1_0) && (DEF_PROTO_VERS != ZB_PROT_V1_1)
    #error  No legal value for default protocol version
  #endif
  uint8 protoVer = DEF_PROTO_VERS;

  // For joining device behavior. respect the NV version by making
  // all entries in the version array the same as the macro value.
  osal_memset(sPVerList, protoVer, sizeof(sPVerList));
#endif   // DEF_PROTO_VERS

  // if we are or can be the Coordinator then we must update
  // it here. if we're going to be a joining device setting will be done in the
  // confirm callback if necessary.
  if (restore)
  {
    // we need to set NV to a (possibly) new value
    NLME_SetRequest(nwkProtocolVersion, 0, &protoVer);
  }
}

/*********************************************************************
 * @fn      ZDApp_InitUserDesc()
 *
 * @brief   Initialize the User Descriptor, the descriptor is read from NV
 *          when needed.  If you want to initialize the User descriptor to
 *          something other than all zero, do it here.
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_InitUserDesc( void )
{
  UserDescriptorFormat_t ZDO_DefaultUserDescriptor;

  // Initialize the User Descriptor, the descriptor is read from NV
  // when needed.  If you want to initialize the User descriptor to something
  // other than all zero, do it here.
  osal_memset( &ZDO_DefaultUserDescriptor, 0, sizeof( UserDescriptorFormat_t ) );
  if ( ZSUCCESS == osal_nv_item_init( ZCD_NV_USERDESC,
         sizeof(UserDescriptorFormat_t), (void*)&ZDO_DefaultUserDescriptor ) )
  {
    if ( ZSUCCESS == osal_nv_read( ZCD_NV_USERDESC, 0,
         sizeof(UserDescriptorFormat_t), (void*)&ZDO_DefaultUserDescriptor ) )
    {
      if ( ZDO_DefaultUserDescriptor.len != 0 )
      {
        ZDO_Config_Node_Descriptor.UserDescAvail = TRUE;
      }
    }
  }
}

/*********************************************************************
 * @fn      ZDAppCheckForHoldKey()
 *
 * @brief   Check for key to set the device into Hold Auto Start
 *
 * @param   none
 *
 * @return  none
 */
void ZDAppCheckForHoldKey( void )
{
#if (defined HAL_KEY) && (HAL_KEY == TRUE)
  // Get Keypad directly to see if a HOLD_START is needed.
  // Hold down the SW_BYPASS_START key (see OnBoard.h)
  // while booting to avoid starting up the device.
  if ( HalKeyRead () == SW_BYPASS_START)
  {
    // Change the device state to HOLD on start up
    devState = DEV_HOLD;
  }
#endif // HAL_KEY
}

/*********************************************************************
 * @fn      ZDApp_ProcessOSALMsg()
 *
 * @brief   Process the incoming task message.
 *
 * @param   msgPtr - message to process
 *
 * @return  none
 */
void ZDApp_ProcessOSALMsg( osal_event_hdr_t *msgPtr )
{
  // Data Confirmation message fields
  byte sentEP;       // This should always be 0
  byte sentStatus;
  afDataConfirm_t *afDataConfirm;

  switch ( msgPtr->event )
  {
    // Incoming ZDO Message
    case AF_INCOMING_MSG_CMD:
      ZDP_IncomingData( (afIncomingMSGPacket_t *)msgPtr );
      break;

    case AF_DATA_CONFIRM_CMD:
      // This message is received as a confirmation of a data packet sent.
      // The status is of ZStatus_t type [defined in NLMEDE.h]
      // The message fields are defined in AF.h
      afDataConfirm = (afDataConfirm_t *)msgPtr;
      sentEP = afDataConfirm->endpoint;
      sentStatus = afDataConfirm->hdr.status;

      // Action taken when confirmation is received.
      /* Put code here */
#if !defined ( RTR_NWK )
      if ( sentStatus == ZMacNoACK )
      {
        //ZDApp_SendMsg( ZDAppTaskID, ZDO_NWK_JOIN_REQ, sizeof(osal_event_hdr_t), NULL );
      }
#else
     (void)sentStatus;
#endif
      break;

    case ZDO_NWK_DISC_CNF:
      if (devState != DEV_NWK_DISC)
      {
      }
#if !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
  #if defined ( MANAGED_SCAN )
      else if ( (((ZDO_NetworkDiscoveryCfm_t *)msgPtr)->hdr.status == ZDO_SUCCESS) && (zdoDiscCounter > NUM_DISC_ATTEMPTS) )
  #else
      else if ( (((ZDO_NetworkDiscoveryCfm_t *)msgPtr)->hdr.status == ZDO_SUCCESS) && (zdoDiscCounter++ > NUM_DISC_ATTEMPTS) )
  #endif
      {
        if ( devStartMode == MODE_JOIN )
        {
          devState = DEV_NWK_JOINING;

          ZDApp_NodeProfileSync((ZDO_NetworkDiscoveryCfm_t *)msgPtr);

          if ( NLME_JoinRequest( ((ZDO_NetworkDiscoveryCfm_t *)msgPtr)->extendedPANID,
               BUILD_UINT16( ((ZDO_NetworkDiscoveryCfm_t *)msgPtr)->panIdLSB, ((ZDO_NetworkDiscoveryCfm_t *)msgPtr)->panIdMSB ),
               ((ZDO_NetworkDiscoveryCfm_t *)msgPtr)->logicalChannel,
               ZDO_Config_Node_Descriptor.CapabilityFlags ) != ZSuccess )
          {
            ZDApp_NetworkInit( (uint16)(NWK_START_DELAY
                + ((uint16)(osal_rand()& EXTENDED_JOINING_RANDOM_MASK))) );
          }
        }
        else if ( devStartMode == MODE_REJOIN )
        {
          devState = DEV_NWK_REJOIN;
          if ( NLME_ReJoinRequest() != ZSuccess )
          {
            ZDApp_NetworkInit( (uint16)(NWK_START_DELAY
                + ((uint16)(osal_rand()& EXTENDED_JOINING_RANDOM_MASK))) );
          }
        }

        if ( ZDO_Config_Node_Descriptor.CapabilityFlags & CAPINFO_RCVR_ON_IDLE )
        {
          // The receiver is on, turn network layer polling off.
          NLME_SetPollRate( 0 );
          NLME_SetQueuedPollRate( 0 );
          NLME_SetResponseRate( 0 );
        }
      }
      else
      {
#if defined ( SOFT_START ) && !defined ( VIRTKEY_SOFT_START )
  #if defined ( MANAGED_SCAN )
        if ( (softStartAllowCoord)
            && (((ZDO_NetworkDiscoveryCfm_t *)msgPtr)->hdr.status != ZDO_SUCCESS )
              && (zdoDiscCounter > NUM_DISC_ATTEMPTS) )
  #else
        if ( (softStartAllowCoord)
            && (((ZDO_NetworkDiscoveryCfm_t *)msgPtr)->hdr.status != ZDO_SUCCESS )
              && (zdoDiscCounter++ > NUM_DISC_ATTEMPTS) )
  #endif
        {
          ZDO_Config_Node_Descriptor.LogicalType = NODETYPE_COORDINATOR;
          devStartMode = MODE_HARD;
        }
        else if ( continueJoining == FALSE )
        {
          devState = DEV_HOLD;
          osal_stop_timerEx( ZDAppTaskID, ZDO_NETWORK_INIT );
          break;    // Don't init
        }
#endif
  #if defined ( MANAGED_SCAN )
        ZDApp_NetworkInit( MANAGEDSCAN_DELAY_BETWEEN_SCANS );
  #else
        if ( continueJoining )
        {
          ZDApp_NetworkInit( (uint16)(BEACON_REQUEST_DELAY
              + ((uint16)(osal_rand()& BEACON_REQ_DELAY_MASK))) );
        }
  #endif
      }
#endif  // !ZDO_COORDINATOR
      break;

#if !defined( ZDO_COORDINATOR ) || defined( SOFT_START )
    case ZDO_NWK_JOIN_IND:
      ZDApp_ProcessNetworkJoin();
      break;

    case ZDO_NWK_JOIN_REQ:
      retryCnt = 0;
      devStartMode = MODE_RESUME;
      _tmpRejoinState = true;
      zgDefaultStartingScanDuration = BEACON_ORDER_60_MSEC;
      ZDApp_NetworkInit( 0 );

      // indicate state change to apps
      devState = DEV_INIT;
      osal_set_event( ZDAppTaskID, ZDO_STATE_CHANGE_EVT );
      break;
#endif  // !ZDO_COORDINATOR

#if defined ( ZDSECMGR_SECURE )
  #if defined ( ZDSECMGR_COMMERCIAL )
        case ZDO_ESTABLISH_KEY_CFM:
          ZDSecMgrEstablishKeyCfm
            ( (ZDO_EstablishKeyCfm_t*)msgPtr );
          break;
  #endif

  #if defined ( ZDSECMGR_COMMERCIAL )
    #if !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
        case ZDO_ESTABLISH_KEY_IND:
          ZDSecMgrEstablishKeyInd
            ( (ZDO_EstablishKeyInd_t*)msgPtr );
          break;
    #endif
  #endif

  #if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
        case ZDO_TRANSPORT_KEY_IND:
          ZDSecMgrTransportKeyInd
            ( (ZDO_TransportKeyInd_t*)msgPtr );
          break;
  #endif

  #if defined ( ZDO_COORDINATOR )
        case ZDO_UPDATE_DEVICE_IND:
          ZDSecMgrUpdateDeviceInd
            ( (ZDO_UpdateDeviceInd_t*)msgPtr );
          break;
  #endif

  #if defined ( RTR_NWK )
    #if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
        case ZDO_REMOVE_DEVICE_IND:
          ZDSecMgrRemoveDeviceInd
            ( (ZDO_RemoveDeviceInd_t*)msgPtr );
          break;
    #endif
  #endif

  #if defined ( ZDSECMGR_COMMERCIAL )
    #if defined ( ZDO_COORDINATOR )
        case ZDO_REQUEST_KEY_IND:
          ZDSecMgrRequestKeyInd
            ( (ZDO_RequestKeyInd_t*)msgPtr );
          break;
    #endif
  #endif

  #if !defined ( ZDO_COORDINATOR ) || defined( SOFT_START )
        case ZDO_SWITCH_KEY_IND:
          ZDSecMgrSwitchKeyInd
            ( (ZDO_SwitchKeyInd_t*)msgPtr );
          break;
  #endif

#endif // defined ( ZDSECMGR_SECURE )

    default:
      break;
  }

  (void)sentEP;
}

#if !defined( ZDO_COORDINATOR ) || defined( SOFT_START )
/*********************************************************************
 * @fn      ZDApp_ProcessNetworkJoin()
 *
 * @brief
 *
 *   Save off the Network key information.
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_ProcessNetworkJoin( void )
{
  if ( (devState == DEV_NWK_JOINING) ||
      ((devState == DEV_NWK_ORPHAN)  &&
       (ZDO_Config_Node_Descriptor.LogicalType == NODETYPE_ROUTER)) )
  {
    // Result of a Join attempt by this device.
    if ( nwkStatus == ZSuccess )
    {
      osal_set_event( ZDAppTaskID, ZDO_STATE_CHANGE_EVT );

#if defined ( POWER_SAVING )
      osal_pwrmgr_device( PWRMGR_BATTERY );
#endif

#if   ( SECURE != 0  )
      if ( _NIB.SecurityLevel && (ZDApp_RestoreNwkKey() == false ) )
      {
        // wait for auth from trust center!!
        devState = DEV_END_DEVICE_UNAUTH;

        // Start the reset timer for MAX UNAUTH time
        ZDApp_ResetTimerStart( MAX_DEVICE_UNAUTH_TIMEOUT );
      }
      else
#endif  // SECURE
      {
#if defined ( RTR_NWK )
        if ( devState == DEV_NWK_ORPHAN
            && ZDO_Config_Node_Descriptor.LogicalType != NODETYPE_DEVICE )
        {
          // Change NIB state to router for restore
          _NIB.nwkState = NWK_ROUTER;
        }
#endif
        devState = DEV_END_DEVICE;
#if defined ( RTR_NWK )
        // NOTE: first two parameters are not used, see NLMEDE.h for details
  #if !defined (AUTO_SOFT_START)
        if ( ZDO_Config_Node_Descriptor.LogicalType != NODETYPE_DEVICE )
        {
          NLME_StartRouterRequest( 0, 0, false );
        }
  #endif // AUTO_SOFT_START
#endif  // RTR

#if defined ( ZDO_ENDDEVICE_ANNCE_GENERATE )
        ZDP_EndDeviceAnnce( ZDAppNwkAddr.addr.shortAddr, saveExtAddr,
                           ZDO_Config_Node_Descriptor.CapabilityFlags, 0 );
#endif
      }
    }
    else
    {
      if ( (devStartMode == MODE_RESUME) && (++retryCnt >= MAX_RESUME_RETRY) )
      {
        if ( _NIB.nwkPanId == 0xFFFF || _NIB.nwkPanId == INVALID_PAN_ID )
          devStartMode = MODE_JOIN;
        else
        {
          devStartMode = MODE_REJOIN;
          _tmpRejoinState = true;
        }
      }

      if ( (NLME_GetShortAddr() != INVALID_NODE_ADDR) ||
           (_NIB.nwkDevAddress != INVALID_NODE_ADDR) )
      {
        uint16 addr = INVALID_NODE_ADDR;
        // Invalidate nwk addr so end device does not use in its data reqs.
        _NIB.nwkDevAddress = INVALID_NODE_ADDR;
        ZMacSetReq( ZMacShortAddress, (byte *)&addr );
      }

      zdoDiscCounter = 1;

//      ZDApp_NetworkInit( (uint16)
//                         ((NWK_START_DELAY * (osal_rand() & 0x0F)) +
//                          (NWK_START_DELAY * 5)) );
      ZDApp_NetworkInit( (uint16)(NWK_START_DELAY
           + ((uint16)(osal_rand()& EXTENDED_JOINING_RANDOM_MASK))) );
    }
  }
  else if ( devState == DEV_NWK_ORPHAN || devState == DEV_NWK_REJOIN )
  {
    // results of an orphaning attempt by this device
    if (nwkStatus == ZSuccess)
    {
#if ( SECURE != 0 )
      ZDApp_RestoreNwkKey();
#endif
      devState = DEV_END_DEVICE;
      osal_set_event( ZDAppTaskID, ZDO_STATE_CHANGE_EVT );
      // setup Power Manager Device
#if defined ( POWER_SAVING )
      osal_pwrmgr_device( PWRMGR_BATTERY );
#endif

      if ( ZDO_Config_Node_Descriptor.CapabilityFlags & CAPINFO_RCVR_ON_IDLE )
      {
        // The receiver is on, turn network layer polling off.
        NLME_SetPollRate( 0 );
        NLME_SetQueuedPollRate( 0 );
        NLME_SetResponseRate( 0 );
      }

#if defined ( ZDO_ENDDEVICE_ANNCE_GENERATE )
      ZDP_EndDeviceAnnce( ZDAppNwkAddr.addr.shortAddr, saveExtAddr,
                         ZDO_Config_Node_Descriptor.CapabilityFlags, 0 );
#endif

    }
    else
    {
      if ( (devStartMode == MODE_RESUME) && (++retryCnt >= MAX_RESUME_RETRY) )
      {
        if ( _NIB.nwkPanId == 0xFFFF || _NIB.nwkPanId == INVALID_PAN_ID )
          devStartMode = MODE_JOIN;
        else
        {
          devStartMode = MODE_REJOIN;
          _tmpRejoinState = true;
        }
      }

      // setup a retry for later...
      ZDApp_NetworkInit( (uint16)(NWK_START_DELAY
           + (osal_rand()& EXTENDED_JOINING_RANDOM_MASK)) );
    }
  }
  else
  {
    // this is an error case!!
  }
}
#endif // !ZDO_COORDINATOR

#if ( SECURE != 0 )
/*********************************************************************
 * @fn      ZDApp_SaveNwkKey()
 *
 * @brief   Save off the Network key information.
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_SaveNwkKey( void )
{
  nwkActiveKeyItems keyItems;

  SSP_ReadNwkActiveKey( &keyItems );
  keyItems.frameCounter++;

  osal_nv_write( ZCD_NV_NWKKEY, 0, sizeof( nwkActiveKeyItems ),
                (void *)&keyItems );

  nwkFrameCounterChanges = 0;
}

/*********************************************************************
 * @fn      ZDApp_ResetNwkKey()
 *
 * @brief   Reset the Network key information in NV.
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_ResetNwkKey( void )
{
  nwkActiveKeyItems keyItems;

  osal_memset( &keyItems, 0, sizeof( nwkActiveKeyItems ) );
  osal_nv_write( ZCD_NV_NWKKEY, 0, sizeof( nwkActiveKeyItems ),
                (void *)&keyItems );
}
#endif

#if ( SECURE != 0 )
/*********************************************************************
 * @fn      ZDApp_RestoreNwkKey()
 *
 * @brief
 *
 *   Save off the Network key information.
 *
 * @param   none
 *
 * @return  true if restored from NV, false if not
 */
byte ZDApp_RestoreNwkKey( void )
{
  nwkActiveKeyItems keyItems;
  byte ret = false;

  if ( osal_nv_read( ZCD_NV_NWKKEY, 0, sizeof(nwkActiveKeyItems), (void*)&keyItems )
      == ZSUCCESS )
  {
    if ( keyItems.frameCounter > 0 )
    {
      // Restore the key information
      keyItems.frameCounter += MAX_NWK_FRAMECOUNTER_CHANGES;
      SSP_WriteNwkActiveKey( &keyItems );
      ret = true;
    }
    nwkFrameCounterChanges = MAX_NWK_FRAMECOUNTER_CHANGES; // Force a save for the first
  }
  return ( ret );
}
#endif

/*********************************************************************
 * @fn      ZDApp_SendEndDeviceBindReq()
 *
 * @brief
 *
 *   This function will look up the endpoint description
 *   and send an End Device Bind Request message.
 *
 * @param  endPoint - Endpoint to auto find
 *
 * @return  none
 */
void ZDApp_SendEndDeviceBindReq( byte endPoint )
{
#if defined ( ZDO_ENDDEVICEBIND_REQUEST )
  zAddrType_t dstAddr;
  SimpleDescriptionFormat_t *sDesc;
  byte free;
  if ( (endPoint == ZDO_EP) || (endPoint > MAX_ENDPOINTS) )
  {
    return;   // Can't do for ZDO
  }

  HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

  free = afFindSimpleDesc( &sDesc, endPoint );
  if ( sDesc != NULL )
  {
    dstAddr.addrMode = Addr16Bit;
    dstAddr.addr.shortAddr = 0;   // Zigbee Coordinator
    ZDP_EndDeviceBindReq( &dstAddr,
#if defined ( REFLECTOR  )
                // We have a reflector, so tell the coordinator to send
                // binding messages this way
                NLME_GetShortAddr(),
#else
                // tell the coordinator to store if available
                NWK_PAN_COORD_ADDR,
#endif
                endPoint,sDesc->AppProfId,
                sDesc->AppNumOutClusters, sDesc->pAppOutClusterList,
                sDesc->AppNumInClusters, sDesc->pAppInClusterList,
                0 );

    if ( free )
    {
      osal_mem_free( sDesc );
    }
  }
  else
  {

  }
#endif // ZDO_ENDDEVICEBIND_REQUEST
}

/*********************************************************************
 * @fn      ZDApp_AutoFindDestination()
 *
 * @brief
 *
 *   This function will try to find the Input Match for this device's
 *   (endpoint passed in) outputs.
 *
 * @param  endPoint - Endpoint to auto find
 * @param  task_id  - task ID override, if NULL use endpoint desc's
 *                    task_id
 *
 * @return  none
 */
void ZDApp_AutoFindDestinationEx( byte endPoint, uint8 *task_id )
{
#if defined ( ZDO_MATCH_REQUEST )
  zAddrType_t dstAddr;
  SimpleDescriptionFormat_t *sDesc;
  endPointDesc_t *tmpDesc;
  if ( endPoint == ZDO_EP )
    return;   // Can't do for ZDO

  HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

  tmpDesc = afFindEndPointDesc( endPoint );
  if ( tmpDesc )
  {
    if ( ZDApp_AutoFindMode_epDesc == NULL )
    {
      ZDApp_AutoFindMode_epDesc = (endPointDesc_t *)osal_mem_alloc( sizeof ( endPointDesc_t ) );
    }

    if ( ZDApp_AutoFindMode_epDesc )
    {
      osal_memcpy( ZDApp_AutoFindMode_epDesc, tmpDesc, sizeof( endPointDesc_t ) );
      if ( task_id )
      {
        // Override the Task ID, if needed.
        ZDApp_AutoFindMode_epDesc->task_id = task_id;
      }

      sDesc = ZDApp_AutoFindMode_epDesc->simpleDesc;

      // This message is sent to everyone
      dstAddr.addrMode = AddrBroadcast;
      dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR, sDesc->AppProfId,
                        sDesc->AppNumOutClusters, sDesc->pAppOutClusterList,
                        sDesc->AppNumInClusters, sDesc->pAppInClusterList, 0 );

    }
  }
  else
  {
  }
#endif // ZDO_MATCH_REQUEST
}

/*********************************************************************
 * @fn      ZDApp_ResetTimerStart
 *
 * @brief   Start the reset timer.
 *
 * @param   delay - delay time(ms) before reset
 *
 * @return  none
 */
void ZDApp_ResetTimerStart( uint16 delay )
{
  // Start the rest timer
  osal_start_timerEx( ZDAppTaskID, ZDO_DEVICE_RESET, delay );
}

/*********************************************************************
 * @fn      ZDApp_ResetTimerCancel
 *
 * @brief   Cancel the reset timer.
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_ResetTimerCancel( void )
{
  // Cancel the reset timer
  osal_stop_timerEx( ZDAppTaskID, ZDO_DEVICE_RESET );
}

/*********************************************************************
 * @fn      ZDApp_LeaveCtrlInit
 *
 * @brief   Initialize the leave control logic.
 *
 * @param   none
 *
 * @return  none
 */
void ZDApp_LeaveCtrlInit( void )
{
  uint8 status;


  // Initialize control state
  ZDApp_LeaveCtrl = ZDAPP_LEAVE_CTRL_INIT;

  status = osal_nv_item_init( ZCD_NV_LEAVE_CTRL,
                              sizeof(ZDApp_LeaveCtrl),
                              &ZDApp_LeaveCtrl );

  if ( status == ZSUCCESS )
  {
    // Read saved control
    osal_nv_read( ZCD_NV_LEAVE_CTRL,
                  0,
                  sizeof( uint8 ),
                  &ZDApp_LeaveCtrl);
  }
}

/*********************************************************************
 * @fn      ZDApp_LeaveCtrlSet
 *
 * @brief   Set the leave control logic.
 *
 * @param   ra - reassociate flag
 *
 * @return  none
 */
void ZDApp_LeaveCtrlSet( uint8 ra )
{
  ZDApp_LeaveCtrl = ZDAPP_LEAVE_CTRL_SET;

  if ( ra == TRUE )
  {
    ZDApp_LeaveCtrl |= ZDAPP_LEAVE_CTRL_RA;
  }

  // Write the leave control
  osal_nv_write( ZCD_NV_LEAVE_CTRL,
                 0,
                 sizeof( uint8 ),
                 &ZDApp_LeaveCtrl);
}

/*********************************************************************
 * @fn      ZDApp_LeaveCtrlBypass
 *
 * @brief   Check if NV restore should be skipped during a leave reset.
 *
 * @param   none
 *
 * @return  uint8 - (TRUE bypass:FALSE do not bypass)
 */
uint8 ZDApp_LeaveCtrlBypass( void )
{
  uint8 bypass;

  if ( ZDApp_LeaveCtrl & ZDAPP_LEAVE_CTRL_SET )
  {
    bypass = TRUE;
  }
  else
  {
    bypass = FALSE;
  }

  return bypass;
}

/*********************************************************************
 * @fn      ZDApp_LeaveCtrlStartup
 *
 * @brief   Check for startup conditions during a leave reset.
 *
 * @param   state      - devState_t determined by leave control logic
 * @param   startDelay - startup delay
 *
 * @return  none
 */
void ZDApp_LeaveCtrlStartup( devStates_t* state, uint16* startDelay )
{
  *startDelay = 0;

  if ( ZDApp_LeaveCtrl & ZDAPP_LEAVE_CTRL_SET )
  {
    if ( ZDApp_LeaveCtrl & ZDAPP_LEAVE_CTRL_RA )
    {
      *startDelay = 5000;
    }
    else
    {
      *state = DEV_HOLD;
    }

    // Set leave control to initialized state
    ZDApp_LeaveCtrl = ZDAPP_LEAVE_CTRL_INIT;

    // Write initialized control
    osal_nv_write( ZCD_NV_LEAVE_CTRL,
                  0,
                  sizeof( uint8 ),
                  &ZDApp_LeaveCtrl);
  }
}

/*********************************************************************
 * @fn      ZDApp_LeaveReset
 *
 * @brief   Setup a device reset due to a leave indication/confirm.
 *
 * @param   ra - reassociate flag
 *
 * @return  none
 */
void ZDApp_LeaveReset( uint8 ra )
{
  ZDApp_LeaveCtrlSet( ra );

  ZDApp_ResetTimerStart( 5000 );
}

/*********************************************************************
 * @fn      ZDApp_LeaveUpdate
 *
 * @brief   Update local device data related to leaving device.
 *
 * @param   nwkAddr        - NWK address of leaving device
 * @param   extAddr        - EXT address of leaving device
 * @param   removeChildren - remove children of leaving device
 *
 * @return  none
 */
void ZDApp_LeaveUpdate( uint16 nwkAddr, uint8* extAddr,
                        uint8 removeChildren )
{
  /*
  AddrMgrEntry_t entry;
  */


  // Remove if child
  NLME_RemoveChild( extAddr, removeChildren );

  /*
  // Set NWK address to invalid
  entry.user    = ADDRMGR_USER_DEFAULT;
  entry.nwkAddr = INVALID_NODE_ADDR;
  AddrMgrExtAddrSet( entry.extAddr, extAddr );
  AddrMgrEntryUpdate( &entry );

  // Check
  if ( removeChildren == TRUE )
  {
    // Set index to INVALID_NODE_ADDR to start search
    entry.index = INVALID_NODE_ADDR;

    // Get first entry
    AddrMgrEntryGetNext( &entry );

    // Remove all descendents
    while ( entry.index != INVALID_NODE_ADDR )
    {
      // Check NWK address allocation algorithm
      if ( RTG_ANCESTOR( entry.nwkAddr, thisAddr ) != 0 )
      {
        // Set NWK address to invalid
        entry.nwkAddr = INVALID_NODE_ADDR;
        AddrMgrEntryUpdate( &entry );
      }

      // Get next entry
      AddrMgrEntryGetNext( &entry );
    }
  }
  */
}

/*********************************************************************
 * CALLBACK FUNCTIONS
 */

#if defined ( ZDO_COORDINATOR )
/*********************************************************************
 * @fn      ZDApp_EndDeviceBindReqCB()
 *
 * @brief
 *
 *   Called by ZDO when an End Device Bind Request message is received.
 *
 * @param  bindReq  - binding request information
 * @param  SecurityUse - Security enable/disable
 *
 * @return  none
 */
void ZDApp_EndDeviceBindReqCB( ZDEndDeviceBind_t *bindReq )
{
#if defined ( COORDINATOR_BINDING )
  if ( bindReq->localCoordinator == 0x0000 )
  {
    ZDO_DoEndDeviceBind( bindReq );
  }
  else
#endif // COORDINATOR_BINDING
  {
    ZDO_MatchEndDeviceBind( bindReq );
  }
}
#endif // ZDO_COORDINATOR

#if !defined ( REFLECTOR ) && defined ( ZDO_BIND_UNBIND_RESPONSE )
/*********************************************************************
 * @fn      ZDApp_AppBindReq()
 *
 * @brief
 *
 *   Called to send an App Bind Request message.
 *
 * @param  SrcAddr     - Source address ( who sent the message )
 * @param  SrcAddress  - Source Address (64 bit)
 * @param  SrcEndPoint - Source endpoint
 * @param  ClusterID   - Cluster ID
 * @param  DstAddress  - Destination Address (64 bit)
 * @param  DstEndPoint - Destination endpoint
 * @param  SecurityUse - Security enable/disable
 *
 * @return  none
 */
void ZDApp_AppBindReq( byte TransSeq, zAddrType_t *SrcAddr, byte *SrcAddress,
                      byte SrcEndPoint, cId_t ClusterID, byte *DstAddress,
                      byte DstEndPoint, byte SecurityUse, uint8 Type )
{
  ZDO_BindReq_t *pBindReq;

  if ( ZDApp_BindReq_TaskID )
  {
    // Send the IEEE Address response structure to the registered task
    pBindReq = (ZDO_BindReq_t *)osal_msg_allocate( sizeof( ZDO_BindReq_t ) );
    if ( pBindReq )
    {
      pBindReq->event_hdr.event = Type;

      // Build the structure
      pBindReq->hdr.srcAddr = SrcAddr->addr.shortAddr;
      pBindReq->hdr.transSeq = TransSeq;
      pBindReq->hdr.SecurityUse = SecurityUse;

      osal_cpyExtAddr( pBindReq->srcAddr, SrcAddress );
      pBindReq->srcEP = SrcEndPoint;
      pBindReq->clusterID = ClusterID;
      osal_cpyExtAddr( pBindReq->dstAddr, DstAddress );
      pBindReq->dstEP = DstEndPoint;

      osal_msg_send( ZDApp_BindReq_TaskID, (uint8 *)pBindReq );
    }
  }
}
#endif // !REFLECTOR && ZDO_BIND_UNBIND_RESPONSE

#if defined ( REFLECTOR ) || defined ( ZDO_BIND_UNBIND_RESPONSE )
/*********************************************************************
 * @fn      ZDApp_BindReqCB()
 *
 * @brief
 *
 *   Called by ZDO when a Bind Request message is received.
 *
 * @param  SrcAddr     - Source address ( who sent the message )
 * @param  SrcAddress  - Source Address (64 bit)
 * @param  SrcEndPoint - Source endpoint
 * @param  ClusterID   - Cluster ID
 * @param  DstAddress  - Destination Address (64 bit)
 * @param  DstEndPoint - Destination endpoint
 * @param  SecurityUse - Security enable/disable
 *
 * @return  none
 */
void ZDApp_BindReqCB( byte TransSeq, zAddrType_t *SrcAddr, byte *SrcAddress,
                      byte SrcEndPoint, cId_t ClusterID, zAddrType_t *DstAddress,
                      byte DstEndPoint, byte SecurityUse )
{
#if defined ( REFLECTOR )
  zAddrType_t SourceAddr;       // Binding Source addres
  byte bindStat;

  SourceAddr.addrMode = Addr64Bit;
  osal_cpyExtAddr( SourceAddr.addr.extAddr, SrcAddress );

  if ( DstAddress->addrMode != Addr64Bit &&
         DstAddress->addrMode != AddrGroup )
  {
    bindStat = ZDP_NOT_SUPPORTED;
  }

  else
  { // Check source endpoints
    if ( SrcEndPoint == 0 || SrcEndPoint > MAX_ENDPOINTS )
    {
      bindStat = ZDP_INVALID_EP;
    }
    // Check the destination endpoints for ext address mode
    else if ( ( DstAddress->addrMode == Addr64Bit ) &&
            ( DstEndPoint == 0 || DstEndPoint > MAX_ENDPOINTS ) )
    {
      bindStat = ZDP_INVALID_EP;
    }

    else
    {
#if defined ( ZDO_NWKADDR_REQUEST )
      {
        uint16 nwkAddr;

        // Check for the source address
        if ( APSME_LookupNwkAddr( SrcAddress, &nwkAddr ) == FALSE )
        {
          ZDP_NwkAddrReq( SrcAddress, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
        }

        // Check for the destination address
        if ( DstAddress->addrMode == Addr64Bit )
        {
          if ( APSME_LookupNwkAddr( DstAddress->addr.extAddr, &nwkAddr ) == FALSE )
          {
            ZDP_NwkAddrReq( DstAddress->addr.extAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
          }
        }
      }
#endif

      if ( APSME_BindRequest( &SourceAddr, SrcEndPoint, ClusterID,
                     DstAddress, DstEndPoint ) == ZSuccess )
      {
        bindStat = ZDP_SUCCESS;
        // Notify to save info into NV
        osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );
      }
      else
        bindStat = ZDP_TABLE_FULL;
    }
  }
  // Send back a response message
  ZDP_BindRsp( TransSeq, SrcAddr, bindStat, SecurityUse );

#else  // must be ZDO_BIND_UNBIND_RESPONSE

  ZDApp_AppBindReq( TransSeq, SrcAddr, SrcAddress, SrcEndPoint, ClusterID,
                    DstAddress->addr.extAddr, DstEndPoint, SecurityUse, ZDO_BIND_REQUEST );

#endif // REFLECTOR
}
#endif // REFLECTOR OR ZDO_BIND_UNBIND_RESPONSE

#if defined ( REFLECTOR ) || defined ( ZDO_BIND_UNBIND_RESPONSE )
/*********************************************************************
 * @fn      ZDApp_UnbindReqCB()
 *
 * @brief
 *
 *   Called by ZDO when an Unbind Request message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  SrcAddress - Source Address (64 bit)
 * @param  SrcEndPoint - Source endpoint
 * @param  ClusterID - Cluster ID
 * @param  DstAddress - Destination Address (64 bit)
 * @param  DstEndPoint - Destination endpoint
 * @param  SecurityUse - Security enable/disable
 *
 * @return  none
 */
void ZDApp_UnbindReqCB( byte TransSeq, zAddrType_t *SrcAddr, byte *SrcAddress,
                        byte SrcEndPoint, cId_t ClusterID, zAddrType_t *DstAddress,
                        byte DstEndPoint, byte SecurityUse )
{
#if defined ( REFLECTOR )
  zAddrType_t SourceAddr;       // Binding Source addres
  byte bindStat;

  SourceAddr.addrMode = Addr64Bit;
  osal_cpyExtAddr( SourceAddr.addr.extAddr, SrcAddress );

  // Check endpoints
  if ( SrcEndPoint == 0 || SrcEndPoint > MAX_ENDPOINTS ||
       DstEndPoint == 0 || DstEndPoint > MAX_ENDPOINTS )
  {
    bindStat = ZDP_INVALID_EP;
  }

  else
  {
    if ( APSME_UnBindRequest( &SourceAddr, SrcEndPoint, ClusterID,
                             DstAddress, DstEndPoint ) == ZSuccess )
    {
      bindStat = ZDP_SUCCESS;

      // Notify to save info into NV
      osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );
    }
    else
      bindStat = ZDP_NO_ENTRY;
  }

  // Send back a response message
  ZDP_UnbindRsp( TransSeq, SrcAddr, bindStat, SecurityUse );

#else // Must be ZDO_BIND_UNBIND_RESPONSE

  ZDApp_AppBindReq( TransSeq, SrcAddr, SrcAddress, SrcEndPoint, ClusterID,
                    DstAddress->addr.extAddr, DstEndPoint, SecurityUse, ZDO_UNBIND_REQUEST );

#endif // ZDO_BIND_UNBIND_RESPONSE
}
#endif // REFLECTOR OR ZDO_BIND_UNBIND_RESPONSE

/*********************************************************************
 * @fn      ZDApp_SendNewDstAddr()
 *
 * @brief
 *
 *   Used to send an OSAL message to an application that contains a
 *   new destination address
 *
 * @param  dstEP  - Destination endpoint
 * @param  dstAddr - response status
 * @param  clusterID - relavent cluster for this dst address
 * @param  removeFlag - false if add, true to remove
 * @param  task_id - What task to send it to
 * @param  endpoint - who the new address is for
 *
 * @return  none
 */
void ZDApp_SendNewDstAddr( byte dstEP, zAddrType_t *dstAddr,
                   cId_t clusterID, byte removeFlag, byte task_id, byte endpoint )
{
  byte bufLen;
  ZDO_NewDstAddr_t *msgPtr;

  // Send the address to the task
  bufLen = sizeof(ZDO_NewDstAddr_t);

  msgPtr = (ZDO_NewDstAddr_t *)osal_msg_allocate( bufLen );
  if ( msgPtr )
  {
    msgPtr->hdr.event = ZDO_NEW_DSTADDR;
    msgPtr->dstAddrDstEP = dstEP;
    osal_memcpy(&msgPtr->dstAddr, dstAddr, sizeof( zAddrType_t ) );
    msgPtr->dstAddrClusterIDLSB = LO_UINT16( clusterID );
    msgPtr->dstAddrClusterIDMSB = HI_UINT16( clusterID );
    msgPtr->dstAddrRemove = removeFlag;
    msgPtr->dstAddrEP = endpoint;

    osal_msg_send( task_id, (uint8 *)msgPtr );
  }
}

/*********************************************************************
 * @fn      ZDApp_SendEventMsg()
 *
 * @brief
 *
 *   Sends a Network Join message
 *
 * @param  cmd - command ID
 * @param  len - length (in bytes) of the buf field
 * @param  buf - buffer for the rest of the message.
 *
 * @return  none
 */
void ZDApp_SendEventMsg( byte cmd, byte len, byte *buf )
{
  ZDApp_SendMsg( ZDAppTaskID, cmd, len, buf );
}

/*********************************************************************
 * @fn      ZDApp_SendMsg()
 *
 * @brief   Sends a OSAL message
 *
 * @param  taskID - Where to send the message
 * @param  cmd - command ID
 * @param  len - length (in bytes) of the buf field
 * @param  buf - buffer for the rest of the message.
 *
 * @return  none
 */
void ZDApp_SendMsg( byte taskID, byte cmd, byte len, byte *buf )
{
  osal_event_hdr_t *msgPtr;

  // Send the address to the task
  msgPtr = (osal_event_hdr_t *)osal_msg_allocate( len );
  if ( msgPtr )
  {
    if ( (len > 0) && (buf != NULL) )
      osal_memcpy( msgPtr, buf, len );

    msgPtr->event = cmd;
    osal_msg_send( taskID, (byte *)msgPtr );
  }
}

#if defined ( ZDO_NWKADDR_REQUEST )
/*********************************************************************
 * @fn      ZDApp_NwkAddrRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a NWK_addr_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  IEEEAddr - 64 bit IEEE address of device
 * @param  aoi - 16 bit network address of interest.
 * @param  NumAssocDev - number of associated devices to reporting device
 * @param  AssocDevList - array short addresses of associated devices
 *
 * @return  none
 */
void ZDApp_NwkAddrRspCB( zAddrType_t *SrcAddr, byte Status, byte *IEEEAddr,
                         uint16 nwkAddr, byte NumAssocDev,
                         byte StartIndex, uint16 *AssocDevList )
{
  uint8 bufLen;
  ZDO_NwkAddrResp_t *pNwkAddrRsp;

#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_NWK_ADDR_RSP )
  {
    zdo_MTCB_NwkIEEEAddrRspCB( SPI_CB_ZDO_NWK_ADDR_RSP, SrcAddr, Status,
                  IEEEAddr, nwkAddr, NumAssocDev, StartIndex, AssocDevList );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( ZDApp_NwkAddrRsp_TaskID )
  {
    // Send the NWK Address response structure to the registered task
    bufLen = sizeof( ZDO_NwkAddrResp_t ) + sizeof( uint16 ) * NumAssocDev;

    pNwkAddrRsp = (ZDO_NwkAddrResp_t *)osal_msg_allocate( bufLen );

    if ( pNwkAddrRsp )
    {
      pNwkAddrRsp->hdr.event = ZDO_NWK_ADDR_RESP;

      // Build the structure
      pNwkAddrRsp->nwkAddr = nwkAddr;
      osal_cpyExtAddr( pNwkAddrRsp->extAddr, IEEEAddr );
      pNwkAddrRsp->numAssocDevs = NumAssocDev;
      pNwkAddrRsp->startIndex = StartIndex;
      osal_memcpy( pNwkAddrRsp->devList, AssocDevList, (sizeof( uint16 ) * NumAssocDev) );

      osal_msg_send( ZDApp_NwkAddrRsp_TaskID, (uint8 *)pNwkAddrRsp );
    }
  }
}
#endif // ZDO_NWKADDR_REQUEST

#if defined ( ZDO_IEEEADDR_REQUEST )
/*********************************************************************
 * @fn      ZDApp_IEEEAddrRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a NWK_addr_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  IEEEAddr - 64 bit IEEE address of device
 * @param  aoi - 16 bit network address of interest.
 * @param  NumAssocDev - number of associated devices to reporting device
 * @param  AssocDevList - array short addresses of associated devices
 *
 * @return  none
 */
void ZDApp_IEEEAddrRspCB( zAddrType_t *SrcAddr, byte Status, byte *IEEEAddr,
                          uint16 aoi, byte NumAssocDev,
                          byte StartIndex, uint16 *AssocDevList )
{
  uint8 bufLen;
  ZDO_IEEEAddrResp_t *pIEEEAddrRsp;

#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_IEEE_ADDR_RSP )
  {
    zdo_MTCB_NwkIEEEAddrRspCB( SPI_CB_ZDO_IEEE_ADDR_RSP, SrcAddr, Status,
                  IEEEAddr, 0, NumAssocDev, StartIndex, AssocDevList );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( ZDApp_IEEEAddrRsp_TaskID )
  {
    // Send the IEEE Address response structure to the registered task
    bufLen = sizeof( ZDO_IEEEAddrResp_t ) + sizeof( uint16 ) * NumAssocDev;

    pIEEEAddrRsp = (ZDO_IEEEAddrResp_t *)osal_msg_allocate( bufLen );
    if ( pIEEEAddrRsp )
    {
      pIEEEAddrRsp->hdr.event = ZDO_IEEE_ADDR_RESP;

      // Build the structure
      pIEEEAddrRsp->nwkAddr = aoi;
      osal_cpyExtAddr( pIEEEAddrRsp->extAddr, IEEEAddr );
      pIEEEAddrRsp->numAssocDevs = NumAssocDev;
      pIEEEAddrRsp->startIndex = StartIndex;
      osal_memcpy( pIEEEAddrRsp->devList, AssocDevList, (sizeof( uint16 ) * NumAssocDev) );

      osal_msg_send( ZDApp_IEEEAddrRsp_TaskID, (uint8 *)pIEEEAddrRsp );
    }
  }
}
#endif // ZDO_IEEEADDR_REQUEST

#if defined ( ZDO_NODEDESC_REQUEST )
/*********************************************************************
 * @fn      ZDApp_NodeDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Node_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  aoi - 16 bit network address of interest.
 * @param  pNodeDesc - pointer to the devices Node Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
void ZDApp_NodeDescRspCB( zAddrType_t *SrcAddr, byte Status, uint16 aoi,
                          NodeDescriptorFormat_t *pNodeDesc )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_NODE_DESC_RSP )
  {
    zdo_MTCB_NodeDescRspCB( SrcAddr, Status, aoi, pNodeDesc );
    return;
  }
#endif  //MT_ZDO_FUNC
}
#endif

#if defined ( ZDO_POWERDESC_REQUEST )
/*********************************************************************
 * @fn      ZDApp_PowerDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Power_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  aoi - 16 bit network address of interest.
 * @param  pPwrDesc - pointer to the devices Power Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
void ZDApp_PowerDescRspCB( zAddrType_t *SrcAddr, byte Status,
                            uint16 aoi, NodePowerDescriptorFormat_t *pPwrDesc )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_POWER_DESC_RSP )
  {
    zdo_MTCB_PowerDescRspCB( SrcAddr, Status, aoi, pPwrDesc );
    return;
  }
#endif  //MT_ZDO_FUNC
}
#endif // ZDO_POWERDESC_REQUEST

#if defined ( ZDO_SIMPLEDESC_REQUEST )
/*********************************************************************
 * @fn      ZDApp_SimpleDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Simple_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  aoi - 16 bit network address of interest.
 * @param  endPoint - Endpoint for description
 * @param  pSimpleDesc - pointer to the devices Simple Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
void ZDApp_SimpleDescRspCB( zAddrType_t *SrcAddr, byte Status,
                            uint16 aoi, byte endPoint,
                            SimpleDescriptionFormat_t *pSimpleDesc )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_SIMPLE_DESC_RSP )
  {
    zdo_MTCB_SimpleDescRspCB( SrcAddr, Status, aoi, endPoint, pSimpleDesc );
    return;
  }
#endif  //MT_ZDO_FUNC
}
#endif // ZDO_SIMPLEDESC_REQUEST

#if defined ( ZDO_ACTIVEEP_REQUEST )
/*********************************************************************
 * @fn      ZDApp_ActiveEPRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Active_EP_rsp message is received.
 *
 * @param  src    - Device's short address that this response describes
 * @param  Status - response status
 * @param  epCnt  - number of epList items
 * @param  epList - array of active endpoint.
 *
 * @return  none
 */
void ZDApp_ActiveEPRspCB( zAddrType_t *src, byte Status,
                                                     byte epCnt, byte *epList )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_ACTIVE_EPINT_RSP )
  {
    zdo_MTCB_MatchActiveEPRspCB( SPI_CB_ZDO_ACTIVE_EPINT_RSP, src,
                                  src->addr.shortAddr, Status, epCnt, epList );
    return;
  }
#endif  //MT_ZDO_FUNC
}
#endif // ZDO_ACTIVEEP_REQUEST

#if defined ( ZDO_MATCH_REQUEST )
/*********************************************************************
 * @fn      ZDApp_MatchDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Match_Desc_rsp message is received.
 *
 * NOTE:  Currently, this function accepts any responding device as THE
 *        match and updates the endpoint (requested) destination's
 *        address.  So, the last response received is the application's
 *        match.
 *
 *        This function could be changed to do further device discovery
 *        and/or accept multiple responses.
 *
 * @param  src     - Device's short address that this response describes
 * @param  Status  - response status
 * @param  epCnt   - number of epList items
 * @param  epList  - array of active endpoint
 *
 * @return  none
 */
void ZDApp_MatchDescRspCB( zAddrType_t *src, byte Status,
                                                     byte epCnt, byte *epList )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_MATCH_DESC_RSP )
  {
    zdo_MTCB_MatchActiveEPRspCB( SPI_CB_ZDO_MATCH_DESC_RSP, src,
                                  src->addr.shortAddr, Status, epCnt, epList );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( (Status != ZDP_SUCCESS) || (epCnt == 0) )
  {
    return;
  }

  if ( ZDApp_MatchDescRsp_TaskID )
  {
    // Send the IEEE Address response structure to the registered task.
    uint8 bufLen = sizeof( ZDO_MatchDescResp_t ) + epCnt;
    ZDO_MatchDescResp_t *pMatchDescRsp = (ZDO_MatchDescResp_t *)osal_msg_allocate( bufLen );

    if ( pMatchDescRsp )
    {
      pMatchDescRsp->hdr.event = ZDO_MATCH_DESC_RESP;

      // Build the structure.
      pMatchDescRsp->nwkAddr = src->addr.shortAddr;
      pMatchDescRsp->epCnt = epCnt;
      osal_memcpy( pMatchDescRsp->epList, epList, epCnt );

      osal_msg_send( ZDApp_MatchDescRsp_TaskID, (uint8 *)pMatchDescRsp );
    }
  }

  if ( ZDApp_AutoFindMode_epDesc )
  {
    ZDApp_SendNewDstAddr( *epList, src, 0, false,
                          *(ZDApp_AutoFindMode_epDesc->task_id),
                            ZDApp_AutoFindMode_epDesc->endPoint );

    HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
  }
}
#endif // ZDO_MATCH_REQUEST

#if defined ( ZDO_ENDDEVICEBIND_REQUEST )
/*********************************************************************
 * @fn      ZDApp_EndDeviceBindRsp()
 *
 * @brief
 *
 *   Called by ZDO when a End_Device_Bind_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void ZDApp_EndDeviceBindRsp( zAddrType_t *SrcAddr, byte Status )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_END_DEVICE_BIND_RSP )
  {
    zdo_MTCB_BindRspCB( SPI_CB_ZDO_END_DEVICE_BIND_RSP, SrcAddr, Status );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( Status == ZDP_SUCCESS )
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_ON );
#if defined(BLINK_LEDS)
  else
    // Flash LED to show failure
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
#endif
}
#endif // ZDO_ENDDEVICEBIND_REQUEST

#if defined ( ZDO_BIND_UNBIND_REQUEST )
/*********************************************************************
 * @fn      ZDApp_BindRsp()
 *
 * @brief
 *
 *   Called by ZDO when a Bind_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void ZDApp_BindRsp( zAddrType_t *SrcAddr, byte Status )
{
  ZDO_BindRsp_t bindRsp;

#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_BIND_RSP )
  {
    zdo_MTCB_BindRspCB( SPI_CB_ZDO_BIND_RSP, SrcAddr, Status );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( ZDApp_BindUnbindRsp_TaskID != TASK_NO_TASK )
  {
    // Send the response structure to the registered task
    bindRsp.nwkAddr = SrcAddr->addr.shortAddr;
    bindRsp.status  = Status;

    ZDApp_SendMsg( ZDApp_BindUnbindRsp_TaskID,
                   ZDO_BIND_RESP,
                   sizeof(ZDO_BindRsp_t),
                   (byte*)(&bindRsp) );
  }
}
#endif // ZDO_BIND_UNBIND_REQUEST

#if defined ( ZDO_BIND_UNBIND_REQUEST )
/*********************************************************************
 * @fn      ZDApp_UnbindRsp()
 *
 * @brief
 *
 *   Called by ZDO when a Unbind_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
void ZDApp_UnbindRsp( zAddrType_t *SrcAddr, byte Status )
{
  ZDO_UnbindRsp_t unbindRsp;

#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_UNBIND_RSP )
  {
    zdo_MTCB_BindRspCB( SPI_CB_ZDO_UNBIND_RSP, SrcAddr, Status );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( ZDApp_BindUnbindRsp_TaskID != TASK_NO_TASK )
  {
    // Send the response structure to the registered task
    unbindRsp.nwkAddr = SrcAddr->addr.shortAddr;
    unbindRsp.status  = Status;

    ZDApp_SendMsg( ZDApp_BindUnbindRsp_TaskID,
                   ZDO_UNBIND_RESP,
                   sizeof(ZDO_UnbindRsp_t),
                   (byte*)(&unbindRsp) );
  }
}
#endif // ZDO_BIND_UNBIND_REQUEST

/*********************************************************************
 * Call Back Functions from NWK  - API
 */

/*********************************************************************
 * @fn          ZDO_NetworkDiscoveryConfirmCB
 *
 * @brief       This function returns a choice of PAN to join.
 *
 * @param       ResultCount - Number of routers discovered
 * @param               NetworkList - Pointer to list of network descriptors
 *
 * @return      ZStatus_t
 */
#define STACK_PROFILE_MAX 2
ZStatus_t ZDO_NetworkDiscoveryConfirmCB( byte ResultCount,
                                         networkDesc_t *NetworkList )
{
  networkDesc_t *pNwkDesc;
  ZDO_NetworkDiscoveryCfm_t msg;
  byte  i, j;
  uint8 stackProfile;
  uint8 stackProfilePro;
  uint8 selected;

#if defined ( ZDO_MGMT_NWKDISC_RESPONSE )
  if ( zdappMgmtNwkDiscReqInProgress )
  {
    zdappMgmtNwkDiscReqInProgress = false;
    ZDO_FinishProcessingMgmtNwkDiscReq( ResultCount, NetworkList );
    return ( ZSuccess );
  }
#endif

  // process discovery results
  stackProfilePro = FALSE;
  selected = FALSE;

  for ( stackProfile = 0; stackProfile < STACK_PROFILE_MAX; stackProfile++ )
  {
    for ( j = 0; j < (sizeof(sPVerList)/sizeof(sPVerList[0])); ++j )
    {
      pNwkDesc = NetworkList;
      for ( i = 0; i < ResultCount; i++, pNwkDesc = pNwkDesc->nextDesc )
      {
        if ( zgConfigPANID != 0xFFFF )
        {
          // PAN Id is preconfigured. check if it matches
          // only 14 bits of pan id is used
          if ( pNwkDesc->panId != ( zgConfigPANID & 0x3FFF ) )
            continue;
          #if !defined ( DEF_PROTO_VERS )
          // If the macro was not defined ensure we join the version supported by
          // this PAN by forcing a match below. We need this statement because we
          // want to (possibly) override the NV value if the PAN was pre-defined.
          // See App Note 026.
          sPVerList[j] = pNwkDesc->version;
          #endif
        }

        // check that network is allowing joining
        //------------------------------------------------------------
        #if defined( RTR_NWK )
        //------------------------------------------------------------
        if ( stackProfilePro == FALSE )
        {
          if ( !pNwkDesc->routerCapacity )
          {
            continue;
          }
        }
        else
        {
          if ( !pNwkDesc->deviceCapacity )
          {
            continue;
          }
        }
        //------------------------------------------------------------
        #else
        //------------------------------------------------------------
        if ( !pNwkDesc->deviceCapacity )
        {
          continue;
        }
        //------------------------------------------------------------
        #endif
        //------------------------------------------------------------

        // check version of zigbee protocol
        if ( pNwkDesc->version != sPVerList[j] )
          continue;

        // check version of stack profile
        if ( pNwkDesc->stackProfile != zgStackProfile )
        {
          stackProfilePro = TRUE;

          if ( stackProfile == 0 )
          {
            continue;
          }
        }

        // check if beacon order is the right value..
    //  if ( pNwkDesc->beaconOrder < ZDO_CONFIG_MAX_BO )
    //    continue;

        // choose this pan for joining
        break;
      }
      if (i < ResultCount)
      {
        selected = TRUE;
        break;
      }
    }

    // break if selected or stack profile pro wasn't found
    if ( (selected == TRUE) || (stackProfilePro == FALSE) )
    {
      break;
    }
  }

  if ( i == ResultCount )
  {
    msg.hdr.status = ZDO_FAIL;   // couldn't find appropriate PAN to join !
  }
  else
  {
    // (possibly) reset NV network version we're running under.
    if (NLME_GetProtocolVersion() != sPVerList[j])
    {
      NLME_SetRequest(nwkProtocolVersion, 0, &sPVerList[j]);
      // make sure we update NV
      osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 1000 );
    }

    msg.hdr.status = ZDO_SUCCESS;
    msg.panIdLSB = LO_UINT16( pNwkDesc->panId );
    msg.panIdMSB = HI_UINT16( pNwkDesc->panId );
    msg.logicalChannel = pNwkDesc->logicalChannel;
    msg.version = pNwkDesc->version;
    osal_cpyExtAddr( msg.extendedPANID, pNwkDesc->extendedPANID );
  }

  ZDApp_SendMsg( ZDAppTaskID, ZDO_NWK_DISC_CNF, sizeof(ZDO_NetworkDiscoveryCfm_t), (byte *)&msg );

  return (ZSuccess);
}  // ZDO_NetworkDiscoveryConfirmCB

/*********************************************************************
 * @fn          ZDO_NetworkFormationConfirmCB
 *
 * @brief       This function reports the results of the request to
 *              initialize a coordinator in a network.
 *
 * @param       Status - Result of NLME_NetworkFormationRequest()
 *
 * @return      none
 */
void ZDO_NetworkFormationConfirmCB( ZStatus_t Status )
{
#if defined(ZDO_COORDINATOR)
  nwkStatus = (byte)Status;

  if ( Status == ZSUCCESS )
  {
    // LED on shows Coordinator started
    HalLedSet ( HAL_LED_3, HAL_LED_MODE_ON );

    // LED off forgets HOLD_AUTO_START
    HalLedSet (HAL_LED_4, HAL_LED_MODE_OFF);

#if defined ( ZBIT )
    SIM_SetColor(0xd0ffd0);
#endif

    if ( devState == DEV_HOLD )
    {
      // Began with HOLD_AUTO_START
      devState = DEV_COORD_STARTING;
    }
  }
#if defined(BLINK_LEDS)
  else
    HalLedSet ( HAL_LED_3, HAL_LED_MODE_FLASH );  // Flash LED to show failure
#endif

  osal_set_event( ZDAppTaskID, ZDO_NETWORK_START );
#endif  //ZDO_COORDINATOR
}

#if defined(RTR_NWK)
/*********************************************************************
 * @fn          ZDO_StartRouterConfirmCB
 *
 * @brief       This function reports the results of the request to
 *              start functioning as a router in a network.
 *
 * @param       Status - Result of NLME_StartRouterRequest()
 *
 * @return      none
 */
void ZDO_StartRouterConfirmCB( ZStatus_t Status )
{
  nwkStatus = (byte)Status;

  if ( Status == ZSUCCESS )
  {
    // LED on shows Router started
    HalLedSet ( HAL_LED_3, HAL_LED_MODE_ON );
    // LED off forgets HOLD_AUTO_START
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF);
    if ( devState == DEV_HOLD )
    {
      // Began with HOLD_AUTO_START
      devState = DEV_END_DEVICE;
    }
  }
#if defined(BLINK_LEDS)
  else
    HalLedSet( HAL_LED_3, HAL_LED_MODE_FLASH );  // Flash LED to show failure
#endif

  osal_set_event( ZDAppTaskID, ZDO_ROUTER_START );
}
#endif  //RTR_NWK

/*********************************************************************
 * @fn          ZDO_JoinConfirmCB
 *
 * @brief       This function allows the next hight layer to be notified
 *              of the results of its request to join itself or another
 *              device to a network.
 *
 * @param       Status - Result of NLME_JoinRequest()
 *
 * @return      none
 */
void ZDO_JoinConfirmCB( uint16 PanId, ZStatus_t Status )
{
  nwkStatus = (byte)Status;

  if ( Status == ZSUCCESS )
  {
    // LED on shows device joined
    HalLedSet ( HAL_LED_3, HAL_LED_MODE_ON );
    // LED off forgets HOLD_AUTO_START
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF);
    if ( (devState == DEV_HOLD) )
    {
      // Began with HOLD_AUTO_START
      devState = DEV_NWK_JOINING;
    }
#if !  ( SECURE != 0  )
    // Notify to save info into NV
    osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 100 );
#endif
  }
#if defined(BLINK_LEDS)
  else
    HalLedSet ( HAL_LED_3, HAL_LED_MODE_FLASH );  // Flash LED to show failure
#endif

  // Notify ZDApp
  ZDApp_SendMsg( ZDAppTaskID, ZDO_NWK_JOIN_IND, sizeof(osal_event_hdr_t), (byte*)NULL );
}

/*********************************************************************
 * @fn          ZDO_JoinIndicationCB
 *
 * @brief       This function allows the next higher layer of a
 *              coordinator to be notified of a remote join request.
 *
 * @param       ShortAddress - 16-bit address
 * @param       ExtendedAddress - IEEE (64-bit) address
 * @param       CapabilityInformation - Association Capability Information
 *
 * @return      ZStatus_t
 */
#if defined(RTR_NWK)
ZStatus_t ZDO_JoinIndicationCB( uint16 ShortAddress, byte *ExtendedAddress,
                                         byte CapabilityInformation )
{
#if defined (AUTO_SOFT_START)
    ZDX_PostCoordinatorIEEE(ShortAddress);
#endif

  // Notify to save info into NV
  osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 1000 );

#if   ( SECURE != 0  )
  // send notification to TC of new device..
  if ( _NIB.SecurityLevel )
    osal_start_timerEx( ZDAppTaskID, ZDO_NEW_DEVICE, 600 );
#endif  // SECURE

  return ( ZSuccess );
}
#endif  //RTR_NWK

/*********************************************************************
 * @fn          ZDO_LeaveCnf
 *
 * @brief       This function allows the next higher layer to be
 *              notified of the results of its request for this or
 *              a child device to leave the network.
 *
 * @param       cnf - NLME_LeaveCnf_t
 *
 * @return      none
 */
void ZDO_LeaveCnf( NLME_LeaveCnf_t* cnf )
{
  // Check for this device
  if ( osal_ExtAddrEqual( cnf->extAddr,
                          NLME_GetExtAddr() ) == TRUE )
  {
    // Prepare to leave with reset
    ZDApp_LeaveReset( cnf->rejoin );
  }
  //------------------------------------------------------------------
  #if defined( RTR_NWK )
  //------------------------------------------------------------------
  else
  {
    // Remove device address(optionally descendents) from data
    ZDApp_LeaveUpdate( cnf->dstAddr,
                       cnf->extAddr,
                       cnf->removeChildren );
  }
  //------------------------------------------------------------------
  #endif
  //------------------------------------------------------------------
}

/*********************************************************************
 * @fn          ZDO_LeaveInd
 *
 * @brief       This function allows the next higher layer of a
 *              device to be notified of a remote leave request or
 *              indication.
 *
 * @param       ind - NLME_LeaveInd_t
 *
 * @return      none
 */
void ZDO_LeaveInd( NLME_LeaveInd_t* ind )
{
  uint8 leave;


  // Parent is requesting the leave - NWK layer filters out illegal
  // requests
  if ( ind->request == TRUE )
  {
    // Notify network of leave
    //----------------------------------------------------------------
    #if defined( RTR_NWK )
    //----------------------------------------------------------------
    NLME_LeaveRsp_t rsp;
    rsp.rejoin         = ind->rejoin;
    rsp.removeChildren = ind->removeChildren;
    NLME_LeaveRsp( &rsp );
    //----------------------------------------------------------------
    #endif
    //----------------------------------------------------------------

    // Prepare to leave with reset
    ZDApp_LeaveReset( ind->rejoin );
  }
  else
  {
    leave = FALSE;

    // Check if this device needs to leave as a child or descendent
    if ( ind->srcAddr == NLME_GetCoordShortAddr() )
    {
      if ( ( ind->removeChildren == TRUE               ) ||
           ( ZDO_Config_Node_Descriptor.LogicalType ==
             NODETYPE_DEVICE                           )    )
      {
        leave = TRUE;
      }
    }
    else if ( ind->removeChildren == TRUE )
    {
      // Check NWK address allocation algorithm
      //leave = RTG_ANCESTOR(nwkAddr,thisAddr);
    }

    if ( leave == TRUE )
    {
      // Prepare to leave with reset
      ZDApp_LeaveReset( ind->rejoin );
    }
    else
    {
      // Remove device address(optionally descendents) from data
      ZDApp_LeaveUpdate( ind->srcAddr,
                         ind->extAddr,
                         ind->removeChildren );
    }
  }
}

/*********************************************************************
 * @fn          ZDO_SyncIndicationCB
 *
 * @brief       This function allows the next higher layer of a
 *              coordinator to be notified of a loss of synchronization
 *                          with the parent/child device.
 *
 * @param       type: 0 - child; 1 - parent
 *
 *
 * @return      none
 */
void ZDO_SyncIndicationCB( byte type, uint16 shortAddr )
{

#if !defined ( RTR_NWK )
    if ( type == 1 )
    {
      ZDApp_SendMsg( ZDAppTaskID, ZDO_NWK_JOIN_REQ, sizeof(osal_event_hdr_t), NULL );
    }
#endif
  return;
}

/*********************************************************************
 * @fn          ZDO_PollConfirmCB
 *
 * @brief       This function allows the next higher layer to be
 *              notified of a Poll Confirm.
 *
 * @param       none
 *
 * @return      none
 */
void ZDO_PollConfirmCB( byte status )
{
  return;
}

/******************************************************************************
 * @fn          ZDApp_NwkWriteNVRequest (stubs AddrMgrWriteNVRequest)
 *
 * @brief       Stub routine implemented by NHLE. NHLE should call
 *              <AddrMgrWriteNV> when appropriate.
 *
 * @param       none
 *
 * @return      none
 */
void ZDApp_NwkWriteNVRequest( void )
{
  if ( !osal_get_timeoutEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV ) )
  {
    // Trigger to save info into NV
    osal_start_timerEx( ZDAppTaskID,
                        ZDO_NWK_UPDATE_NV,
                        ZDAPP_UPDATE_NWK_NV_TIME );
  }
}

/*********************************************************************
 * Call Back Functions from Security  - API
 */

#if defined( ZDO_COORDINATOR )
 /*********************************************************************
 * @fn          ZDO_UpdateDeviceIndication
 *
 * @brief       This function notifies the "Trust Center" of a
 *              network when a device joins or leaves the network.
 *
 * @param       extAddr - pointer to 64 bit address of new device
 * @param       status  - 0 if a new device joined securely
 *                      - 1 if a new device joined un-securely
 *                      - 2 if a device left the network
 *
 * @return      true if newly joined device should be allowed to
 *                                              remain on network
 */
ZStatus_t ZDO_UpdateDeviceIndication( byte *extAddr, byte status )
{

#if   ( SECURE != 0  )
  // can implement a network access policy based on the
  // IEEE address of newly joining devices...
#endif  // Trust Center

  return ZSuccess;
}
#endif // ZDO

/*********************************************************************
 * @fn          ZDApp_InMsgCB
 *
 * @brief       This function is called to pass up any message that is
 *              not yet supported.  This allows for the developer to
 *              support features themselves..
 *
 * @return      none
 */
void ZDApp_InMsgCB( byte TransSeq, zAddrType_t *srcAddr, byte wasBroadcast,
                  cId_t clusterID, byte asduLen, byte *asdu, byte SecurityUse )
{
  if ( clusterID & ZDO_RESPONSE_BIT )
  {
    // Handle the response message
  }
  else
  {
    // Handle the request message by sending a generic "not supported".
    // End Device Announce doesn't have a response.
    if ( !wasBroadcast && clusterID != End_Device_annce )
    {
      ZDP_GenericRsp( TransSeq, srcAddr, ZDP_NOT_SUPPORTED, 0,
                      (uint16)(clusterID | ZDO_RESPONSE_BIT), SecurityUse );
    }
  }
}

#if defined ( ZDO_MGMT_LQI_REQUEST )
/*********************************************************************
 * @fn          ZDApp_MgmtLqiRspCB
 *
 * @brief       This function handles Management LQI response for the
 *              Device Object application.
 *
 * @param       Status - ZSuccess or other for failure
 * @param       NeighborLqiEntries - number of possible entries on
 *                       the device
 * @param       StartIndex - where this list start in possible entries
 * @param       NeighborLqiCount - number of entries in this list
 * @param       pList - pointer to the list of LQI items.
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_MgmtLqiRspCB( uint16 SrcAddr, byte Status, byte NeighborLqiEntries,
                         byte StartIndex, byte NeighborLqiCount,
                         neighborLqiItem_t *pList )
{
  byte x;

#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_MGMT_LQI_RSP )
  {
    zdo_MTCB_MgmtLqiRspCB( SrcAddr, Status, NeighborLqiEntries, StartIndex,
                                        NeighborLqiCount, pList );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( Status == ZSuccess )
  {
    for ( x = 0; x < NeighborLqiCount; x++, pList++ )
    {
      // Do something with the results
    }
  }
  else
  {
    // Error
  }
}
#endif // ZDO_MGMT_LQI_REQUEST

#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
/*********************************************************************
 * @fn          ZDApp_MgmtNwkDiscRspCB
 *
 * @brief       This function handles Management Network Discovery
 *              response for the Device Object application.
 *
 * @param       SrcAddr - source of the message
 * @param       Status - ZSuccess or other for failure
 * @param       NetworkCount - number of possible entries on
 *                       the device
 * @param       StartIndex - where this list start in possible entries
 * @param       networkListCount - number of entries in this list
 * @param       pList - pointer to the list of Network Discover items.
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_MgmtNwkDiscRspCB( uint16 SrcAddr, byte Status, byte NetworkCount,
                             byte StartIndex, byte networkListCount,
                             mgmtNwkDiscItem_t *pList )
{
  byte x;

#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_MGMT_NWKDISC_RSP )
  {
    zdo_MTCB_MgmtNwkDiscRspCB( SrcAddr, Status, NetworkCount, StartIndex,
                                        networkListCount, pList );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( Status == ZSuccess )
  {
    for ( x = 0; x < networkListCount; x++, pList++ )
    {
      // Do something with the results
    }
  }
  else
  {
    // Error
  }
}
#endif // ZDO_MGMT_NWKDISC_REQUEST

#if defined ( ZDO_MGMT_RTG_REQUEST )
/*********************************************************************
 * @fn          ZDApp_MgmtRtgRspCB
 *
 * @brief       This function handles Management Routing response for the
 *              Device Object application.
 *
 * @param       SrcAddr - source of the message
 * @param       Status - ZSuccess or other for failure
 * @param       rtgCount - number of possible entries on
 *                       the device
 * @param       StartIndex - where this list start in possible entries
 * @param       rtgCount - number of entries in this list
 * @param       pList - pointer to the list of Network Discover items.
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_MgmtRtgRspCB( uint16 SrcAddr, byte Status, byte rtgCount,
                         byte StartIndex, byte rtgListCount,
                         rtgItem_t *pList )
{
  byte x;

#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_MGMT_RTG_RSP )
  {
    zdo_MTCB_MgmtRtgRspCB( SrcAddr, Status, rtgCount, StartIndex,
                                                        rtgListCount, pList );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( Status == ZSuccess )
  {
    for ( x = 0; x < rtgListCount; x++, pList++ )
    {
      // Do something with the results
    }
  }
  else
  {
    // Error
  }
}
#endif // ZDO_MGMT_RTG_REQUEST

#if defined ( ZDO_MGMT_BIND_REQUEST )
/*********************************************************************
 * @fn          ZDApp_MgmtBindRspCB
 *
 * @brief       This function handles Management Binding response for the
 *              Device Object application.
 *
 * @param       SrcAddr - source of the message
 * @param       Status - ZSuccess or other for failure
 * @param       BindingCount - number of possible entries on
 *                       the device
 * @param       StartIndex - where this list start in possible entries
 * @param       BindingListCount - number of entries in this list
 * @param       pList - pointer to the list of Network Discover items.
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_MgmtBindRspCB( uint16 SrcAddr, byte Status, byte BindingCount,
                         byte StartIndex, byte BindingListCount,
                         apsBindingItem_t *pList )
{
  byte x;

#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_MGMT_BIND_RSP )
  {
    zdo_MTCB_MgmtBindRspCB( SrcAddr, Status, BindingCount, StartIndex,
                                                  BindingListCount, pList );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( Status == ZSuccess )
  {
    for ( x = 0; x < BindingListCount; x++, pList++ )
    {
      // Do something with the results
    }
  }
  else
  {
    // Error
  }
}
#endif // ZDO_MGMT_BIND_REQUEST

#if defined ( ZDO_MGMT_JOINDIRECT_REQUEST )
/*********************************************************************
 * @fn          ZDApp_MgmtDirectJoinRspCB
 *
 * @brief       This function handles Management Direct Join response for the
 *              Device Object application.
 *
 * @param       SrcAddr - source of the message
 * @param       Status - ZSuccess or other for failure
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_MgmtDirectJoinRspCB( uint16 SrcAddr, byte Status, byte SecurityUse )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_MGMT_DIRECT_JOIN_RSP )
  {
    zdo_MTCB_MgmtDirectJoinRspCB( SrcAddr, Status, SecurityUse );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( Status == ZSuccess )
  {
    // Do something with the results
  }
  else
  {
    // Error
  }
}
#endif // ZDO_MGMT_JOINDIRECT_REQUEST

#if defined ( ZDO_MGMT_LEAVE_REQUEST )
/*********************************************************************
 * @fn          ZDApp_MgmtLeaveRspCB
 *
 * @brief       This function handles Management Leave response for the
 *              Device Object application.
 *
 * @param       SrcAddr - source of the message
 * @param       Status - ZSuccess or other for failure
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_MgmtLeaveRspCB( uint16 SrcAddr, byte Status, byte SecurityUse )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_MGMT_LEAVE_RSP )
  {
    zdo_MTCB_MgmtLeaveRspCB( SrcAddr, Status, SecurityUse );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( Status == ZSuccess )
  {
    // Do something with the results
  }
  else
  {
    // Error
  }
}
#endif // ZDO_MGMT_LEAVE_REQUEST

#if defined ( ZDO_MGMT_BIND_RESPONSE ) && !defined( REFLECTOR )
/*********************************************************************
 * @fn          ZDApp_MgmtBindReqCB
 *
 * @brief       This function finishes the processing of the Management
 *              Bind Request and generates the response.
 *
 * @param       none
 *
 * @return      none
 */
void ZDApp_MgmtBindReqCB( byte TransSeq, zAddrType_t *SrcAddr, byte StartIndex, byte SecurityUse )
{
  ZDO_MgmtBindReq_t *pBindReq;
  osal_event_hdr_t *msgPtr;

  if ( ZDApp_MgmtBindReq_TaskID )
  {
    // Send the IEEE Address response structure to the registered task
    msgPtr = (osal_event_hdr_t *)osal_msg_allocate( sizeof(osal_event_hdr_t) + sizeof( ZDO_MgmtBindReq_t ) );
    if ( msgPtr )
    {
      msgPtr->event = ZDO_MGMT_BIND_REQ;
      pBindReq = (ZDO_MgmtBindReq_t *)(msgPtr + 1);

      // Build the structure
      pBindReq->hdr.srcAddr = SrcAddr->addr.shortAddr;
      pBindReq->hdr.transSeq = TransSeq;
      pBindReq->hdr.SecurityUse = SecurityUse;

      pBindReq->startIndex = StartIndex;
      osal_msg_send( ZDApp_MgmtBindReq_TaskID, (uint8 *)msgPtr );
    }
  }
}
#endif // ZDO_MGMT_BIND_RESPONSE && !REFLECTOR

#if defined ( ZDO_MGMT_PERMIT_JOIN_REQUEST )

/*********************************************************************
 * @fn          ZDApp_MgmtPermitJoinRspCB
 *
 * @brief       This function handles Management permit join response
 *              for the Device Object application.
 *
 * @param       SrcAddr - source of the message
 * @param       Status - ZSuccess or other for failure
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_MgmtPermitJoinRspCB( uint16 SrcAddr, byte Status,
                                byte SecurityUse )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_MGMT_PERMIT_JOIN_RSP )
  {
    zdo_MTCB_MgmtPermitJoinRspCB( SrcAddr, Status, SecurityUse );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( Status == ZSuccess )
  {
    // Do something with the results
  }
  else
  {
    // Error
  }
}
#endif // ZDO_MGMT_LEAVE_REQUEST

#if defined ( ZDO_USERDESC_REQUEST )
/*********************************************************************
 * @fn          ZDApp_UserDescRspCB
 *
 * @brief       This function handles User Descriptor response for the
 *              Device Object application.
 *
 * @param       SrcAddr - source of the message
 * @param       Status - ZSuccess or other for failure
 * @param       nwkAddrOfInterest - network address of remote device
 * @param       userDescLen - length of user descriptor
 * @param       userDesc - user descriptor byte string
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_UserDescRspCB( uint16 SrcAddr, byte status, uint16 nwkAddrOfInterest,
                          byte userDescLen, byte *userDesc, byte SecurityUse )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_USER_DESC_RSP )
  {
    zdo_MTCB_UserDescRspCB( SrcAddr, status, nwkAddrOfInterest,
                            userDescLen, userDesc, SecurityUse );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( status == ZSuccess )
  {
    // Do something with the results
  }
  else
  {
    // Error
  }
}
#endif

#if defined ( ZDO_USERDESCSET_REQUEST )
/*********************************************************************
 * @fn          ZDApp_UserDescConfCB
 *
 * @brief       This function handles Management Direct Join response for the
 *              Device Object application.
 *
 * @param       SrcAddr - source of the message
 * @param       Status - ZSuccess or other for failure
 * @param       SecurityUse -
 *
 * @return      none
 */
void ZDApp_UserDescConfCB( uint16 SrcAddr, byte status, byte SecurityUse )
{
#if defined ( MT_ZDO_FUNC )
  /* First check if MT has subscribed for this callback. If so , pass it as
  a event to MonitorTest and return control to calling function after that */
  if ( _zdoCallbackSub & CB_ID_ZDO_USER_DESC_CONF )
  {
    zdo_MTCB_UserDescConfCB( SrcAddr, status, SecurityUse );
    return;
  }
#endif  //MT_ZDO_FUNC

  if ( status == ZSuccess )
  {
    // Do something with the results
  }
  else
  {
    // Error
  }
}
#endif

#if defined ( ZDO_SERVERDISC_REQUEST )
/*********************************************************************
 * @fn          ZDApp_ServerDiscRspCB
 *
 * @brief       Handle the Server_Discovery_rsp response.
 *
 * @param       srcAddr     - Source Address of the message.
 * @param       status      - ZSuccess.
 * @param       serverMask  - Bit mask of services matching the req serverMask.
 * @param       securityUse -
 *
 * @return      none
 */
void ZDApp_ServerDiscRspCB( uint16 srcAddr, byte status,
                            uint16 serverMask, byte securityUse )
{
#if defined ( MT_ZDO_FUNC )
  if ( _zdoCallbackSub & CB_ID_ZDO_SERVERDISC_RSP )
  {
    zdo_MTCB_ServerDiscRspCB( srcAddr, status, serverMask, securityUse );
    return;
  }
#endif

  if ( status == ZSuccess )
  {
    // Do something with the results.
  }
  else
  {
    // Error
  }
}
#endif

/*********************************************************************
 * @fn      ZDApp_EndDeviceAnnounceCB()
 *
 * @brief   Received an End Device Announce.
 *
 * @param   SrcAddr - Source of the message
 * @param   nwkAddr - short address of the new device
 * @param   extAddr - IEEE address of the new device
 * @param   capabilities - device capabilities.  This field is only
 *             populated in a v1.1 network so use the following before
 *             using it:
 *                  if ( NLME_GetProtocolVersion() != ZB_PROT_V1_0 )
 *
 * @return  none
 */
void ZDApp_EndDeviceAnnounceCB( uint16 SrcAddr, uint16 nwkAddr, uint8 *extAddr,
                               uint8 capabilities )
{
  ZDO_EndDeviceAnnounce_t Announce;

  // If it interests you - put your own code here.

  if ( ZDApp_EndDeviceAnnounce_TaskID )
  {
    // Build the structure
    Announce.srcAddr = SrcAddr;
    Announce.nwkAddr = nwkAddr;
    osal_cpyExtAddr( Announce.extAddr, extAddr );
    Announce.capabilities = capabilities;

    ZDApp_SendMsg( ZDApp_EndDeviceAnnounce_TaskID, ZDO_END_DEVICE_ANNOUNCE,
                  sizeof( ZDO_EndDeviceAnnounce_t ), (uint8 *)&Announce );
  }
}

/*********************************************************************
 * @fn      ZDApp_ChangeMatchDescRespPermission()
 *
 * @brief   Changes the Match Descriptor Response permission.
 *
 * @param   endpoint - endpoint to allow responses
 * @param   action - true to allow responses, false to not
 *
 * @return  none
 */
void ZDApp_ChangeMatchDescRespPermission( uint8 endpoint, uint8 action )
{
  // Store the action
  afSetMatch( endpoint, action );
}

/*********************************************************************
 * @fn      ZDApp_NetworkInit()
 *
 * @brief   Used to start the network joining process
 *
 * @param   delay - mSec delay to wait before starting
 *
 * @return  none
 */
void ZDApp_NetworkInit( uint16 delay )
{
  if ( delay )
  {
    // Wait awhile before starting the device
    osal_start_timerEx( ZDAppTaskID, ZDO_NETWORK_INIT, delay );
  }
  else
  {
    osal_set_event( ZDAppTaskID, ZDO_NETWORK_INIT );
  }
}

#if defined ( ZDO_IEEEADDR_REQUEST )
/*********************************************************************
 * @fn      ZDApp_RegisterForIEEEAddrRsp()
 *
 * @brief   Register to receive IEEE Addr Response messages
 *
 * @param   TaskID - ID of task to send message
 *
 * @return  none
 */
void ZDApp_RegisterForIEEEAddrRsp( byte TaskID )
{
  ZDApp_IEEEAddrRsp_TaskID = TaskID;    // Only 1 task at a time
}
#endif // defined ( ZDO_IEEEADDR_REQUEST )

#if defined ( ZDO_NWKADDR_REQUEST )
/*********************************************************************
 * @fn      ZDApp_RegisterForNwkAddrRsp()
 *
 * @brief   Register to receive NWK Addr Response messages
 *
 * @param   TaskID - ID of task to send message
 *
 * @return  none
 */
void ZDApp_RegisterForNwkAddrRsp( byte TaskID )
{
  ZDApp_NwkAddrRsp_TaskID = TaskID;    // Only 1 task at a time
}
#endif // defined ( ZDO_NWKADDR_REQUEST )

/*********************************************************************
 * @fn      ZDApp_RegisterForMatchDescRsp()
 *
 * @brief   Register to receive Match Descriptor Response messages
 *
 * @param   TaskID - ID of task to send message
 *
 * @return  none
 */
void ZDApp_RegisterForMatchDescRsp( byte TaskID )
{
  ZDApp_MatchDescRsp_TaskID = TaskID;    // Only 1 task at a time
}

/*********************************************************************
 * @fn      ZDApp_RegisterForEndDeviceAnnounce()
 *
 * @brief   Register to receive End Device Announce messages
 *
 * @param   TaskID - ID of task to send message
 *
 * @return  none
 */
void ZDApp_RegisterForEndDeviceAnnounce( byte TaskID )
{
  ZDApp_EndDeviceAnnounce_TaskID = TaskID;    // Only 1 task at a time
}

#if defined ( ZDO_BIND_UNBIND_REQUEST )
/*********************************************************************
 * @fn      ZDApp_RegisterForBindRsp()
 *
 * @brief   Register to receive Bind_rsp and Unbind_rsp messages
 *
 * @param   TaskID - ID of task to send message
 *
 * @return  none
 */
void ZDApp_RegisterForBindRsp( byte TaskID )
{
  ZDApp_BindUnbindRsp_TaskID = TaskID;    // Only 1 task at a time
}
#endif // ZDO_BIND_UNBIND_REQUEST

#if defined ( ZDO_BIND_UNBIND_RESPONSE ) && !defined ( REFLECTOR )
/*********************************************************************
 * @fn      ZDApp_RegisterForBindReq()
 *
 * @brief   Register to receive Bind and Unbind Request messages
 *
 * @param   TaskID - ID of task to send message
 *
 * @return  none
 */
void ZDApp_RegisterForBindReq( byte TaskID )
{
  ZDApp_BindReq_TaskID = TaskID;
}
#endif

#if defined ( ZDO_MGMT_BIND_RESPONSE ) && !defined ( REFLECTOR )
/*********************************************************************
 * @fn      ZDApp_RegisterForMgmtBindReq()
 *
 * @brief   Register to receive Mgmt Bind Request messages
 *
 * @param   TaskID - ID of task to send message
 *
 * @return  none
 */
void ZDApp_RegisterForMgmtBindReq( byte TaskID )
{
  ZDApp_MgmtBindReq_TaskID = TaskID;
}
#endif

/*********************************************************************
 * @fn      ZDApp_StartUpFromApp()
 *
 * @brief   Start the device.  This function will only start a device
 *          that is in the Holding state.
 *
 * @param   mode - ZDAPP_STARTUP_COORD - Start up as coordinator only
 *                 ZDAPP_STARTUP_ROUTER - Start up as router only
 *                 ZDAPP_STARTUP_AUTO - Startup in auto, look for coord,
 *                                       if not found, become coord.
 *
 * @return  TRUE if started, FALSE if in the wrong mode
 */
ZStatus_t ZDApp_StartUpFromApp( uint8 mode )
{
  ZStatus_t ret = ZFailure;

  if ( devState == DEV_HOLD )
  {
    // Start the device's joining process
    if ( ZDOInitDevice( 0 ) == ZDO_INITDEV_NEW_NETWORK_STATE )
    {
#if defined( SOFT_START )
      if ( mode == ZDAPP_STARTUP_COORD )
      {
        devStartMode = MODE_HARD;     // Start as a coordinator
        ZDO_Config_Node_Descriptor.LogicalType = NODETYPE_COORDINATOR;
      }
      else
      {
        if ( mode == ZDAPP_STARTUP_ROUTER )
        {
          softStartAllowCoord = FALSE;  // Don't allow coord to start
          continueJoining = TRUE;
        }
        devStartMode = MODE_JOIN;     // Assume joining
      }
#endif  // SOFT_START
    }
    ret = ZSuccess;
  }

  return ( ret );
}

/*********************************************************************
 * @fn      ZDApp_StopStartUp()
 *
 * @brief   Stops the joining process of a router.  This will only
 *          work if the router is in the scanning process and
 *          the SOFT_START feature is enabled.
 *
 * @param   none
 *
 * @return  TRUE if SOFT_START is enabled, FALSE if not possible
 */
uint8 ZDApp_StopStartUp( void )
{
  uint8 ret = FALSE;

#if defined( SOFT_START )
  continueJoining = FALSE;
  ret = TRUE;
#endif  // SOFT_START

  return ( ret );
}

/*********************************************************************
 * @fn      ZDApp_StartJoiningCycle()
 *
 * @brief   Starts the joining cycle of a device.  This will only
 *          continue an already started (or stopped) joining cycle.
 *
 * @param   none
 *
 * @return  TRUE if joining stopped, FALSE if joining or rejoining
 */
uint8 ZDApp_StartJoiningCycle( void )
{
  if ( devState == DEV_INIT || devState == DEV_NWK_DISC )
  {
    continueJoining = TRUE;
    ZDApp_NetworkInit( 0 );

    return ( TRUE );
  }
  else
    return ( FALSE );
}

/*********************************************************************
 * @fn      ZDApp_StopJoiningCycle()
 *
 * @brief   Stops the joining or rejoining process of a device.
 *
 * @param   none
 *
 * @return  TRUE if joining stopped, FALSE if joining or rejoining
 */
uint8 ZDApp_StopJoiningCycle( void )
{
  if ( devState == DEV_INIT || devState == DEV_NWK_DISC )
  {
    continueJoining = FALSE;
    return ( TRUE );
  }
  else
    return ( FALSE );
}

#if !defined ( ZDO_COORDINATOR ) || defined ( SOFT_START )
/*********************************************************************
 * @fn      ZDApp_NodeProfileSync()
 *
 * @brief   Sync node with stack profile.
 *
 * @param   cfm - ZDO_NetworkDiscoveryCfm_t
 *
 * @return  none
 */
void ZDApp_NodeProfileSync( ZDO_NetworkDiscoveryCfm_t* cfm )
{
  networkDesc_t* desc;
  uint16         panID;

  if ( ZDO_Config_Node_Descriptor.CapabilityFlags & CAPINFO_DEVICETYPE_FFD  )
  {
    panID = BUILD_UINT16( cfm->panIdLSB, cfm->panIdMSB );

    desc = nwk_getNetworkDesc( cfm->extendedPANID, panID, cfm->logicalChannel );

    if (desc != NULL)
    {
      if ( desc->stackProfile != zgStackProfile )
      {
        ZDO_Config_Node_Descriptor.LogicalType = NODETYPE_DEVICE;
        ZDO_Config_Node_Descriptor.CapabilityFlags = CAPINFO_DEVICETYPE_RFD | CAPINFO_POWER_AC | CAPINFO_RCVR_ON_IDLE;
      }
    }
  }
}
#endif

/*********************************************************************
*********************************************************************/
