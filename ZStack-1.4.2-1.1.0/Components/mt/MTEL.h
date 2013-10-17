#ifndef MTEL_H
#define MTEL_H

/*********************************************************************
    Filename:       MTEL.h
    Revised:        $Date: 2007-05-14 21:25:19 -0700 (Mon, 14 May 2007) $
    Revision:       $Revision: 14297 $

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

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

#include "OSAL.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*
* Definitions to allow conditional compiling -
*   To use these in an embedded environment include them as a compiler
*   option (ex. "-DMT_NWK_FUNC" )
*/

/*** Task Event IDs - bit masks ***/
#define MT_ZTOOL_SERIAL_RCV_CHAR          0x0001
#define MT_ZAPP_SERIAL_RCV_CHAR           0x0002
#define MT_ZTOOL_SERIAL_RCV_BUFFER_FULL   0x0004
#define MT_ZAPP_SERIAL_RCV_BUFFER_FULL    0x0008
#define MT_SERIAL_ZTOOL_XMT_READY         0x0010
#define MT_SERIAL_ZAPP_XMT_READY          0x0020
#define MT_MSG_SEQUENCE_EVT               0x0040
#define MT_KEYPRESS_POLL_EVT              0x0080

/*** Message Command IDs ***/
#define CMD_SERIAL_MSG                  0x01
#define CMD_DEBUG_MSG                   0x02
#define CMD_TX_MSG                      0x03
#define CB_FUNC                         0x04
#define CMD_SEQUENCE_MSG                0x05
#define CMD_DEBUG_STR                   0x06
#define AF_INCOMING_MSG_FOR_MT          0x0F

/*** Error Response IDs ***/
#define UNRECOGNIZED_COMMAND            0x00
#define UNSUPPORTED_COMMAND             0x01
#define RECEIVE_BUFFER_FULL             0x02

/*** Serial Message Command IDs ***/
#define SPI_CMD_SYS_RAM_READ            0x0001
#define SPI_CMD_SYS_RAM_WRITE           0x0002
#define SPI_CMD_SYS_SET_DEBUG_THRESHOLD 0x0003
#define SPI_CMD_DEBUG_MSG               0x4003
#define SPI_CMD_TRACE_SUB               0x0004
#define SPI_CMD_TRACE_MSG               0x4004
#define SPI_CMD_SYS_RESET               0x0005
#define SPI_CMD_SYS_CALLBACK_SUB_CMD    0x0006
#define SPI_CMD_SYS_PING                0x0007
#define SPI_CMD_SYS_VERSION             0x0008

#define SPI_CMD_USER0                   0x000A
#define SPI_CMD_USER1                   0x000B
#define SPI_CMD_USER2                   0x000C
#define SPI_CMD_USER3                   0x000D
#define SPI_CMD_USER4                   0x000E
#define SPI_CMD_USER5                   0x000F

#define SPI_CMD_SYS_SET_EXTADDR         0x0010
#define SPI_CMD_SYS_GET_EXTADDR         0x0011
#define SPI_CMD_SYS_SET_NV              0x0012
#define SPI_CMD_SYS_GET_NV              0x0013
#define SPI_CMD_SYS_GET_DEVICE_INFO     0x0014
#define SPI_SYS_STRING_MSG              0x0015
#define SPI_CMD_SYS_KEY_EVENT           0x0016
#define SPI_CMD_SYS_HEARTBEAT           0x0017
#define SPI_CMD_SYS_APP_MSG             0x0018
#define SPI_CMD_SYS_LED_CONTROL         0x0019
#define SPI_CMD_SYS_TIME_ALIVE          0x001A
#define SPI_CMD_SYS_SET_PANID           0x001B
#define SPI_CMD_SYS_SET_CHANNELS        0x001C
#define SPI_CMD_SYS_SET_SECLEVEL        0x001D
#define SPI_CMD_SYS_SET_PRECFGKEY       0x001E
#define SPI_CMD_SYS_GET_NV_INFO         0x001F
#define SPI_CMD_SYS_NETWORK_START       0x0020

#define SPI_CMD_ZIGNET_DATA             0x0022

/* system command response */
#define SPI_CB_SYS_RESET_RSP            0x1005
#define SPI_CB_SYS_CALLBACK_SUB_RSP     0x1006
#define SPI_CB_SYS_PING_RSP             0x1007
#define SPI_CB_SYS_GET_DEVICE_INFO_RSP  0x1014
#define SPI_CB_SYS_KEY_EVENT_RSP        0x1016
#define SPI_CB_SYS_HEARTBEAT_RSP        0x1017
#define SPI_CB_SYS_LED_CONTROL_RSP      0x1019

#if defined ( MT_AF_FUNC )             //AF commands
#define SPI_CMD_AF_INIT                 0x0900
#define SPI_CMD_AF_REGISTER             0x0901
#define SPI_CMD_AF_SENDMSG              0x0902
#define SPI_RESP_LEN_AF_DEFAULT         0x01
#endif

#if defined ( MT_AF_CB_FUNC )
#define SPI_CB_AF_DATA_IND              0x0903
#endif

#define SPI_CMD_USER_TEST               0x0B51

/*** Message Sequence definitions ***/
#define SPI_CMD_SEQ_START               0x0600
#define SPI_CMD_SEQ_WAIT                0x0601
#define SPI_CMD_SEQ_END                 0x0602
#define SPI_CMD_SEQ_RESET               0x0603
#define DEFAULT_WAIT_INTERVAL           5000      //5 seconds

/*** Serial Message Command Routing Bits ***/
#define SPI_RESPONSE_BIT                0x1000
#define SPI_SUBSCRIPTION_BIT            0x2000
#define SPI_DEBUGTRACE_BIT              0x4000

#define SPI_0DATA_MSG_LEN                5
#define SPI_RESP_MSG_LEN_DEFAULT         6

#define LEN_MAC_BEACON_MSDU             15
#define LEN_MAC_COORDEXTND_ADDR          8
#define LEN_MAC_ATTR_BYTE                1
#define LEN_MAC_ATTR_INT                 2

#define SOP_FIELD                        0
#define CMD_FIELD_HI                     1
#define CMD_FIELD_LO                     2
#define DATALEN_FIELD                    3
#define DATA_BEGIN                       4

//MT PACKET (For Test Tool): FIELD IDENTIFIERS
#define MT_MAC_CB_ID                0
#define MT_OFFSET                   1
#define MT_SOP_FIELD                MT_OFFSET + SOP_FIELD
#define MT_CMD_FIELD_HI             MT_OFFSET + CMD_FIELD_HI
#define MT_CMD_FIELD_LO             MT_OFFSET + CMD_FIELD_LO
#define MT_DATALEN_FIELD            MT_OFFSET + DATALEN_FIELD
#define MT_DATA_BEGIN               MT_OFFSET + DATA_BEGIN

#define MT_INFO_HEADER_LEN         1
#define MT_RAM_READ_RESP_LEN       0x02
#define MT_RAM_WRITE_RESP_LEN      0x01

//Defines for the fields in the AF structures
#define AF_INTERFACE_BITS          0x07
#define AF_INTERFACE_OFFSET        0x05
#define AF_APP_DEV_VER_MASK        0x0F
#define AF_APP_FLAGS_MASK          0x0F
#define AF_TRANSTYPE_MASK          0x0F
#define AF_TRANSDATATYPE_MASK      0x0F

//Defines for the semi-precision structure
#define AF_SEMI_PREC_SIGN          0x8000
#define AF_SEMI_PREC_EXPONENT      0x7C00
#define AF_SEMI_PREC_MANTISSA      0x03FF
#define AF_SEMI_PREC_SIGN_OFFSET   0x0F
#define AF_SEMI_PREC_EXP_OFFSET    0x0A

//Defines for the application commands accessed by MT
#define SRC_CHANGE_STATE            0x0000
#define DRC_TOGGLE_STATE            0x0001
#define DRC_TOGGLE_PRESET           0x0002
#define OS_TOGGLE_STATE             0x0003
#define SLC_RCV_SET_ONOFF           0x0004
#define DLC_RCV_SET_ONOFF           0x0005
#define DLC_RCV_SET_DIMBRIGHT       0x0006
#define DLC_RCV_SET_PRESET          0x0007
#define LSM_TOGGLE_STATE            0x0008

#define DRC_DIMBRIGHT               0x0009

#define TGEN_START									0x000a
#define TGEN_STOP										0x000b
#define TGEN_COUNT									0x000c
#define DEBUG_GET					          0x000d
#define HW_TEST                     0x000e
#define HW_DISPLAY_RESULT						0x000f
#define HW_SEND_STATUS							0x0010

#if defined( APP_TP ) || defined ( APP_TP2 )
#if defined( APP_TP )
#define TP_SEND_NODATA              0x0011
#else
#define TP_SEND_BCAST_RSP           0x0011
#endif
#define TP_SEND_BUFFERTEST          0x0012
#if defined (APP_TP)
#define TP_SEND_UINT8               0x0013
#define TP_SEND_INT8                0x0014
#define TP_SEND_UINT16              0x0015
#define TP_SEND_INT16               0x0016
#define TP_SEND_SEMIPREC            0x0017
#endif
#define TP_SEND_FREEFORM            0x0018
#if defined( APP_TP )
#define TP_SEND_ABS_TIME            0x0019
#define TP_SEND_REL_TIME            0x001A
#define TP_SEND_CHAR_STRING         0x001B
#define TP_SEND_OCTET_STRING        0x001C
#endif
#define TP_SET_DSTADDRESS           0x001D
#if defined( APP_TP2 )
#define TP_SEND_BUFFER_GROUP        0x001E
#endif
#define TP_SEND_BUFFER              0x001F
#if defined( APP_TP )
#define TP_SEND_CON_INT8						0x0020
#define TP_SEND_CON_INT16						0x0021
#define TP_SEND_CON_TIME						0x0022

#define TP_SEND_MULT_KVP_8BIT       0x0023
#define TP_SEND_MULT_KVP_16BIT      0x0024
#define TP_SEND_MULT_KVP_TIME       0x0025
#define TP_SEND_MULT_KVP_STRING     0x0026
#endif

#define TP_SEND_COUNTED_PKTS        0x0027
#define TP_SEND_RESET_COUNTER       0x0028
#define TP_SEND_GET_COUNTER         0x0029

#if defined( APP_TP )
#define TP_SEND_MULTI_KVP_STR_TIME  0x0030
#endif

#define TP_SET_PERMIT_JOIN          0x0040

#define TP_ADD_GROUP                0x0041
#define TP_REMOVE_GROUP             0x0042

#define TP_SEND_UPDATE_KEY          0x0043
#define TP_SEND_SWITCH_KEY          0x0044

#if defined( APP_TP2 )
#define TP_SEND_BUFFERTEST_GROUP    0x0045
#define TP_SEND_ROUTE_DISC_REQ      0x0046
#define TP_SEND_ROUTE_DISCOVERY     0x0047
#endif

#endif

#if defined ( OSAL_TOTAL_MEM )
  #define OSAL_MEM_STACK_HIGH_WATER   0x0100
  #define OSAL_MEM_HEAP_HIGH_WATER    0x0101
#endif

//Special definitions for ZTOOL (Zigbee 0.7 release)
#define ZTEST_DEFAULT_PARAM_LEN    0x10      //( 16 Bytes)
#define ZTEST_DEFAULT_ADDR_LEN     0x08      //(  8 Bytes)
#define ZTEST_DEFAULT_DATA_LEN     0x66      //(102 Bytes) - MAC,NWK
#define ZTEST_DEFAULT_AF_DATA_LEN  0x20      //( 32 Bytes) - AF
#define ZTEST_DEFAULT_SEC_LEN      0x0B

// Capabilities - PING Response
#if defined ( MT_MAC_FUNC )
  #define MT_CAP_MAC    0x0001
#else
  #define MT_CAP_MAC    0x0000
#endif

#if defined ( MT_NWK_FUNC )
  #define MT_CAP_NWK    0x0002
#else
  #define MT_CAP_NWK    0x0000
#endif

#if defined ( MT_AF_FUNC )
  #define MT_CAP_AF     0x0004
#else
  #define MT_CAP_AF     0x0000
#endif

#if defined ( MT_ZDO_FUNC )
  #define MT_CAP_ZDO    0x0008
#else
  #define MT_CAP_ZDO    0x0000
#endif

#if defined ( MT_USER_TEST_FUNC )
  #define MT_CAP_USER_TEST 0x0010
#else
  #define MT_CAP_USER_TEST 0x0000
#endif

#if defined ( MT_SAPI_FUNC )
  #define MT_CAP_SAPI_FUNC 0x0020
#else
  #define MT_CAP_SAPI_FUNC 0x0000
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*  This is a temporary data structure we will remove when we fully define the APS messages */
typedef struct {
  unsigned char		TransID;
  unsigned char   TransType_Err;
  unsigned char		Endpoint;
  unsigned char		TransObjAttID;
  unsigned char   TransData[16];
} af_cmd_format_t;

typedef struct {
  uint16 waitInterval;
  byte *msg;
  void *next;
} MT_msg_queue_t;

typedef struct
{
  osal_event_hdr_t  hdr;
  uint8             endpoint;
  uint8             appDataLen;
  uint8             *appData;
} mtSysAppMsg_t;

typedef struct
{
  osal_event_hdr_t  hdr;
  uint8             compID;
  uint8             severity;
  uint8             numParams;
  uint16            param1;
  uint16            param2;
  uint16            param3;
  uint16            timestamp;
} mtDebugMsg_t;

typedef struct
{
  osal_event_hdr_t  hdr;
  uint8             sln; // String length
  uint8             *pString;
} mtDebugStr_t;

typedef struct
{
  osal_event_hdr_t  hdr;
  uint8             *msg;
} mtOSALSerialData_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern byte MT_TaskID;
extern byte debugThreshold;
extern byte debugCompId;

extern byte queueMsgs;
extern MT_msg_queue_t *_pMtMsgQueue;
extern MT_msg_queue_t *_pLastInQueue;
extern MT_msg_queue_t *_pCurQueueElem;

//DEBUG
extern uint32 longvar;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * MonitorTest Task Initialization
 */
extern void MT_TaskInit( byte task_id );

/*
 * MonitorTest Event Processor
 */
extern UINT16 MT_ProcessEvent( byte task_id, UINT16 event );

/*
 * Build and send a ZTool response message
 */
extern void MT_BuildAndSendZToolResponse( byte msgLen, uint16 cmd,
                                    byte dataLen, byte *dataPtr );

/*
 * Build and send a ZTool callback message
 */
extern void MT_BuildAndSendZToolCB( uint16 callbackID, byte len, byte *pData );

/*
 * MonitorTest Format an SPI Message
 */
extern void MT_BuildSPIMsg( UINT16 cmd, byte *msg, byte dataLen, byte *dataPtr );

/*
 * Temp test function
 */
extern void MT_ProcessUserCmd( byte cmd );

/*
 * MonitorTest function handling PhY commands
 */
extern void MT_RadioCommandProcessing( uint16 cmd_id , byte len , byte *pData );

/*
 * MonitorTest function handling PhY commands
 */
extern void MT_PhyCommandProcessing( uint16 cmd_id , byte len , byte *pData );

/*
 * MonitorTest function handling MAC commands
 */
extern void MT_MacCommandProcessing( uint16 cmd_id , byte len , byte *pData );

/*
 * MonitorTest function handling NWK commands
 */
extern void MT_NwkCommandProcessing( uint16 cmd_id , byte len , byte *pData );

/*
 * MonitorTest function handling MT command responses
 */
extern void MT_SendSPIRespMsg( byte ret, uint16 cmd_id, byte msgLen, byte respLen);

/*
 * MonitorTest function to reverse bytes in a buffer
 */
extern void MT_ReverseBytes( byte *pData, byte len );

/*
 * Reset indication
 */
#if defined ( ZTOOL_PORT ) || defined ( ZAPP_P1 ) || defined ( ZAPP_P2 )
extern void MT_IndReset( void );
#endif

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MTEL_H */
