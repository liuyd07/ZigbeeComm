#ifndef ZPZLOAD_APP_H
#define ZPZLOAD_APP_H

/*********************************************************************
    Filename:       ZLOAD_App.h
    Revised:        $Date: 2006-04-06 08:19:08 -0700 (Thu, 06 Apr 2006) $
    Revision:       $Revision: 10396 $

    Description:

         ZLOAD header declaring prototypes and specifying the
         protocol packet formats.

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

/*********************************************************************
 * CONSTANTS
 */
#define ZLOADAPP_ENDPOINT              (200)

#define ZLOADAPP_MAX_CLUSTERS          1
#define ZLOADAPP_DEVICEID              0x0001
#define ZLOADAPP_DEVICE_VERSION        0
#define ZLOADAPP_FLAGS                 0


#define ZLOADAPP_MSG_TIMER_EVT         0x0001


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the ZLOAD Application
 */
extern void ZLOADApp_Init( byte task_id );

/*
 * Task Event Processor for the ZLOAD Application
 */
extern UINT16 ZLOADApp_ProcessEvent( byte task_id, UINT16 events );

// ************ MESSAGE FORMATS ***************

// message ID defines
#define  ZLMSGID_STATUSQ        ((uint8) 0x01)
#define  ZLMSGID_SESSION_START  ((uint8) 0x02)
#define  ZLMSGID_SESSION_TERM   ((uint8) 0x03)
#define  ZLMSGID_CLIENT_CMD     ((uint8) 0x04)
#define  ZLMSGID_CODE_ENABLE    ((uint8) 0x05)
#define  ZLMSGID_SEND_DATA      ((uint8) 0x06)
#define  ZLMSGID_RESET          ((uint8) 0x07)
#define  ZLMSGID_RESET_BOARD    ((uint8) 0x3F)  // secret command - undocumented
// bit OR'ed into message ID to signify a reply to that message
#define  ZLMSGID_REPLY_BIT      ((uint8) 0x80)

// for now, the block size and number of blocks per data transfer tansaction is fixed.
#define  ZL_DATA_BLK_SIZE       (8)
#define  ZL_NUM_DATA_BLKS       (4)

// capabilties bits. none for now.
#define ZLOAD_CAPABILTIES       (0)
#define ZLOAD_PROTOCOL_VERSION  (1)

// message header
typedef struct  {
    uint8  zlhdr_msgid;
    uint8  zlhdr_seqnum;
    uint8  zlhdr_msglen;
} zlmhdr_t;

// Z-Architect headers
// inbound to host (from dongle directly or external platform)
typedef struct  {
    uint16 zaproxy_nwkAddr;
    uint8  zaproxy_endp;
    uint16 zaproxy_ClusterID;
    uint8  zaproxy_msglen;
    uint8  zaproxy_payload[1];
} zahdrin_t;

// outbound from host (to dongle directly or external platform)
typedef struct  {
    uint16 zaproxy_nwkAddr;
    uint8  zaproxy_endp;
    uint16 zaproxy_ClusterID;
    uint8  zaproxy_msglen;
    uint8  zaproxy_payload[1];
} zahdrout_t;

//   STATUS QUERY
//       Command:  no payload with status query

//       Reply
typedef struct  {
    uint8  zlsqR_state;
    uint8  zlsqR_errorCode;
    uint8  zlsqR_ProtocolVersion;
    uint8  zlsqR_capabilties;
    uint16 zlsqR_opVer;
    uint16 zlsqR_opManu;
    uint16 zlsqR_opProd;
    uint16 zlsqR_dlVer;
    uint16 zlsqR_dlManu;
    uint16 zlsqR_dlProd;
    uint16 zlsqR_curPkt;
    uint16 zlsqR_totPkt;
} zlstatusR_t;

//   SESSION START
//       Command
typedef struct  {
    uint16 zlbsC_ver;
    uint16 zlbsC_manu;
    uint16 zlbsC_prod;
    uint8  zlbsC_sessionID;
} zlbegsessC_t;

//       Reply
typedef struct  {
    uint8  zlbsR_state;
    uint8  zlbsR_errorCode;
    uint32 zlbsR_imgLen;
    uint8  zlbsR_blkSize;
    uint8  zlbsR_numBlks;
    uint8  zlbsR_preambleOffset;
} zlbegsessR_t;

//   SESSION TERMINATE
//       Command
typedef struct  {
    uint8  zlesC_sessionID;
} zlendsessC_t;

//       Reply
typedef struct  {
    uint8  zlesR_state;
    uint8  zlesR_errorCode;
} zlendsessR_t;

//   CLIENT COMMAND
//       Command
typedef struct  {
    uint16 zlclC_ver;
    uint16 zlclC_manu;
    uint16 zlclC_prod;
    uint8  zlclC_IEEE[Z_EXTADDR_LEN];
    uint16 zlclC_nwk;
    uint8  zlclC_endp;
} zlclientC_t;

//       Reply
typedef struct  {
    uint8  zlclR_state;
    uint8  zlclR_errorCode;
} zlclientR_t;

//   CODE ENABLE COMMAND
//       Command
typedef struct  {
    uint16 zlceC_ver;
    uint16 zlceC_manu;
    uint16 zlceC_prod;
} zlceC_t;

//       Reply
typedef struct  {
    uint8  zlceR_state;
    uint8  zlceR_errorCode;
} zlceR_t;

//   SEND DATA COMMAND
//       Command
typedef struct  {
    uint16 zlsdC_pktNum;
    uint8  zlsdC_sessionID;
} zlsdC_t;

//       Reply
typedef struct  {
    uint8  zlsdR_state;
    uint8  zlsdR_errorCode;
    uint16 zlsdR_pktNum;
    uint8  zlsdR_data[ZL_DATA_BLK_SIZE * ZL_NUM_DATA_BLKS];
} zlsdR_t;

//   RESET COMMAND
//       Command:  no payload with status query

//       Reply
typedef struct  {
    uint8  zlrstR_state;
    uint8  zlrstR_errorCode;
} zlrstR_t;


// the replies (except for send data) are all small. use a union so we can have a single
// malloc() and save code space.
typedef union  {
    zlstatusR_t  statusq;
    zlbegsessR_t begSess;
    zlendsessR_t endSess;
    zlclientR_t  client;
    zlceR_t      enable;
    zlrstR_t     reset;
} zlreply_t;


// event message bits
#define  ZLOAD_CODE_ENABLE_EVT  (0x0001)
#define  ZLOAD_IS_CLIENT_EVT    (0x0002)
#define  ZLOAD_XFER_DONE_EVT    (0x0004)
#define  ZLOAD_RESET_EVT        (0x0008)
#define  ZLOAD_SDRTIMER_EVT     (0x0010)
#define  ZLOAD_RESET_BOARD_EVT  (0x0020)

/*********************************************************************
*********************************************************************/

// STATE definitions for state machine
#define ZLSTATE_IDLE               (1)
#define ZLSTATE_CLIENT             (2)
//#define ZLSTATE_SERVER             (3)
#define ZLSTATE_PASS_THROUGH       (4)
#define ZLSTATE_SUBSTATE_NONE      (1)
#define ZLSTATE_SUBSTATE_XFER_DONE (2)


// Error Codes for replies to commands.
// Status Query and Reset always return No Error

#define EC_NO_ERROR        ((uint8) 0x00)

// Begin Session
#define EC_BS_NOT_IDLE     ((uint8) 0x21)
#define EC_BS_NO_MATCHES   ((uint8) 0x22)
#define EC_BS_NO_MEM       ((uint8) 0x23)
#define EC_BS_NOT_SERVER   ((uint8) 0x24)
// End Session
#define EC_ES_BAD_SESS_ID  ((uint8) 0x31)
#define EC_ES_NOT_SERVER   ((uint8) 0x32)
#define EC_ES_NO_MEM       ((uint8) 0x33)
// Client
#define EC_CL_NOT_IDLE     ((uint8) 0x41)
#define EC_CL_NO_MEM       ((uint8) 0x42)
#define EC_CL_NOT_CLIENT   ((uint8) 0x43)
// Code enable
#define EC_CE_NO_IMAGE     ((uint8) 0x51)
#define EC_CE_NO_MATCH     ((uint8) 0x52)
#define EC_CE_NOT_IDLE     ((uint8) 0x53)
#define EC_CE_IMAGE_INSANE ((uint8) 0x54)
#define EC_CE_NOT_CLIENT   ((uint8) 0x55)
// Send data
#define EC_SD_NOT_SERVER   ((uint8) 0x61)
#define EC_SD_BAD_SESS_ID  ((uint8) 0x62)
#define EC_SD_BAD_PKT_NUM  ((uint8) 0x63)
#define EC_SD_NO_BEG_SESS  ((uint8) 0x64)
#define EC_SD_NOT_CLIENT   ((uint8) 0x65)
#define EC_SD_NO_MEM       ((uint8) 0x66)

// other support
#define  PREAMBLE_DL           ((uint8) 1)
#define  PREAMBLE_OP           ((uint8) 2)
#define  ZL_IMAGE_ID_LENGTH    (6)
#define  SERIAL_SERVER_ADDRESS (0xFFFE)

#ifdef __cplusplus
}
#endif

#endif /* ZLOAD_APP_H */