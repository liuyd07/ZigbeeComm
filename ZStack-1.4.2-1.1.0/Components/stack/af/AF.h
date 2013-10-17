#ifndef AF_H
#define AF_H
/*********************************************************************
    Filename:       AF.h
    Revised:        $Date: 2007-02-23 11:29:38 -0800 (Fri, 23 Feb 2007) $
    Revision:       $Revision: 13588 $

    Description:

       This file contains the General Operational Framework definitions.

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
#include "nwk.h"
#include "APSMEDE.h"

/*********************************************************************
 * CONSTANTS
 */

// Manage AF code size with the following conditional compilation flags.
#if !defined ( AF_KVP_SUPPORT )
  #define AF_KVP_SUPPORT FALSE
#endif

#if !defined ( AF_V1_SUPPORT )
  #define AF_V1_SUPPORT  FALSE
#endif

#if !defined ( AF_FLOAT_SUPPORT )
  #define AF_FLOAT_SUPPORT  FALSE
#endif

#define AF_BROADCAST_ENDPOINT              0xFF

#define AF_FRAGMENTED                      0x01
#define AF_ACK_REQUEST                     0x10
#define AF_DISCV_ROUTE                     0x20
#define AF_EN_SECURITY                     0x40
#define AF_SKIP_ROUTING                    0x80

// Backwards support for afAddOrSendMessage / afFillAndSendMessage.
#if ( AF_V1_SUPPORT || AF_KVP_SUPPORT )
  #define AFCMD_KVPMSG_TRANSID             1
#endif
#define AF_TX_OPTIONS_NONE                 0
#define AF_MSG_ACK_REQUEST                 AF_ACK_REQUEST

// Default Radius Count value
#define AF_DEFAULT_RADIUS                  DEF_NWK_RADIUS

/*********************************************************************
 * Node Descriptor
 */

#define AF_MAX_USER_DESCRIPTOR_LEN         16
#define AF_USER_DESCRIPTOR_FILL          0x20
typedef struct
{
  byte len;     // Length of string descriptor
  byte desc[AF_MAX_USER_DESCRIPTOR_LEN];
} UserDescriptorFormat_t;

// Node Logical Types
#define NODETYPE_COORDINATOR    0x00
#define NODETYPE_ROUTER         0x01
#define NODETYPE_DEVICE         0x02

// Node Frequency Band - bit map
#define NODEFREQ_800            0x01    // 868 - 868.6 MHz
#define NODEFREQ_900            0x04    // 902 - 928 MHz
#define NODEFREQ_2400           0x08    // 2400 - 2483.5 MHz

// Node MAC Capabilities - bit map
//   Use CAPINFO_ALTPANCOORD, CAPINFO_DEVICETYPE_FFD,
//       CAPINFO_DEVICETYPE_RFD, CAPINFO_POWER_AC,
//       and CAPINFO_RCVR_ON_IDLE from NLMEDE.h

// Node Descriptor format structure
typedef struct
{
  byte LogicalType:3;
  byte ComplexDescAvail:1;  /* AF_V1_SUPPORT - reserved bit. */
  byte UserDescAvail:1;     /* AF_V1_SUPPORT - reserved bit. */
  byte Reserved:3;
  byte APSFlags:3;
  byte FrequencyBand:5;
  byte CapabilityFlags;
  byte ManufacturerCode[2];
  byte MaxBufferSize;
  byte MaxTransferSize[2];
  uint16 ServerMask;
} NodeDescriptorFormat_t;

// Bit masks for the ServerMask.
#define PRIM_TRUST_CENTER  0x01
#define BKUP_TRUST_CENTER  0x02
#define PRIM_BIND_TABLE    0x04
#define BKUP_BIND_TABLE    0x08
#define PRIM_DISC_TABLE    0x10
#define BKUP_DISC_TABLE    0x20

/*********************************************************************
 * Node Power Descriptor
 */

// Node Current Power Modes (CURPWR)
// Receiver permanently on or sync with coordinator beacon.
#define NODECURPWR_RCVR_ALWAYS_ON   0x00
// Receiver automatically comes on periodically as defined by the
// Node Power Descriptor.
#define NODECURPWR_RCVR_AUTO        0x01
// Receiver comes on when simulated, eg by a user pressing a button.
#define NODECURPWR_RCVR_STIM        0x02

// Node Available Power Sources (AVAILPWR) - bit map
//   Can be used for AvailablePowerSources or CurrentPowerSource
#define NODEAVAILPWR_MAINS          0x01  // Constant (Mains) power
#define NODEAVAILPWR_RECHARGE       0x02  // Rechargeable Battery
#define NODEAVAILPWR_DISPOSE        0x04  // Disposable Battery

// Power Level
#define NODEPOWER_LEVEL_CRITICAL    0x00  // Critical
#define NODEPOWER_LEVEL_33          0x04  // 33%
#define NODEPOWER_LEVEL_66          0x08  // 66%
#define NODEPOWER_LEVEL_100         0x0C  // 100%

// Node Power Descriptor format structure
typedef struct
{
  unsigned int PowerMode:4;
  unsigned int AvailablePowerSources:4;
  unsigned int CurrentPowerSource:4;
  unsigned int CurrentPowerSourceLevel:4;
} NodePowerDescriptorFormat_t;

/*********************************************************************
 * Simple Descriptor
 */

// AppDevVer values
#if ( AF_V1_SUPPORT )
  #define APPDEVVER_1               0x00
#else
  #define APPDEVVER_1               0x01
#endif

// AF_V1_SUPPORT AppFlags - bit map
#define APPFLAG_NONE                0x00  // Backwards compatibility to AF_V1.

// AF-AppFlags - bit map
#define AF_APPFLAG_NONE             0x00
#define AF_APPFLAG_COMPLEXDESC      0x01  // Complex Descriptor Available
#define AF_APPFLAG_USERDESC         0x02  // User Descriptor Available

typedef uint16  cId_t;
// Simple Description Format Structure
typedef struct
{
  byte          EndPoint;
  uint16        AppProfId;
  uint16        AppDeviceId;
  byte          AppDevVer:4;
  byte          Reserved:4;             // AF_V1_SUPPORT uses for AppFlags:4.
  byte          AppNumInClusters;
  cId_t         *pAppInClusterList;
  byte          AppNumOutClusters;
  cId_t         *pAppOutClusterList;
} SimpleDescriptionFormat_t;

/*********************************************************************
 * AF Message Format
 */

// Frame Types
#define FRAMETYPE_KVP          0x01     // 0001
#define FRAMETYPE_MSG          0x02     // 0010

#if ( AF_KVP_SUPPORT )
  // Command Types
  #define CMDTYPE_SET          0x01
  #define CMDTYPE_EVENT        0x02
  #define CMDTYPE_GET_ACK      0x04     // GET with ACK
  #define CMDTYPE_SET_ACK      0x05     // SET with ACK
  #define CMDTYPE_EVENT_ACK    0x06     // EVENT with ACK
  #define CMDTYPE_GET_RESP     0x08     // GET Response
  #define CMDTYPE_SET_RESP     0x09     // SET Response
  #define CMDTYPE_EVENT_RESP   0x0A     // EVENT Response

  // Attribute Data Types
  #define DATATYPE_NO_DATA     0x00
  #define DATATYPE_UINT8       0x01     // unsigned integer 8 bit
  #define DATATYPE_INT8        0x02     // signed integer 8 bit
  #define DATATYPE_UINT16      0x03     // unsigned integer 16 bit
  #define DATATYPE_INT16       0x04     // signed integer 16 bit
  #define DATATYPE_SEMIPREC    0x0B     // Semi-Precision (16 bit)
  #define DATATYPE_ABS_TIME    0x0C     // Absolute Time (32 bits)
  #define DATATYPE_REL_TIME    0x0D     // Relative Time (32 bits)
  #define DATATYPE_CHAR_STR    0x0E     // Character String
  #define DATATYPE_OCTET_STR   0x0F     // Octet String

  // Error Codes
  #define ERRORCODE_UNSUPPORTED_ATTRIB  0x03
  #define ERRORCODE_INVALIDE_CMDTYPE    0x04
  #define ERRORCODE_INVALID_DATALENGTH  0x05
  #define ERRORCODE_INVALID_DATA        0x06
#endif

#define ERRORCODE_SUCCESS               0x00

#define AF_HDR_KVP_MAX_LEN   0x08  // Max possible AF KVP header.
#define AF_HDR_V1_0_MAX_LEN  0x03  // Max possible AF Ver 1.0 header.
#define AF_HDR_V1_1_MAX_LEN  0x00  // Max possible AF Ver 1.1 header.

#if ( AF_KVP_SUPPORT )
  // Generalized KVP Command Format
  typedef struct
  {
    byte           TransSeqNumber;
    unsigned int   CommandType:4;
    unsigned int   AttribDataType:4;
    uint16         AttribId;
    byte           ErrorCode;        // Only used in responses
    byte           DataLength;  // Number of bytes in TransData
    byte           *Data;
  } afKVPCommandFormat_t;
#endif

#if ( AF_V1_SUPPORT || AF_KVP_SUPPORT )
  // Add Or Send - Multiple Transactions
  typedef enum
  {
    ADD_MESSAGE,
    SEND_MESSAGE
  } afAddOrSend_t;
#endif

// Generalized MSG Command Format
typedef struct
{
  byte   TransSeqNumber;
  uint16 DataLength;               // Number of bytes in TransData
  byte  *Data;
} afMSGCommandFormat_t;

typedef enum
{
  noLatencyReqs,
  fastBeacons,
  slowBeacons
} afNetworkLatencyReq_t;

/*********************************************************************
 * Endpoint  Descriptions
 */

typedef enum
{
  afAddrNotPresent = AddrNotPresent,
  afAddr16Bit      = Addr16Bit,
  afAddrGroup      = AddrGroup,
  afAddrBroadcast  = AddrBroadcast
} afAddrMode_t;

typedef struct
{
  union
  {
    uint16  shortAddr;
  } addr;
  afAddrMode_t addrMode;
  byte endPoint;
} afAddrType_t;

#if ( AF_KVP_SUPPORT )
  typedef struct
  {
    osal_event_hdr_t hdr;
    uint16 clusterId;
    afAddrType_t srcAddr;
    byte endPoint;                  // Destination endpoint
    byte wasBroadcast;
    uint8 LinkQuality;
    byte SecurityUse;
    byte count;                     // trans count number within packet (1-n)
    byte totalTrans;                // total number of trans in packet
    afKVPCommandFormat_t cmd;
  } afIncomingKVPPacket_t;
#endif

typedef struct
{
  osal_event_hdr_t hdr;
  uint16 groupId;
  uint16 clusterId;
  afAddrType_t srcAddr;
  byte endPoint;
  byte wasBroadcast;
  byte LinkQuality;
  byte SecurityUse;
  uint32 timestamp;
  afMSGCommandFormat_t cmd;
} afIncomingMSGPacket_t;

typedef struct
{
  osal_event_hdr_t hdr;
  byte endpoint;
  byte transID;
} afDataConfirm_t;

// Endpoint Table - this table is the device description
// or application registration.
// There will be one entry in this table for every
// endpoint defined.
typedef struct
{
  byte endPoint;
  byte *task_id;  // Pointer to location of the Application task ID.
  SimpleDescriptionFormat_t *simpleDesc;
  afNetworkLatencyReq_t latencyReq;
} endPointDesc_t;

// Typedef for callback function to retrieve an endpoints
//   descriptors, contained in the endpoint descriptor.
//   This will allow an application to dynamically change
//   the descriptor and not use the RAM/ROM.
typedef void *(*pDescCB)( uint8 type, uint8 endpoint );

// Descriptor types used in the above callback
#define AF_DESCRIPTOR_SIMPLE            1
#define AF_DESCRIPTOR_PROFILE_ID        2

// Bit definitions for epList_t flags.
typedef enum
{
  eEP_AllowMatch = 1,
#if ( AF_KVP_SUPPORT )
  eEP_UsesKVP =    2,
#endif
  eEP_NotUsed
} eEP_Flags;

typedef struct
{
  endPointDesc_t *epDesc;
  uint16 reflectorAddr;
  eEP_Flags flags;
  pDescCB  pfnDescCB;     // Don't use if this function pointer is NULL.
  void *nextDesc;
} epList_t;

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */

typedef enum
{
  afStatus_SUCCESS,
  afStatus_FAILED = 0x80,
  afStatus_MEM_FAIL,
  afStatus_INVALID_PARAMETER
} afStatus_t;

typedef struct
{
  uint8              kvp;
  APSDE_DataReqMTU_t aps;
} afDataReqMTU_t;


/*********************************************************************
 * Special AF Data Types
 */

#if ( AF_FLOAT_SUPPORT )
  // ZigBee Absolute Time format (32 bit)
  typedef uint32 afAbsoluteTime_t;

  // ZigBee Relative Time format (32 bit)
  typedef uint32 afRelativeTime_t;

  // ZigBee Semi-Precision format (16 bit)
  typedef struct
  {
    unsigned int sign:1;
    unsigned int exponent:5;
    unsigned int mantissa:10;
  } afSemiPrecision_t;
#endif

/*********************************************************************
 * Globals
 */

extern epList_t *epList;

/*********************************************************************
 * FUNCTIONS
 */

 /*
  * afInit - Initialize the AF.
  */
  extern void afInit( void );

 /*
  * afRegisterExtended - Register an Application's EndPoint description
  *           with a callback function for descriptors.
  *
  */
  extern epList_t *afRegisterExtended( endPointDesc_t *epDesc, pDescCB descFn );

 /*
  * afRegister - Register an Application's EndPoint description.
  *
  */
  extern afStatus_t afRegister( endPointDesc_t *epDesc );

 /*
  * afRegisterFlags - If trying to run KVP under V2 network, any non-ZDO
  * endpoint is assumed to be running KVP unless it is specially registered
  * to not use it.
  */
#if ( AF_KVP_SUPPORT )
  extern afStatus_t afRegisterFlags( endPointDesc_t *epDesc, eEP_Flags flags );
#endif

 /*
  * afDataConfirm - APS will call this function after a data message
  *                 has been sent.
  */
  extern void afDataConfirm( uint8 endPoint, uint8 transID, ZStatus_t status );

 /*
  * afIncomingData - APS will call this function when an incoming
  *                   message is received.
  */
  extern void afIncomingData( aps_FrameFormat_t *aff, zAddrType_t *SrcAddress,
                       uint8 LinkQuality, byte SecurityUse, uint32 timestamp );

#if ( AF_KVP_SUPPORT )
 /*
  */
  afStatus_t afAddOrSendMessage(
    afAddrType_t *dstAddr, byte srcEndPoint, cId_t clusterID,
    afAddOrSend_t AddOrSend, byte FrameType, byte *TransSeqNumber,
    byte CommandType, byte AttribDataType, uint16 AttribId, byte ErrorCode,
    byte DataLength, byte *Data,
    byte txOptions, byte DiscoverRoute, byte RadiusCounter );
#endif

#if ( AF_V1_SUPPORT || AF_KVP_SUPPORT )
 /*
  * Fills in the cmd format structure and sends the out-going message.
  */
  afStatus_t afFillAndSendMessage(
    afAddrType_t *dstAddr, byte srcEndPoint, cId_t clusterID,
    byte TransCount, byte FrameType, byte *TransSeqNumber,
    byte CommandType, byte AttribDataType, uint16 AttribId, byte ErrorCode,
    byte DataLength, byte *Data,
    byte txOptions, byte DiscoverRoute, byte RadiusCounter );
#endif

 /*
  * Common functionality for invoking APSDE_DataReq() for both
  * KVP-Send/SendMulti and MSG-Send.
  */
#if ( AF_V1_SUPPORT )
#define AF_DataRequest(dstAddr, srcEP, cID, len, buf, transID, options, radius)\
  afFillAndSendMessage( (dstAddr), (srcEP)->endPoint, (cID), \
                         1, FRAMETYPE_MSG, (transID), \
                         NULL, NULL, NULL, NULL, (byte)(len), (buf), \
                        (options), (byte)((options) & AF_DISCV_ROUTE), (radius) )
#else
  afStatus_t AF_DataRequest( afAddrType_t *dstAddr, endPointDesc_t *srcEP,
                             uint16 cID, uint16 len, uint8 *buf, uint8 *transID,
                             uint8 options, uint8 radius );
#endif

/*********************************************************************
 * Direct Access Functions - ZigBee Device Object
 */

 /*
  *	afFindEndPointDesc - Find the endpoint description entry from the
  *                      endpoint number.
  */
  extern endPointDesc_t *afFindEndPointDesc( byte endPoint );

 /*
  *	afFindSimpleDesc - Find the Simple Descriptor from the endpoint number.
  *   	  If return value is not zero, the descriptor memory must be freed.
  */
  extern byte afFindSimpleDesc( SimpleDescriptionFormat_t **ppDesc, byte EP );

 /*
  *	afGetReflector - Find the reflector for the endpoint.
  *         0xFFFF return if not found
  */
  extern uint16 afGetReflector( byte EndPoint );

 /*
  *	afSetReflector - Set the reflector address for the endpoint.
  *          FALSE if endpoint not found.
  */
  extern uint8 afSetReflector( byte EndPoint, uint16 reflectorAddr );

 /*
  *	afDataReqMTU - Get the Data Request MTU(Max Transport Unit)
  */
  extern uint8 afDataReqMTU( afDataReqMTU_t* fields );

 /*
  *	afGetMatch - Get the action for the Match Descriptor Response
  *             TRUE allow match descriptor response
  */
  extern uint8 afGetMatch( uint8 ep );

 /*
  *	afSetMatch - Set the action for the Match Descriptor Response
  *             TRUE allow match descriptor response
  */
  extern uint8 afSetMatch( uint8 ep, uint8 action );

 /*
  *	afNumEndPoints - returns the number of endpoints defined.
  */
  extern byte afNumEndPoints( void );

 /*
  *	afEndPoints - builds an array of endpoints.
  */
  extern void afEndPoints( byte *epBuf, byte skipZDO );

#if ( AF_FLOAT_SUPPORT )
 /*
  * afCnvtSP_uint16 - Converts uint16 to semi-precision structure format.
  */
  extern uint16 afCnvtSP_uint16( afSemiPrecision_t sp );

 /*
  * afCnvtuint16_SP - Converts uint16 to semi-precision structure format.
  */
  extern afSemiPrecision_t afCnvtuint16_SP( uint16 rawSP );

 /*
  * afCnvtFloat_SP - Converts float to semi-precision
  *                    structure format
  * NOTE: This function will convert to the closest possible
  *       representation in a 16 bit format.  Meaning that
  *       the number may not be exact.  For example, 10.7 will
  *       convert to 10.69531, because .7 is a repeating binary
  *       number.  The mantissa for afSemiPrecision_t is 10 bits
  *       and .69531 is the 10 bit representative of .7.
  */
  extern afSemiPrecision_t afCnvtFloat_SP( float f );

 /*
  * afCnvtSP_Float - Converts semi-precision structure format to float.
  */
  extern float afCnvtSP_Float( afSemiPrecision_t sp );
#endif

#if ( AF_KVP_SUPPORT )
 /*
  * GetDataTypeLength
  */
extern byte GetDataTypeLength ( byte DataType);
#endif

 /*
  * afCopyAddress
  *
  */
extern void afCopyAddress (afAddrType_t *afAddr, zAddrType_t *zAddr);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
