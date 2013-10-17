#ifndef NWK_H
#define NWK_H
/*********************************************************************
    Filename:       nwk.h
    Revised:        $Date: 2007-01-08 12:56:09 -0800 (Mon, 08 Jan 2007) $
    Revision:       $Revision: 13236 $

    Description:

        Network layer logic component interface.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "ZMAC.h"
#include "nwk_bufs.h"
#include "NLMEDE.h"
#include "ssp.h"

//Temp
#include "MTEL.h"

/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * CONSTANTS
 */

//NWK event identifiers
#define MAC_SCAN_REQ          0x01
#define NWK_NETWORKSTART_REQ  0x02
#define MAC_ASSOCIATE_REQ     0x03
#define NWK_REMOTE_GET_REQ    0x04
#define NWK_REMOTE_SET_REQ    0x05
#define NWK_ASSOCIATE_RESP    0x06
#define NWK_DISASSOCIATE_REQ  0x07

#define NWK_AUTO_POLL_EVT     0x0001
#define NWK_NOT_EXPECTING_EVT 0x0004
#define RTG_TIMER_EVENT       0x0010
#define NWK_DATABUF_SEND      0x0020
#define NWK_BCAST_TIMER_EVT   0x0040
#define NWK_PERMITJOIN_EVT    0x0080
#define NWK_STREE_TO_EVT      0x0100

// MAC Callback messages
#define MACCB_DATA_CNF_CMD                0x20
#define MACCB_DATA_IND_CMD                0x21
#define MACCB_ASSOCIATE_IND_CMD           0x22
#define MACCB_ASSOCIATE_CNF_CMD           0x23
#define MACCB_DISASSOCIATE_IND_CMD        0x24
#define MACCB_SCAN_CNF_CMD                0x25
#define MACCB_BEACON_NOTIFY_IND_CMD       0x26
#define MACCB_GTS_CNF_CMD                 0x27
#define MACCB_GTS_IND_CMD                 0x28
#define MACCB_ORPHAN_IND_CMD              0x29
#define MACCB_DISASSOCIATE_CNF_CMD        0x2A
#define MACCB_SECURITY_ERROR_IND_CMD      0x30
#define MACCB_START_CNF_CMD               0x31
#define MACCB_SYNC_LOSS_IND_CMD           0x32
#define MACCB_POLL_CNF_CMD                0x33
#define MACCB_LLC_UNITDATA_IND_CMD        0x34
#define MACCB_LLC_UNITDATA_STATUS_IND_CMD 0x35
#define MACCB_PURGE_CNF_CMD               0x36
#define MACCB_COMM_STATUS_IND_CMD         0x37
#define MACCB_CMD_DATA_REQ                0x40
#define MACCB_POLL_IND_CMD                0x41

//NWK PACKET: FIELD IDENTIFIERS
#define NWK_CMD_ID                  0
#define NWK_PARAMS_ID               1
#define NWK_REQ_ATTR_ID             1
#define NWK_REQ_ATTR                2
#define NWK_CMD_PYLD_BEGIN          NWK_HEADER_LEN
#define NWK_DEVICE_LIST_LEN_FIELD   NWK_HEADER_LEN + 1

// This value needs to be set or read from somewhere
#define ED_SCAN_MAXCHANNELS 27

// Max length of packet that can be sent to the MAC
#define MAX_DATA_PACKET_LEN 102

#define NWK_TASK_ID              0
#define ASSOC_CAPABILITY_INFO    0
#define ASSOC_SECURITY_EN        0

#define DEF_DEST_EP              2
#define DEVICE_APPLICATION       0

#define MAC_ADDR_LEN             8

#define NWK_TXOPTIONS_ACK        0x01
#define NWK_TXOPTIONS_INDIRECT   0x04

// TxOptions for a data request - Indirect data and ACK required
#define NWK_TXOPTIONS_COORD      (NWK_TXOPTIONS_ACK | NWK_TXOPTIONS_INDIRECT)

// TxOptions for a data request - Direct data and ACK required
//#define NWK_TXOPTIONS_COORD       (NWK_TXOPTIONS_ACK)

//Assume value until defined By SuperApp or design spec
#define DEF_MAX_NUM_COORDINATORS 15        //Default value
#define DEF_CHANNEL_SCAN_BITMAP  MAX_CHANNELS_24GHZ
#define SOFT_SCAN_DURATION       1         //in secs

#define DEF_SCAN_DURATION        2

#define NO_BEACONS              15

#define DEF_BEACON_ORDER         NO_BEACONS
//#define DEF_BEACON_ORDER         10   // 15 seconds
//#define DEF_BEACON_ORDER         9    // 7.5 seconds
//#define DEF_BEACON_ORDER         8    // 3.75 seconds
//#define DEF_BEACON_ORDER         6    // 1 second
//#define DEF_BEACON_ORDER         1    // 30 millisecond

//#define DEF_SUPERFRAMEORDER      2
#define DEF_SUPERFRAMEORDER      DEF_BEACON_ORDER
#define NWK_SECURITY_ENABLE      FALSE
#define NWK_MAC_ASSOC_CNF_LEN    4
#define FIXED_SIZ_MAC_DATA_CNF   4         //Length of all fixed params except data
#define FIXED_SIZ_MAC_DATA_IND   26
#define FIXED_SIZ_MAC_SCAN_CNF   7

#define ALL_PAIRING_TABLE_ENTRIES   0
#define SIZE_OF_PAIRING_TABLE_ENTRY 6 //Two short addr and two endpts
#define SIZE_OF_DEVICE_LIST_ENTRY   2 //short addr in dev list is 2 bytes

// the default network radius set twice the value of <nwkMaxDepth>
#define DEF_NWK_RADIUS 10

#define NWK_SEND_TIMER_INTERVAL  2
#define NWK_BCAST_TIMER_INTERVAL 100 // NWK_BCAST_TIMER_EVT duration

#define INVALID_NODE_ADDR                           0xFFFE
#define INVALID_PAN_ID                              0xFFFE

#define DEF_LINK_COST                                   2   // starting tx cost
#define MAX_LINK_COST                                   4   // max tx cost
#define LINK_DOWN_TRIGGER             3   // link is down if txCost exceeds this
#define LINK_ACTIVE_TRIGGER           2   // link is up if txCost goes below this

//NWK Callback subscription IDs
#define CB_ID_APP_ANNOUNCE_CNF          0x00
#define CB_ID_APP_ASSOCIATE_CNF         0x01
#define CB_ID_APP_ASSOCIATE_IND         0x02
#define CB_ID_APP_DATA_CNF              0x03
#define CB_ID_APP_DATA_IND              0x04
#define CB_ID_APP_DISASSOCIATE_CNF      0x05
#define CB_ID_APP_DISASSOCIATE_IND      0x06
#define CB_ID_APP_NETWORK_DETECT_CNF    0x07
#define CB_ID_APP_REMOTE_GET_CNF        0x08
#define SPI_CB_APP_REMOTE_SET_CNF       0x09
#define CB_ID_APP_SERVICE_CNF           0x0a
#define CB_ID_APP_SERVICE_IND           0x0b
#define CB_ID_APP_START_CNF             0x0c

#define NUM_PING_ROUTE_ADDRS            12
#define PING_ROUTE_ADDRS_INDEX          8

#define NWK_GetNodeDepth()              (_NIB.nodeDepth)
#define NWK_GetTreeDepth()              (0)

/*********************************************************************
 * TYPEDEFS
 */
typedef enum
{
  NWK_INIT,
  NWK_JOINING_ORPHAN,
  NWK_DISC,
  NWK_JOINING,
  NWK_ENDDEVICE,
  PAN_CHNL_SELECTION,
  PAN_CHNL_VERIFY,
  PAN_STARTING,
  NWK_ROUTER,
  NWK_REJOINING
} nwk_states_t;

// MAC Command Buffer types
typedef enum
{
  MACCMDBUF_NONE,
  MACCMDBUF_ASSOC_REQ,
  MACCMDBUF_DISASSOC_REQ
} nwkMacCmds_t;


// Neighbor table entry
typedef struct
{
  uint16  neighborAddress;
  uint16  panId;
  linkInfo_t linkInfo;
} neighborEntry_t;

typedef struct
{
  byte  SequenceNum;
  byte  PassiveAckTimeout;
  byte  MaxBroadcastRetries;
  byte  MaxChildren;
  byte  MaxDepth;
  byte  MaxRouters;

  //neighborEntry_t *       pNeighborTable;
  byte  dummyNeighborTable;     // to make everything a byte!!

  byte  BroadcastDeliveryTime;
  byte  ReportConstantCost;
  byte  RouteDiscRetries;

  //rtgEntry_t *                pRoutingTable;
  byte  dummyRoutingTable;      // to make everything a byte!!

  byte  SecureAllFrames;
  byte  SecurityLevel;
  byte  SymLink;
  byte  CapabilityInfo;

  uint16 TransactionPersistenceTime;

  byte   nwkProtocolVersion;

  // non-standard attributes
  byte  RouteDiscoveryTime;
  byte  RouteExpiryTime;        // set to 0 to turn off expiration of routes

  // non-settable
  uint16  nwkDevAddress;
  byte    nwkLogicalChannel;
  uint16  nwkCoordAddress;
  byte    nwkCoordExtAddress[Z_EXTADDR_LEN];
  uint16  nwkPanId;

  // Other global items - non-settable
  nwk_states_t  nwkState;
  uint32        channelList;
  byte          beaconOrder;
  byte          superFrameOrder;
  byte          scanDuration;
  byte          battLifeExt;
  uint32        allocatedRouterAddresses;
  uint32        allocatedEndDeviceAddresses;
  byte          nodeDepth;

  // Version 1.1 - extended PAN ID
  uint8         extendedPANID[Z_EXTADDR_LEN];
  
  // Key information
  uint8      nwkKeyLoaded;
  nwkKeyDesc nwkActiveKey;
  nwkKeyDesc nwkAlternateKey;
  
} nwkIB_t;


/*********************************************************************
 * GLOBAL VARIABLES
 */
extern nwkIB_t _NIB;
extern byte NWK_TaskID;
extern networkDesc_t *NwkDescList;
extern byte nwkExpectingMsgs;
extern neighborEntry_t  neighborTable[];
extern byte nwk_beaconPayload[ZMAC_MAX_BEACON_PAYLOAD_LEN];
extern byte nwk_beaconPayloadSize;

/*********************************************************************
 * FUNCTIONS
 */

 /*
 * NWK Task Initialization
 */
extern void nwk_init( byte task_id );

 /*
 * Calls mac_data_req then handles the buffering
 */
extern ZStatus_t nwk_data_req_send( nwkDB_t* db );

 /*
 * NWK Event Loop
 */
extern UINT16 nwk_event_loop( byte task_id, UINT16 events );

 /*
 * Process incoming command packet
 */
//extern void CommandPktProcessing( NLDE_FrameFormat_t *ff );

#if defined(RTR_NWK)
 /*
 * Start a coordinator
 */
 extern ZStatus_t nwk_start_coord( void );

#endif

/*
 * Free any network discovery data
 */
extern void nwk_desc_list_free( void );
networkDesc_t *nwk_getNetworkDesc( uint8 *ExtendedPANID, uint16 PanId, byte Channel );
extern void nwk_BeaconFromNative(byte* buff, byte size, beaconPayload_t* beacon);
extern void nwk_BeaconToNative(beaconPayload_t* beacon, byte* buff, byte size);

/*********************************************************************
 * Functionality - not to be called directly.
 */
extern void nwk_ScanJoiningOrphan( ZMacScanCnf_t *param );
extern void nwk_ScanPANChanSelect( ZMacScanCnf_t *param );
extern void nwk_ScanPANChanVerify( ZMacScanCnf_t *param );
extern void nwk_associate_cnf_processing( ZMacAssociateCnf_t *param );
extern void nwk_associate_ind_processing( ZMacAssociateInd_t *param );


/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* NWK_H */


