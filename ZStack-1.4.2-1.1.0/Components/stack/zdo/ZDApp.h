#ifndef ZDAPP_H
#define ZDAPP_H

/*********************************************************************
    Filename:       ZDApp.h
    Revised:        $Date: 2007-05-08 09:06:50 -0700 (Tue, 08 May 2007) $
    Revision:       $Revision: 14217 $
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

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "ZMac.h"
#include "NLMEDE.h"
#include "APS.h"
#include "AF.h"
#include "ZDProfile.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Set this value for use in choosing a PAN to join
// modify ZDApp.c to enable this...
#define ZDO_CONFIG_MAX_BO             15

  // Task Events
#define ZDO_NETWORK_INIT          0x0001
#define ZDO_NETWORK_START         0x0002
#define ZDO_DEVICE_RESET          0x0004
#define ZDO_COMMAND_CNF           0x0008
#define ZDO_STATE_CHANGE_EVT      0x0010
#define ZDO_ROUTER_START          0x0020
#define ZDO_NEW_DEVICE            0x0040
#define ZDO_DEVICE_AUTH           0x0080
#define ZDO_SECMGR_EVENT          0x0100
#define ZDO_NWK_UPDATE_NV         0x0200
#define ZDO_FRAMECOUNTER_CHANGE   0x0400

// Incoming to ZDO
#define ZDO_NWK_DISC_CNF        0x01
#define ZDO_NWK_JOIN_IND        0x02
#define ZDO_NWK_JOIN_REQ        0x03
#define ZDO_ESTABLISH_KEY_CFM   0x04
#define ZDO_ESTABLISH_KEY_IND   0x05
#define ZDO_TRANSPORT_KEY_IND   0x06
#define ZDO_UPDATE_DEVICE_IND   0x07
#define ZDO_REMOVE_DEVICE_IND   0x08
#define ZDO_REQUEST_KEY_IND     0x09
#define ZDO_SWITCH_KEY_IND      0x0A

//  ZDO command message fields
#define ZDO_CMD_ID     0
#define ZDO_CMD_ID_LEN 1

//  ZDO security message fields
#define ZDO_ESTABLISH_KEY_CFM_LEN   \
  ((uint8)                          \
   (sizeof(ZDO_EstablishKeyCfm_t) ) )

#define ZDO_ESTABLISH_KEY_IND_LEN   \
  ((uint8)                          \
   (sizeof(ZDO_EstablishKeyInd_t) ) )

#define ZDO_TRANSPORT_KEY_IND_LEN   \
  ((uint8)                          \
   (sizeof(ZDO_TransportKeyInd_t) ) )

#define ZDO_UPDATE_DEVICE_IND_LEN   \
  ((uint8)                          \
   (sizeof(ZDO_UpdateDeviceInd_t) ) )

#define ZDO_REMOVE_DEVICE_IND_LEN   \
  ((uint8)                          \
   (sizeof(ZDO_RemoveDeviceInd_t) ) )

#define ZDO_REQUEST_KEY_IND_LEN   \
  ((uint8)                        \
   (sizeof(ZDO_RequestKeyInd_t) ) )

#define ZDO_SWITCH_KEY_IND_LEN   \
  ((uint8)                       \
   (sizeof(ZDO_SwitchKeyInd_t) ) )

#define NWK_RETRY_DELAY             1000   // in milliseconds

#define ZDO_MATCH_DESC_ACCEPT_ACTION    1   // Message field

// Options for ZDApp_StartUpFromApp()
#define ZDAPP_STARTUP_COORD   2       // Start up as coordinator only
#define ZDAPP_STARTUP_ROUTER  1       // Start up as router only
#define ZDAPP_STARTUP_AUTO    0       // Startup in auto, look for coord,
                                      // if not found, become coord.

#define NUM_DISC_ATTEMPTS           2

// ZDOInitDevice return values
#define ZDO_INITDEV_RESTORED_NETWORK_STATE      0x00
#define ZDO_INITDEV_NEW_NETWORK_STATE           0x01
#define ZDO_INITDEV_LEAVE_NOT_STARTED           0x02

#if defined ( MANAGED_SCAN )
  // Only use in a battery powered device

  // This is the number of times a channel is scanned at the shortest possible
  // scan time (which is 30 MS (2 x 15).  The idea is to scan one channel at a
  // time (from the channel list), but scan it multiple times.
  #define MANAGEDSCAN_TIMES_PRE_CHANNEL         5
  #define MANAGEDSCAN_DELAY_BETWEEN_SCANS       150   // milliseconds

  extern byte zdoDiscCounter;

#endif // MANAGED_SCAN

/*********************************************************************
 * TYPEDEFS
 */
typedef enum
{
  DEV_HOLD,               // Initialized - not started automatically
  DEV_INIT,               // Initialized - not connected to anything
  DEV_NWK_DISC,           // Discovering PAN's to join
  DEV_NWK_JOINING,        // Joining a PAN
  DEV_NWK_REJOIN,         // ReJoining a PAN, only for end devices
  DEV_END_DEVICE_UNAUTH,  // Joined but not yet authenticated by trust center
  DEV_END_DEVICE,         // Started as device after authentication
  DEV_ROUTER,             // Device joined, authenticated and is a router
  DEV_COORD_STARTING,     // Started as Zigbee Coordinator
  DEV_ZB_COORD,           // Started as Zigbee Coordinator
  DEV_NWK_ORPHAN          // Device has lost information about its parent..
} devStates_t;

typedef enum
{
  ZDO_SUCCESS,
  ZDO_FAIL
} zdoStatus_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint8      panIdLSB;
  uint8      panIdMSB;
  uint8      logicalChannel;
  uint8      version;
  uint8      extendedPANID[Z_EXTADDR_LEN];
} ZDO_NetworkDiscoveryCfm_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint8       dstAddrDstEP;
  zAddrType_t dstAddr;
  uint8       dstAddrClusterIDLSB;
  uint8       dstAddrClusterIDMSB;
  uint8       dstAddrRemove;
  uint8       dstAddrEP;
} ZDO_NewDstAddr_t;

//  ZDO security message types
typedef struct
{
  osal_event_hdr_t hdr;
  uint8            partExtAddr[Z_EXTADDR_LEN];
  uint8            status;
} ZDO_EstablishKeyCfm_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16           srcAddr;
  uint8            initExtAddr[Z_EXTADDR_LEN];
  uint8            method;
  uint8            secure;
} ZDO_EstablishKeyInd_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16           srcAddr;
  uint8            keyType;
  uint8            keySeqNum;
  uint8            key[SEC_KEY_LEN];
  uint8            srcExtAddr[Z_EXTADDR_LEN];
  uint8            initiator;
  uint8            secure;
} ZDO_TransportKeyInd_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16           srcAddr;
  uint8            devExtAddr[Z_EXTADDR_LEN];
  uint16           devAddr;
  uint8            status;
} ZDO_UpdateDeviceInd_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16           srcAddr;
  uint8            childExtAddr[Z_EXTADDR_LEN];
} ZDO_RemoveDeviceInd_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16           srcAddr;
  uint8            keyType;
  uint8            partExtAddr[Z_EXTADDR_LEN];
} ZDO_RequestKeyInd_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16           srcAddr;
  uint8            keySeqNum;
} ZDO_SwitchKeyInd_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16 nwkAddr;
  uint8  extAddr[Z_EXTADDR_LEN];
  uint8  numAssocDevs;
  uint8  startIndex;
  uint16 devList[];
} ZDO_IEEEAddrResp_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16 nwkAddr;
  uint8  extAddr[Z_EXTADDR_LEN];
  uint8  numAssocDevs;
  uint8  startIndex;
  uint16 devList[];
} ZDO_NwkAddrResp_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16 nwkAddr;
  uint8  epCnt;
  uint8  epList[];
} ZDO_MatchDescResp_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16 nwkAddr;
  uint8 numInClusters;
  uint16 *pInClusters;
  uint8 numOutClusters;
  uint16 *pOutClusters;
} ZDO_MatchDescRspSent_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16 srcAddr;               // Source of the announcement
  uint16 nwkAddr;               // short address of the announce subject
  uint8 extAddr[Z_EXTADDR_LEN]; // extended address of the announce subject
  uint8 capabilities;
} ZDO_EndDeviceAnnounce_t;

typedef struct
{
  uint16 srcAddr;       // Source of the message
  uint8  transSeq;      // Message sequence number
  uint8  SecurityUse;
} ZDO_MsgHdr_t;

typedef struct
{
  osal_event_hdr_t event_hdr;
  ZDO_MsgHdr_t hdr;
  uint8 srcAddr[Z_EXTADDR_LEN];
  uint8 srcEP;
  uint16 clusterID;
  uint8 dstAddr[Z_EXTADDR_LEN];
  uint8 dstEP;
} ZDO_BindReq_t;

typedef struct
{
  ZDO_MsgHdr_t hdr;
  uint8 startIndex;
} ZDO_MgmtBindReq_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16 nwkAddr;
  uint8  status;
} ZDO_BindRsp_t;

typedef struct
{
  osal_event_hdr_t hdr;
  uint16 nwkAddr;
  uint8  status;
} ZDO_UnbindRsp_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern byte ZDAppTaskID;
extern byte nwkStatus;
extern devStates_t devState;

/* Always kept up to date by the network state changed logic, so use this addr
 * in function calls the require my network address as one of the parameters.
 */
extern zAddrType_t ZDAppNwkAddr;
extern byte saveExtAddr[];  // Loaded with value by ZDApp_Init().

#if defined ( ZDO_MGMT_NWKDISC_RESPONSE )
  extern byte zdappMgmtNwkDiscRspTransSeq;
  extern byte zdappMgmtNwkDiscReqInProgress;
  extern zAddrType_t zdappMgmtNwkDiscRspAddr;
  extern byte zdappMgmtNwkDiscStartIndex;
  extern byte zdappMgmtSavedNwkState;
#endif

/*********************************************************************
 * FUNCTIONS - API
 */

/*********************************************************************
 * Task Level Control
 */
  /*
   * ZdApp Task Initialization Function
   */
  extern void ZDApp_Init( byte task_id );

  /*
   * ZdApp Task Event Processing Function
   */
  extern UINT16 ZDApp_event_loop( byte task_id, UINT16 events );

/*********************************************************************
 * Application Level Functions
 */

/*
 *  Start the device in the network.  This function will read 
 *   ZCD_NV_STARTUP_OPTION (NV item) to determine whether or not to 
 *   restore the network state of the device.
 * 
 *  startDelay - timeDelay to start device (in milliseconds).
 *      There is a jitter added to this delay:
 *              ((NWK_START_DELAY + startDelay)
 *              + (osal_rand() & EXTENDED_JOINING_RANDOM_MASK)) 
 *  
 *  NOTE:   If the application would like to force a "new" join, the
 *          application should set the ZCD_STARTOPT_DEFAULT_NETWORK_STATE
 *          bit in the ZCD_NV_STARTUP_OPTION NV item before calling 
 *          this function.
 *
 *  returns: 
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
extern uint8 ZDOInitDevice( uint16 startDelay );

/*
 *
 * @MT SPI_CMD_ZDO_AUTO_FIND_DESTINATION_REQ
 * (byte Endpoint)
 *
 */
#define ZDApp_AutoFindDestination( endPoint )   ZDApp_AutoFindDestinationEx( endPoint, (uint8 *)0 )
extern void ZDApp_AutoFindDestinationEx( byte endPoint, uint8 *task_id );

/*
 *
 * @MT SPI_CMD_ZDO_AUTO_ENDDEVICEBIND_REQ
 * (byte Endpoint)
 *
 */
extern void ZDApp_SendEndDeviceBindReq( byte endPoint );

/*
 * Sends an osal message to ZDApp with parameter as the msg data byte.
 */
extern void ZDApp_SendEventMsg( byte cmd, byte len, byte *buf );

/*
 * ZdApp Function for Establishing a Link Key
 */
extern ZStatus_t ZDApp_EstablishKey( uint8  endPoint,
                                     uint16 nwkAddr,
                                     uint8* extAddr );

/*
 * Start the network formation process
 *    delay - millisecond wait before
 */
extern void ZDApp_NetworkInit( uint16 delay );

/*********************************************************************
 * Callback FUNCTIONS - API
 */

  /*
   * ZdApp Callback for End Device Bind Request
   */
  extern void ZDApp_EndDeviceBindReqCB( ZDEndDeviceBind_t *bindReq );

  /*
   * ZdApp Callback for Bind Request
   */
  extern void ZDApp_BindReqCB( byte TransSeq,
                               zAddrType_t *SrcAddr,
                               byte *SrcAddress,
                               byte SrcEndPoint,
                               uint16 ClusterID,
                               zAddrType_t *DstAddress,
                               byte DstEndPoint,
                               byte SecurityUse );

  /*
   * ZdApp Callback for Unbind Request
   */
  extern void ZDApp_UnbindReqCB( byte TransSeq,
                                 zAddrType_t *SrcAddr,
                                 byte *SrcAddress,
                                 byte SrcEndPoint,
                                 uint16 ClusterID,
                                 zAddrType_t *DstAddress,
                                 byte DstEndPoint,
                                 byte SecurityUse );

  /*
   * ZdApp Callback for Network Address Response (NWK_addr_rsp)
   */
  extern void ZDApp_NwkAddrRspCB( zAddrType_t *SrcAddr,
                                  byte Status,
                                  byte *IEEEAddr,
                                  uint16 aoi,
                                  byte NumAssocDev,
                                  byte StartIndex,
                                  uint16 *AssocDevList );

  /*
   * ZdApp Callback for IEEE Address Response (IEEE_addr_rsp)
   */
  extern void ZDApp_IEEEAddrRspCB( zAddrType_t *SrcAddr,
                                   byte Status,
                                   byte *IEEEAddr,
                                   uint16 aoi,
                                   byte NumAssocDev,
                                   byte StartIndex,
                                   uint16 *AssocDevList );

  /*
   * ZdApp Callback for a Node Descriptor Response
   */
  extern void ZDApp_NodeDescRspCB( zAddrType_t *SrcAddr,
                                   byte Status,
                                   uint16 aoi,
                                   NodeDescriptorFormat_t *pNodeDesc );

  /*
   * ZdApp Callback for a Power Descriptor Response
   */
  extern void ZDApp_PowerDescRspCB( zAddrType_t *SrcAddr,
                                    byte Status,
                                    uint16 aoi,
                                    NodePowerDescriptorFormat_t *pPwrDesc );

  /*
   * ZdApp Callback for a Simple Descriptor Response
   */
  extern void ZDApp_SimpleDescRspCB( zAddrType_t *SrcAddr,
                                     byte Status,
                                     uint16 aoi,
                                     byte EP,
                                     SimpleDescriptionFormat_t *pSimpleDesc );

  /*
   * ZdApp Callback for a Active Endpoint Response
   */
  extern void ZDApp_ActiveEPRspCB( zAddrType_t *src,
                                   byte Status,
                                   byte epCnt,
                                   byte *epList );

  /*
   * ZdApp Callback for a Match Description Response
   */
  extern void ZDApp_MatchDescRspCB( zAddrType_t *src,
                                    byte Status,
                                    byte epCnt,
                                    byte *epList );

  /*
   * ZdApp Callback for an End Device Bind Response
   */
  extern void ZDApp_EndDeviceBindRsp( zAddrType_t *SrcAddr, byte Status );

  /*
   * ZdApp Callback for a Bind Response
   */
  extern void ZDApp_BindRsp( zAddrType_t *SrcAddr, byte Status );

  /*
   * ZdApp Callback for an Unbind Response
   */
  extern void ZDApp_UnbindRsp( zAddrType_t *SrcAddr, byte Status );

/*********************************************************************
 * Call Back Functions from NWK  - API
 */

/*
 * ZDO_NetworkDiscoveryConfirmCB - scan results
 */

extern ZStatus_t ZDO_NetworkDiscoveryConfirmCB( byte ResultCount,
                              networkDesc_t *NetworkList );

/*
 * ZDO_NetworkFormationConfirm - results of the request to
 *              initialize a coordinator in a network
 */
extern void ZDO_NetworkFormationConfirmCB( ZStatus_t Status );
/*
 * ZDO_JoinConfirmCB - results of its request to join itself or another
 *              device to a network
 */
extern void ZDO_JoinConfirmCB( uint16 PanId, ZStatus_t Status );

/*
 * ZDO_JoinIndicationCB - notified of a remote join request
 */
extern ZStatus_t ZDO_JoinIndicationCB( uint16 ShortAddress,
                      byte *ExtendedAddress, byte CapabilityInformation );

/*
 * ZDO_StartRouterConfirm -  results of the request to
 *              start functioning as a router in a network
 */
extern void ZDO_StartRouterConfirmCB( ZStatus_t Status );

/*
 * ZDO_LeaveCnf - results of the request for this or a child device
 *                to leave
 */
extern void ZDO_LeaveCnf( NLME_LeaveCnf_t* cnf );

/*
 * ZDO_LeaveInd - notified of a remote leave request or indication
 */
extern void ZDO_LeaveInd( NLME_LeaveInd_t* ind );

/*
 * ZDO_SyncIndicationCB - notified of sync loss with parent
 */
extern void ZDO_SyncIndicationCB( byte type, uint16 shortAddr );

/*
 * ZDO_PollConfirmCB - notified of poll confirm
 */
extern void ZDO_PollConfirmCB( byte status );

/*********************************************************************
 * Call Back Functions from Security  - API
 */
extern ZStatus_t ZDO_UpdateDeviceIndication( byte *extAddr, byte status );



/*
 * ZDApp_InMsgCB - Allow the ZDApp to handle messages that are not supported
 */
extern void ZDApp_InMsgCB( byte TransSeq,
                           zAddrType_t *srcAddr,
                           byte wasBroadcast,
                           uint16 clusterID,
                           byte asduLen,
                           byte *asdu,
                           byte SecurityUse );

extern void ZDO_StartRouterConfirm( ZStatus_t Status );

/*
 * ZDApp_MgmtLqiRspCB - Apps incoming response indication
 */
extern void ZDApp_MgmtLqiRspCB( uint16 SrcAddr, byte Status,
                         byte NeighborLqiEntries,
                         byte StartIndex, byte NeighborLqiCount,
                         neighborLqiItem_t *pList );

/*
 * ZDApp_MgmtNwkDiscRspCB - Apps incoming response indication
 */
extern void ZDApp_MgmtNwkDiscRspCB( uint16 SrcAddr, byte Status,
                         byte NetworkCount,
                         byte StartIndex, byte networkListCount,
                         mgmtNwkDiscItem_t *pList );

/*
 * ZDApp_MgmtRtgRspCB - Apps incoming response indication
 */
extern void ZDApp_MgmtRtgRspCB( uint16 SrcAddr, byte Status, byte rtgCount,
                         byte StartIndex, byte rtgListCount,
                         rtgItem_t *pList );

/*
 * ZDO_ProcessMgmtBindRsp - Apps incoming response indication
 */
extern void ZDApp_MgmtBindRspCB( uint16 SrcAddr, byte Status, byte BindingCount,
                         byte StartIndex, byte BindingListCount,
                         apsBindingItem_t *pList );

/*
 * ZDApp_MgmtDirectJoinRspCB - Apps incoming response indication
 */
extern void ZDApp_MgmtDirectJoinRspCB( uint16 SrcAddr, byte Status,
                          byte SecurityUse );

/*
 * ZDApp_MgmtLeaveRspCB - Apps incoming response indication
 */
extern void ZDApp_MgmtLeaveRspCB( uint16 SrcAddr, byte Status,
                          byte SecurityUse );

#if defined ( ZDO_MGMT_BIND_RESPONSE ) && !defined( REFLECTOR )
/*
 * ZDApp_MgmtBindReqCB - This function finishes the processing of the Management
 *              Bind Request and generates the response.
 */
extern void ZDApp_MgmtBindReqCB( byte TransSeq, zAddrType_t *SrcAddr,
                                byte StartIndex, byte SecurityUse );
#endif

/*
 * ZDApp_MgmtPermitJoinCB - Apps incoming response indication
 */
extern void ZDApp_MgmtPermitJoinRspCB( uint16 SrcAddr, byte Status,
                          byte SecurityUse );

/*
 * ZDApp_UserDescRspCB - Apps incoming response indication
 */
extern void ZDApp_UserDescRspCB( uint16 SrcAddr, byte status,
                          uint16 nwkAddrOfInterest, byte userDescLen,
                          byte *userDesc, byte SecurityUse );


/*
 * ZDApp_UserDescConfCB - Apps incoming response indication
 */
extern void ZDApp_UserDescConfCB( uint16 SrcAddr, byte status,
                                  byte SecurityUse );

/*
 * ZDApp_ServerDiscRspCB - Handle the Server_Discovery_rsp response.
 */
#if defined ( ZDO_SERVERDISC_REQUEST )
void ZDApp_ServerDiscRspCB( uint16 srcAddr, byte status,
                            uint16 serverMask, byte securityUse );
#endif

/*
 * ZDApp_EndDeviceAnnounceCB - Received an End Device Announce
 */
extern void ZDApp_EndDeviceAnnounceCB( uint16 SrcAddr, uint16 nwkAddr,
                                      uint8 *extAddr, uint8 capabilities );

/*********************************************************************
 * ZDO Control Functions
 */

/*
 * ZDApp_ChangeMatchDescRespPermission
 *    - Change the Match Descriptor Response permission.
 */
extern void ZDApp_ChangeMatchDescRespPermission( uint8 endpoint, uint8 action );

/*
 * ZDApp_ResetNwkKey
 *    - Re initialize the NV Nwk Key
 */
extern void ZDApp_ResetNwkKey( void );

/*
 * ZDApp_StartUpFromApp
 *    - Start the device.  This function will only start a device
 *          that is in the Holding state.
 *
 * mode - ZDAPP_STARTUP_COORD - Start up as coordinator only
 *        ZDAPP_STARTUP_ROUTER - Start up as router only
 *        ZDAPP_STARTUP_AUTO - Startup in auto, look for coord,
 *                               if not found, become coord.
 *
 * returns  ZSucess if started, ZFailure if in the wrong mode
 */
extern ZStatus_t ZDApp_StartUpFromApp( uint8 mode );

/*
 * ZDApp_StopStartUp
 *    - Stops the joining process of a router.  This will only
 *          work if the router is in the scanning process and
 *          the SOFT_START feature is enabled.
 *
 *    returns  TRUE if SOFT_START is enabled, FALSE if not possible
 */
extern uint8 ZDApp_StopStartUp( void );


/*
 * ZDApp_StartJoiningCycle
 *    - Starts the joining cycle of a device.  This will only continue an
 *      already started (or stopped) joining cycle.
 *
 *    returns  TRUE if joining started, FALSE if not in joining or rejoining
 */
extern uint8 ZDApp_StartJoiningCycle( void );

/*
 * ZDApp_StopJoiningCycle
 *    - Stops the joining or rejoining process of a device.
 *
 *    returns  TRUE if joining stopped, FALSE if joining or rejoining
 */
extern uint8 ZDApp_StopJoiningCycle( void );



/*********************************************************************
 * ZDO Information Notification Registration Functions
 */

/*
 * Register to receive IEEE Addr Response messages
 */
extern void ZDApp_RegisterForIEEEAddrRsp( byte TaskID );

/*
 * Register to receive NWK Addr Response messages
 */
extern void ZDApp_RegisterForNwkAddrRsp( byte TaskID );

/*
 * Register to receive Match Descriptor Response messages
 */
extern void ZDApp_RegisterForMatchDescRsp( byte TaskID );

/*
 * Register to receive End Device Announce messages
 */
extern void ZDApp_RegisterForEndDeviceAnnounce( byte TaskID );

#if defined ( ZDO_BIND_UNBIND_REQUEST )
/*
 * Register to receive Bind_rsp and Unbind_rsp messages
 */
extern void ZDApp_RegisterForBindRsp( byte TaskID );
#endif

#if defined ( ZDO_BIND_UNBIND_RESPONSE ) && !defined ( REFLECTOR )
/*
 * Register to receive Bind and Unbind Request messages
 */
extern void ZDApp_RegisterForBindReq( byte TaskID );
#endif

#if defined ( ZDO_MGMT_BIND_RESPONSE ) && !defined ( REFLECTOR )
/*
 * Register to receive Mgmt Bind Request messages
 */
extern void ZDApp_RegisterForMgmtBindReq( byte TaskID );
#endif


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZDOBJECT_H */
