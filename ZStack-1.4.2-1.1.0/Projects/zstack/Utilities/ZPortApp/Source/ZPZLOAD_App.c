#include "OSAL.h"
#include "NLMEDE.h"
#include "AF.h"
#include "ZDApp.h"
#include "OverAirDownload.h"
#include "OADSupport.h"
#include "ZDProfile.h"

#if !defined(MT_TASK)
#error MT_TASK must be defined for Dongle support
#endif

#ifndef CC2420DB
#define  CC2430_FLASH_PAGE_SIZE   0x800
#if (TOP_BOOT_PAGES<1)
#error CC2430 must have at least 1 flash page at the top devoted to boot code
#endif
#endif

#include "ZMAC.h"

#include "ZPZLOAD_App.h"
#include "NLMEDE.h"

#include "OnBoard.h"
#include "FlashUtils.h"

#include <string.h>

#define STATIC static

// these sizes do _not_ include the payload uint8 array. this element is included in the
// definition just for convenience so that the copy call can use an address reference
//                           dest addr        dest ep        cluster id        msg len
#define SIZEOF_ZAOUT_HDR  (sizeof(uint16) + sizeof(uint8) + sizeof(uint16) + sizeof(uint8))
//                           src addr         src ep         cluster id        msg len
#define SIZEOF_ZAIN_HDR   (sizeof(uint16) + sizeof(uint8) + sizeof(uint16) + sizeof(uint8))

// CUSTOMER DEFINITIONS GO HERE
#include "preamble.h"

// END CUSTOMER DEFINITIONS

// this sets up the code preamble in the proper segment as defined
// in the .xcl file

#pragma diag_suppress=Pe177
#ifdef __ICC8051__
STATIC const __code __root  preamble_t s_preamble @ "PREAMBLE" =
    {
        PREAMBLE_MAGIC1,
        PREAMBLE_MAGIC2,
        0,
        IMAGE_VERSION,
        MANUFACTURER_ID,
        PRODUCT_ID
    };

STATIC const __code __root  char s_date[] @ "PREAMBLE" = __DATE__ ;
STATIC const __code __root  char s_time[] @ "PREAMBLE" = __TIME__;
#else
STATIC const __root __farflash preamble_t s_preamble @ "PREAMBLE" =
    {
        PREAMBLE_MAGIC1,
        PREAMBLE_MAGIC2,
        0,
        IMAGE_VERSION,
        MANUFACTURER_ID,
        PRODUCT_ID
    };

STATIC const __root __farflash char s_date[] @ "PREAMBLE" = __DATE__ ;
STATIC const __root __farflash char s_time[] @ "PREAMBLE" = __TIME__;
#endif
#pragma diag_default=Pe177

/*********************************************************************
    Filename:       ZLOAD_App.c
    Revised:        $Date: 2007-05-30 14:20:48 -0700 (Wed, 30 May 2007) $
    Revision:       $Revision: 14464 $

    Description:

      This file contains the interface to the ZLOAD Over Air Download
      application.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

STATIC zlclientC_t  s_clientInfo;
STATIC uint8        s_State, s_SubState, *s_imgBuf, s_offset, s_DLPreambleOffset, s_blkSize;
STATIC uint16       s_NextPacket, s_NumPktGet, s_SDCRetryCount, s_lastSeqNum, s_clientSessionID;
STATIC uint16       s_myNwkAddr = 0xFFFE;
STATIC uint32       s_XferBaseAddr, s_DLImageBase, s_DLFirstPageAddress, s_wrapAddress;
STATIC zlmhdr_t    *s_sdcmd;
STATIC zlsdC_t     *s_sdcpayload;
STATIC uint8        s_serialMsg, *s_replyBuf;

// pass through stuff
STATIC uint8                  s_PTSessNum, s_PTSeqNum;
STATIC afIncomingMSGPacket_t  s_PTClientInfo;
/*********************************************************************
 * MACROS
 */

// Atmega128-Specific:
#define  PAGESIZE                (0x100)
#define  FLASHSIZE               (0x20000)

#define  SDC_RETRY_COUNT         (5)
#define  SDR_WAIT_TO             (1000)

#define  GET_NUM_PAGES(num)    ((num+PAGESIZE-1)>>8)

/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const uint16 ZLOADAPP_ClusterList[ZLOADAPP_MAX_CLUSTERS] =
{
  ZLOADAPP_CLUSTERID_OADMESG  // MSG Cluster ID
};

const SimpleDescriptionFormat_t ZLOADApp_SimpleDesc =
{
  ZLOADAPP_ENDPOINT,              //  int    Endpoint;
  OAD_PROFILE_ID,                 //  uint16 AppProfId[2];
  ZLOADAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  ZLOADAPP_DEVICE_VERSION,        //  int    AppDevVer:4;
  ZLOADAPP_FLAGS,                 //  int    AppFlags:4;
  ZLOADAPP_MAX_CLUSTERS,          //  byte   AppNumInClusters;
  (uint16*)ZLOADAPP_ClusterList,    //  byte   *pAppInClusterList;
  0,                              //  byte   AppNumOutClusters;
  (uint16*)0                        //  byte   *pAppOutClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in ZLOADApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t ZLOADApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

// set up the mailbox in RAM
#ifndef CC2420DB
mbox_t mbox;
#else
__no_init mbox_t mbox @ 0x10EE;
#endif
/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
extern void MTProcessAppRspMsg(uint8 *, uint8);
//extern void FlashInit(void);

/*********************************************************************
 * LOCAL VARIABLES
 */
byte ZLOADApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // ZLOADApp_Init() is called.

devStates_t ZLOADApp_NwkState;


byte ZLOADApp_TransID;  // This is the unique message ID (counter)

afAddrType_t ZLOADApp_DstAddr;

byte ZLOADApp_State;

#define ZLOAD_APPSTATE_IDLE   0

/*********************************************************************
 * LOCAL FUNCTIONS
 */
STATIC void   ZLOADApp_MessageMSGCB( afIncomingMSGPacket_t * );
STATIC void   ZLOADApp_handleCommand(afIncomingMSGPacket_t *, zlmhdr_t *);
STATIC void   ZLOADApp_handleReply(afIncomingMSGPacket_t *, zlmhdr_t *);
STATIC uint8  zlgetPreamble(image_t, preamble_t *);
STATIC void   zlCodeEnable(void);
STATIC void   zlCleanupOnReset(void);
STATIC void   zlStartClientSession(void);
STATIC void   zlProcessSDR(zlsdR_t *);
STATIC void   zlRequestNextDataPacket(void);
STATIC void   zlCleanupOnXferDone(void);
STATIC uint8  zlGetStartFlashAddress(uint32, uint32 *, uint32 *);
STATIC uint8  zlIsDLImageSane(void);
STATIC uint8  zlIsWriteFlashOK(void);
STATIC void   zlResendSDC(void);
STATIC void   zlResetBoard(void);

STATIC void   zlSendSerial(uint8 *, uint8);
STATIC void   ZLOADApp_SerialMessageMSGCB(zahdrout_t *);
STATIC void   zlZArchitectProxyMsg(zahdrout_t *);

STATIC uint8      zlPassOnStartSessionOK(uint8 *);
STATIC zahdrin_t *zlBuildExternalInboundSerialMSG(afIncomingMSGPacket_t *, uint8 *, uint8);
STATIC zahdrin_t *zlBuildInternalInboundSerialMSG(uint8 *, uint8);

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */



/*********************************************************************
 * @fn      ZLOADApp_Init
 *
 * @brief   Initialization function for the ZLOAD Task.
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
void ZLOADApp_Init( byte task_id )
{
  ZLOADApp_TaskID   = task_id;
  ZLOADApp_NwkState = DEV_INIT;
  ZLOADApp_TransID  = 0;

  ZLOADApp_State = ZLOAD_APPSTATE_IDLE;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  ZLOADApp_DstAddr.addrMode       = afAddrNotPresent;
  ZLOADApp_DstAddr.endPoint       = 0;
  ZLOADApp_DstAddr.addr.shortAddr = 0;

  // Fill out the end point description.
  ZLOADApp_epDesc.endPoint   = ZLOADAPP_ENDPOINT;
  ZLOADApp_epDesc.task_id    = &ZLOADApp_TaskID;
  ZLOADApp_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&ZLOADApp_SimpleDesc;
  ZLOADApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint/interface description with the AF
  afRegister( &ZLOADApp_epDesc );

    s_State         = ZLSTATE_IDLE;
    s_SubState      = ZLSTATE_SUBSTATE_NONE;
    mboxMsg.BootRead = 0;

    FlashInit();
}

/*********************************************************************
 * @fn      ZLOADApp_ProcessEvent
 *
 * @brief   ZLOAD Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
UINT16 ZLOADApp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  byte *msgPtr;
#if 0
  byte dstEP;
  zAddrType_t *dstAddr;
#endif

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ZLOADApp_TaskID );
    while ( MSGpkt )
    {

      s_serialMsg = 0;

      switch ( MSGpkt->hdr.event )
      {
        case KEY_CHANGE:
          break;

        case AF_DATA_CONFIRM_CMD:
          // Action taken when confirmation is received: none
          // none on errors. right now we have a stop-and-wait protocl in
          // a command-response paradigm so the application can detect a non-response.
          break;

        // this is how we get messages from a Host application (ZArchitect)
        case MT_SYS_APP_MSG:
        case MT_SYS_APP_RSP_MSG:
          s_serialMsg = 1;
          msgPtr = ((mtSysAppMsg_t *)MSGpkt)->appData;
          ZLOADApp_SerialMessageMSGCB( (zahdrout_t *)msgPtr );
          break;

        case AF_INCOMING_MSG_CMD:
          ZLOADApp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_NEW_DSTADDR:
// never get here. using the ZLOADApp_DstAddr to keep the Server address info
#if 0
          ZDO_NewDstAddr_t *ZDO_NewDstAddr = (ZDO_NewDstAddr_t *)MSGpkt;

          dstEP = ZDO_NewDstAddr->dstAddrDstEP;
          dstAddr = &ZDO_NewDstAddr->dstAddr;

          ZLOADApp_DstAddr.addrMode = dstAddr->addrMode;
          ZLOADApp_DstAddr.endPoint = dstEP;
          if ( dstAddr->addrMode == afAddr16Bit )
            ZLOADApp_DstAddr.addr.shortAddr = dstAddr->addr.shortAddr;
          else
          {
            osal_memcpy( ZLOADApp_DstAddr.addr.extAddr,
                          dstAddr->addr.extAddr, Z_EXTADDR_LEN );
          }
#endif
          break;

        case ZDO_STATE_CHANGE:
          ZLOADApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ZLOADApp_TaskID );
    }

    // Return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if (events & ZLOAD_CODE_ENABLE_EVT)
  {
    zlCodeEnable();

    // Return unprocessed events
    return (events ^ ZLOAD_CODE_ENABLE_EVT);
  }

  if (events & ZLOAD_IS_CLIENT_EVT)
  {
    zlStartClientSession();

    // Return unprocessed events
    return (events ^ ZLOAD_IS_CLIENT_EVT);
  }

  if (events & ZLOAD_XFER_DONE_EVT)
  {
    zlCleanupOnXferDone();

    // Return unprocessed events
    return (events ^ ZLOAD_XFER_DONE_EVT);
  }

  if (events & ZLOAD_RESET_EVT)
  {
    zlCleanupOnReset();

    // Return unprocessed events
    return (events ^ ZLOAD_RESET_EVT);
  }

  if (events & ZLOAD_SDRTIMER_EVT)
  {
    zlResendSDC();

    // Return unprocessed events
    return (events ^ ZLOAD_SDRTIMER_EVT);
  }

  if (events & ZLOAD_RESET_BOARD_EVT)
  {
    zlResetBoard();

    // Return unprocessed events
    return (events ^ ZLOAD_RESET_BOARD_EVT);
  }

  // Discard unknown events
  return 0;
}


/*********************************************************************
 * @fn      ZLOADApp_SerialMSGCB
 *
 * @brief   Message from Host application. It's a command, a reply,
 *          or a proxy command. Send it on appropriately
 *
 * @param   input
 *            inMsg  pointer to incoming message
 *
 * @return  none
 */
STATIC void ZLOADApp_SerialMessageMSGCB(zahdrout_t *zaout)
{
    zlmhdr_t *zlhdr = (zlmhdr_t *)zaout->zaproxy_payload;

    // is this for me?
    if (zaout->zaproxy_nwkAddr == s_myNwkAddr)  {
      // is this a reply?
      if (zlhdr->zlhdr_msgid & ZLMSGID_REPLY_BIT)  {
          ZLOADApp_handleReply(NULL, zlhdr);
      }
      else  {
          ZLOADApp_handleCommand(NULL, zlhdr);
      }
    }
    else  {
        // it's a proxy command
        zlZArchitectProxyMsg(zaout);
    }
    return;
}

/*********************************************************************
 * @fn      zlSendSerial
 *
 * @brief   Prepare message outgoing over serial port.
 *          malloc and copy to add serial encapsulation.
 *
 * @param   input
 *            buf  pointer to ougoing message buffer
 *            len  length of message
 *
 * @return  none
 */
STATIC void zlSendSerial(uint8 *buf, uint8 len)
{
    uint8 *nbuf;

    if ((nbuf=osal_mem_alloc(len+1)))  {
        osal_memcpy(nbuf+1, buf, len);
        *nbuf = ZLOADAPP_ENDPOINT;
#if !defined(ZTOOL_SUPPORT)
        MTProcessAppRspMsg(nbuf, len + 1);
#else
        // ZTool expects an 81 byte message
        MTProcessAppRspMsg(nbuf, 81);
#endif
        osal_mem_free(nbuf);
    }
    return;
}

/****************************************************************************
 * @fn      zlZArchitectProxyCommand
 *
 * @brief   Send command to target device on behalf of ZArchitect Host App
 *
 * @param   input
 *            info  pointer to ZArchitect proxy message
 *
 * @return  none
 */
STATIC void zlZArchitectProxyMsg(zahdrout_t *info)
{
    afAddrType_t taddr;
    zlmhdr_t     *zlhdr = (zlmhdr_t *)info->zaproxy_payload;

    // only certain outgoing messages are valid proxy commands
    switch (zlhdr->zlhdr_msgid)  {
        case ZLMSGID_SESSION_START:
        case ZLMSGID_SESSION_TERM:
        case ZLMSGID_SEND_DATA:
            return;
    }

    // fill in address info
    taddr.addrMode       = afAddr16Bit;
    taddr.endPoint       = info->zaproxy_endp;
    taddr.addr.shortAddr = info->zaproxy_nwkAddr;

    AF_DataRequest( &taddr,
                     afFindEndPointDesc( info->zaproxy_endp ),
                     info->zaproxy_ClusterID,
                     info->zaproxy_msglen, info->zaproxy_payload,
                    &ZLOADApp_TransID,
                     NULL, DEF_NWK_RADIUS );
}

/**********************************************************************************
 * @fn      ZLOADApp_MessageMSGCB
 *
 * @brief   Handle a normal ZLOAD command or reply. Forward it to proper handler
 *
 * @param   input
 *            MSGpkt  pointer to incoming AF packet
 *
 * @return  none
 */
STATIC void ZLOADApp_MessageMSGCB(afIncomingMSGPacket_t *MSGpkt)
{
    zlmhdr_t *inMsg = (zlmhdr_t *)MSGpkt->cmd.Data;

    if (inMsg->zlhdr_msgid & ZLMSGID_REPLY_BIT)  {
        ZLOADApp_handleReply(MSGpkt, inMsg);
    }
    else  {
        ZLOADApp_handleCommand(MSGpkt, inMsg);
    }
    return;
}

/**********************************************************************************
 * @fn      ZLOADApp_handleCommand
 *
 * @brief   Handle a normal ZLOAD command
 *
 * @param   input
 *            MSGpkt  pointer to incoming AF packet. needed to get
 *                    reply address info
 *            msg     pointer to ZLOAD message
 *
 * @return  none
 */
STATIC void ZLOADApp_handleCommand(afIncomingMSGPacket_t *MSGpkt, zlmhdr_t *msg)
{
    uint8      *cpc, *cpr, paylSize;
    preamble_t preamble;

    if (!s_replyBuf)  {
        if (!(s_replyBuf = osal_mem_alloc(sizeof(zlmhdr_t) + sizeof(zlreply_t))))  {
            return;
        }
    }

    cpr = s_replyBuf + sizeof(zlmhdr_t);            // offset of reply payload
    cpc = (uint8 *)msg + sizeof(zlmhdr_t);   // offset of command payload

    switch (msg->zlhdr_msgid)  {
        case ZLMSGID_STATUSQ:
            {
                zlstatusR_t *reply = (zlstatusR_t *)cpr;

                // report capabiltities and version
                reply->zlsqR_ProtocolVersion = ZLOAD_PROTOCOL_VERSION;
                reply->zlsqR_capabilties     = ZLOAD_CAPABILTIES;

                // populate the operational version values
                zlgetPreamble(IMAGE_ACTIVE, &preamble);
                reply->zlsqR_opVer  = preamble.pre_ImgVersion;
                reply->zlsqR_opManu = preamble.pre_ManuID;
                reply->zlsqR_opProd = preamble.pre_ProductID;

                // do downloaded image if it's there
                if (!zlgetPreamble(IMAGE_DL, &preamble))  {
                    reply->zlsqR_dlVer  = preamble.pre_ImgVersion;
                    reply->zlsqR_dlManu = preamble.pre_ManuID;
                    reply->zlsqR_dlProd = preamble.pre_ProductID;
                }
                else  {
                    reply->zlsqR_dlVer  = 0;
                    reply->zlsqR_dlManu = 0;
                    reply->zlsqR_dlProd = 0;
                }
                reply->zlsqR_state     = s_State;
                reply->zlsqR_errorCode = EC_NO_ERROR;
                reply->zlsqR_curPkt    = s_NextPacket;
                reply->zlsqR_totPkt    = s_NumPktGet;

                paylSize = sizeof(zlstatusR_t);
            }

            break;

        case ZLMSGID_SESSION_START:
            // the pass through condition starts here, when a client tries
            // to begin a session with the dongle.
            if (s_State != ZLSTATE_IDLE)  {
                ((zlbegsessR_t *)cpr)->zlbsR_errorCode = EC_BS_NOT_IDLE;
            }
            else if (s_serialMsg)  {
                // somehow this came over the serial port and it isn't legal
                ((zlbegsessR_t *)cpr)->zlbsR_errorCode = EC_BS_NOT_SERVER;
            }
            else  {
              // set mode. delay reply until we hear back from the Host
              s_PTSeqNum     = msg->zlhdr_seqnum;
              s_PTClientInfo = *MSGpkt;
              s_PTSessNum    = ((zlbegsessC_t *)cpc)->zlbsC_sessionID;
              if (zlPassOnStartSessionOK((uint8 *)msg))  {
                  s_State = ZLSTATE_PASS_THROUGH;
                  return;
              }
              else  {
                ((zlbegsessR_t *)cpr)->zlbsR_errorCode = EC_BS_NO_MEM;
              }
            }
            break;

        case ZLMSGID_SESSION_TERM:
            // again, only legal in pass through mode. The dongle will never
            // be a server except under the pass through condition
            {
                zlendsessR_t *reply = (zlendsessR_t *)cpr;

                reply->zlesR_state     = s_State;
                reply->zlesR_errorCode = EC_NO_ERROR;

                // guard against terminating the wrong session
                if (((zlendsessC_t *)cpc)->zlesC_sessionID != s_PTSessNum) {
                    // wrong session ID
                    reply->zlesR_errorCode = EC_ES_BAD_SESS_ID;
                }
                else if (s_serialMsg)  {
                    // somehow this came over the serial port and it isn't legal
                    ((zlbegsessR_t *)cpr)->zlbsR_errorCode = EC_ES_NOT_SERVER;
                }
                else  {
                    if (ZLSTATE_PASS_THROUGH == s_State)  {
                        // everything is OK. pass this up to host if we're in pass-through mode
                        // forward message to host
                        zahdrin_t *zain = zlBuildExternalInboundSerialMSG(&s_PTClientInfo, (uint8 *)msg, sizeof(zlmhdr_t) + sizeof(zlendsessC_t));

                        // we're done.
                        osal_set_event(ZLOADApp_TaskID, ZLOAD_XFER_DONE_EVT);

                        if (zain)  {
                            // we're done.
                            osal_set_event(ZLOADApp_TaskID, ZLOAD_XFER_DONE_EVT);

                            s_PTClientInfo = *MSGpkt;
                            zlSendSerial((uint8 *)zain, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + sizeof(zlendsessC_t));
                            osal_mem_free(zain);
                            return;
                        }
                        else  {
                            reply->zlesR_errorCode = EC_ES_NO_MEM;
                        }
                    }
                    else  {
                      // right session ID but I'm not in pass-through mode
                      reply->zlesR_errorCode = EC_ES_NOT_SERVER;
                    }
                }
            }
            paylSize = sizeof(zlendsessR_t);
            break;

        case ZLMSGID_CLIENT_CMD:
            // we can be the client only if the dongle code itself is being updated
            // and then only if the Host is the server
            {
                zlclientR_t *reply = (zlclientR_t *)cpr;

                reply->zlclR_errorCode = EC_NO_ERROR;
                paylSize               = sizeof(zlclientR_t);
                reply->zlclR_state     = s_State;

                if (!s_serialMsg)  {
                    reply->zlclR_errorCode = EC_CL_NOT_CLIENT;
                }
                else if (s_State != ZLSTATE_IDLE)  {
                    reply->zlclR_errorCode = EC_CL_NOT_IDLE;
                }
                else  {
                    // need memory for holding flash page, client info, and current send-data cmd
                    // do one alloc and set up the pointers
                    s_imgBuf = osal_mem_alloc(PAGESIZE+sizeof(zlclientC_t)+sizeof(zlmhdr_t)+sizeof(zlsdC_t));
                    if (!s_imgBuf)  {
                        // no more memory. let Commissioner decide what to do. maybe retry later.
                        reply->zlclR_errorCode = EC_CL_NO_MEM;
                        break;
                    }
                    else  {
                        // set up other pointers
                        s_sdcmd       = (zlmhdr_t *)(s_imgBuf+PAGESIZE);
                        s_sdcpayload  = (zlsdC_t *)(s_imgBuf+PAGESIZE+sizeof(zlmhdr_t));
                    }

                    // the info on the image to be downloaded and
                    // the address of the Server is now saved
                    osal_memcpy(&s_clientInfo, cpc, sizeof(zlclientC_t));

                    // set event to cause the session to begin
                    osal_set_event(ZLOADApp_TaskID, ZLOAD_IS_CLIENT_EVT);

                }
            }

            break;

        case ZLMSGID_CODE_ENABLE:
            // we can enable code if the dongle code itself is being updated.
            {
                zlceR_t *reply   = (zlceR_t *)cpr;

                paylSize = sizeof(zlceR_t);

                reply->zlceR_state = s_State;

                if (!s_serialMsg)  {
                    reply->zlceR_errorCode = EC_CE_NOT_CLIENT;
                }
                else if (s_State != ZLSTATE_IDLE)  {
                    reply->zlceR_errorCode = EC_CE_NOT_IDLE;
                }
                // see if we're supposed to enable the DL image. spec is in command payload
                // see if there is one...
                else if (!zlgetPreamble(IMAGE_DL, &preamble))  {
                    // see if they match
                    if (!memcmp(cpc, (uint8 *)&preamble.pre_ImgVersion, ZL_IMAGE_ID_LENGTH))  {
                        // DL image there and matches request.
                        // see if image is sane
                        if (zlIsDLImageSane())  {
                            //set event to cause reset
                            osal_set_event(ZLOADApp_TaskID, ZLOAD_CODE_ENABLE_EVT);
                            reply->zlceR_errorCode = EC_NO_ERROR;
                        }
                        else  {
                            reply->zlceR_errorCode = EC_CE_IMAGE_INSANE;
                        }
                    }
                    else  {
                        // DL image there but doesn't match request
                        reply->zlceR_errorCode = EC_CE_NO_MATCH;
                    }
                }
                else  {
                    // no DL image
                    reply->zlceR_errorCode = EC_CE_NO_IMAGE;
                }
            }
            break;

        case ZLMSGID_SEND_DATA:
            {
                zlsdR_t *reply = (zlsdR_t *)cpr;

                paylSize = sizeof(zlsdR_t);

                // do a sanity check.
                if (s_serialMsg)  {
                    reply->zlsdR_errorCode = EC_SD_NOT_SERVER;
                }
                else if (ZLSTATE_PASS_THROUGH == s_State)  {
                    // this should be good enough...
                    if (s_PTClientInfo.srcAddr.addr.shortAddr != MSGpkt->srcAddr.addr.shortAddr)  {
                        // wrong client -- not the current client
                        reply->zlsdR_errorCode = EC_SD_NOT_CLIENT;
                    }
                    else if (((zlsdC_t *)cpc)->zlsdC_sessionID != s_PTSessNum)  {
                        reply->zlsdR_errorCode = EC_SD_BAD_SESS_ID;
                    }
                    else  {
                        zahdrin_t *zain = zlBuildExternalInboundSerialMSG(&s_PTClientInfo, (uint8 *)msg, sizeof(zlmhdr_t) + sizeof(zlsdC_t));

                        // forward message to host
                        if (zain)  {
                            zlSendSerial((uint8 *)zain, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + sizeof(zlsdC_t));
                            osal_mem_free(zain);
                            // save the AF data. there may be AF parameters we need for the reply
                            s_PTClientInfo = *MSGpkt;
                            return;
                        }
                        else  {
                            reply->zlsdR_errorCode = EC_SD_NO_MEM;
                        }
                    }
                }
                else  {
                    reply->zlsdR_errorCode = EC_SD_NOT_SERVER;
                }
            }

            break;

        case ZLMSGID_RESET_BOARD:
            // always legal if over serial port
            if (!s_serialMsg)  {
                return;
            }
            else  {
                zlrstR_t *reply = (zlrstR_t *)cpr;

                reply->zlrstR_state     = s_State;
                reply->zlrstR_errorCode = EC_NO_ERROR;
            }
            paylSize = sizeof(zlrstR_t);
            osal_set_event(ZLOADApp_TaskID, ZLOAD_RESET_BOARD_EVT);
            break;

        case ZLMSGID_RESET:
            // always legal if over serial port
            if (!s_serialMsg)  {
                return;
            }
            else  {
                zlrstR_t *reply = (zlrstR_t *)cpr;

                reply->zlrstR_state     = s_State;
                reply->zlrstR_errorCode = EC_NO_ERROR;
            }
            paylSize = sizeof(zlrstR_t);
            osal_set_event(ZLOADApp_TaskID, ZLOAD_RESET_EVT);
            break;

        default:
            return;
    }

    // the case above has filled in the payload. the header is the
    // same for all. go ahead and fill in header and send reply
    {
        zlmhdr_t *hdr = (zlmhdr_t *)s_replyBuf;

        hdr->zlhdr_msgid  = msg->zlhdr_msgid | ZLMSGID_REPLY_BIT;
        hdr->zlhdr_seqnum = msg->zlhdr_seqnum;
        hdr->zlhdr_msglen = paylSize;

        if (!s_serialMsg)  {
            AF_DataRequest( &MSGpkt->srcAddr,
                             afFindEndPointDesc( MSGpkt->endPoint),
                             MSGpkt->clusterId,
                             sizeof(zlmhdr_t) + paylSize, s_replyBuf,
                            &MSGpkt->cmd.TransSeqNumber,
                             NULL, AF_DEFAULT_RADIUS );
        }
        else  {
            zahdrin_t *zain = zlBuildInternalInboundSerialMSG((uint8 *)hdr, sizeof(zlmhdr_t) + paylSize);

            if (zain)  {
                zlSendSerial((uint8 *)zain, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + paylSize);
                osal_mem_free(zain);
            }
        }
    }

    return;
}


/**********************************************************************************
 * @fn      ZLOADApp_handleReply
 *
 * @brief   Handle a normal ZLOAD reply
 *
 * @param   input
 *            msg pointer to ZLOAD message
 *
 * @return  none.
 */
STATIC void ZLOADApp_handleReply(afIncomingMSGPacket_t *MSGpkt, zlmhdr_t *msg)
{
  uint8    *cpr, msgSize;

    cpr = (uint8 *)msg + sizeof(zlmhdr_t);   // offset of reply payload

	// generate replies to commands here. reply may generate another command out, for
	// example, if data are being transfered and the dvice is acting as client.
	// Commands handled here. replies in next routine

    // ignore reply bit on switch()
    msgSize = 0;
    switch (msg->zlhdr_msgid & (~ZLMSGID_REPLY_BIT))  {
        case ZLMSGID_STATUSQ:
            msgSize = 20;
            //  * * *  NO BREAK  * * *
        case ZLMSGID_CLIENT_CMD:
        case ZLMSGID_RESET:
        case ZLMSGID_RESET_BOARD:
        case ZLMSGID_CODE_ENABLE:
            if (!msgSize)  {
                msgSize = 2;
            }
            // right now, only the host could have sent these -- the dongle never sends these
            // commands on its own. it's a proxy reply. forward it on...
            if (!s_serialMsg)  {
                zahdrin_t *zain = zlBuildExternalInboundSerialMSG(MSGpkt, (uint8 *)msg, sizeof(zlmhdr_t) + msgSize);

                if (zain)  {
                    zlSendSerial((uint8 *)zain, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + msgSize);
                    osal_mem_free(zain);
                }
            }

            break;

	      case ZLMSGID_SESSION_START:
            // this can happen both in pass-through and client mode -- the dongle could be
            // getting updated firmware and is the client receiving the start session reply.
            // in any case we cannot get this reply over air. it has to have come from the Host
            if (!s_serialMsg)  {
                break;
            }
            else if (ZLSTATE_PASS_THROUGH == s_State) {
                // pass reply back to original client
                msg->zlhdr_seqnum = s_PTSeqNum;
                AF_DataRequest( &s_PTClientInfo.srcAddr,
                   afFindEndPointDesc( s_PTClientInfo.endPoint ),
                   s_PTClientInfo.clusterId,
                   sizeof(zlmhdr_t) + sizeof(zlbegsessR_t), (uint8 *)msg,
                   &s_PTClientInfo.cmd.TransSeqNumber,
                   NULL, AF_DEFAULT_RADIUS );
            }
            else if (((zlbegsessR_t *)cpr)->zlbsR_errorCode)  {
                // Server rejected the real client session. back to Idle state.
                osal_set_event(ZLOADApp_TaskID, ZLOAD_RESET_EVT);
            }
            // reply from Server to Begin Session command from Client
            else if (ZLSTATE_CLIENT != s_State) {
                // I didn't send it...ignore.
                break;
            }
            else  {
                uint32 imglen;

                // get length and make sure we have room
                imglen = ((zlbegsessR_t *)cpr)->zlbsR_imgLen;
                if (!zlGetStartFlashAddress(imglen, &s_DLImageBase, &s_DLFirstPageAddress))  {
                    // no room. terminate session
                    osal_set_event(ZLOADApp_TaskID, ZLOAD_XFER_DONE_EVT);
                }
                else  {
                    s_blkSize  = ((zlbegsessR_t *)cpr)->zlbsR_blkSize;
                    s_blkSize *= ((zlbegsessR_t *)cpr)->zlbsR_numBlks;

                    // save preamble offset. if not version 1 or higher, assume ATMega128.
                    // we can infer version from message length. (eventually we could send
                    // a query to target board)
                    if (0x08 == msg->zlhdr_msglen)  {   // version 0: Atmege128 target
                        s_DLPreambleOffset = 0x8C;
                    }
                    else  {
                        s_DLPreambleOffset = ((zlbegsessR_t *)cpr)->zlbsR_preambleOffset;
                    }

                    // figure out how many data packets to request.
                    s_NumPktGet   = (imglen+(s_blkSize-1))>>5;
                    s_NextPacket  = 0;
                    s_offset      = 0;
                    // invalidate any current downloaded image and first page address
                    s_XferBaseAddr = NO_DL_IMAGE_ADDRESS;
                    mbox.AccessZLOADStatus(ACTION_SET, SFIELD_DLBASE, &s_XferBaseAddr);
                    mbox.AccessZLOADStatus(ACTION_SET, SFIELD_DLFIRSTPAGE_ADDR, &s_XferBaseAddr);
                    // set the transfer base address
                    s_XferBaseAddr = s_DLFirstPageAddress;
                    // set wrap address
                    s_wrapAddress = s_DLImageBase + imglen - 1;
                    // request "next" (i.e., first) packet
                    zlRequestNextDataPacket();
                }
            }
            break;

        case ZLMSGID_SESSION_TERM:
            // pass it back if we're in pass-through mode
          if (s_serialMsg && (ZLSTATE_PASS_THROUGH == s_State))  {
              AF_DataRequest( &s_PTClientInfo.srcAddr,
                 afFindEndPointDesc( s_PTClientInfo.endPoint),
                 s_PTClientInfo.clusterId,
                 sizeof(zlmhdr_t) + sizeof(zlendsessR_t), (uint8 *)msg,
                 &s_PTClientInfo.cmd.TransSeqNumber,
                 NULL, AF_DEFAULT_RADIUS );
              // in any case, clean up
              osal_set_event(ZLOADApp_TaskID, ZLOAD_XFER_DONE_EVT);
            }

            break;

        case ZLMSGID_SEND_DATA:
            if (!s_serialMsg)  {
                // reply should not be over air
                break;
            }
            else if (ZLSTATE_CLIENT == s_State)  {
                // not a don't care if we're in client state
                zlProcessSDR((zlsdR_t *)cpr);
            }
            else if (ZLSTATE_PASS_THROUGH == s_State)  {
                // forward to client
                AF_DataRequest( &s_PTClientInfo.srcAddr,
                   afFindEndPointDesc( s_PTClientInfo.endPoint),
                   s_PTClientInfo.clusterId,
                   sizeof(zlmhdr_t) + sizeof(zlsdR_t), (uint8 *)msg,
                   &s_PTClientInfo.cmd.TransSeqNumber,
                   NULL, AF_DEFAULT_RADIUS );
            }
            break;

        default:
            // don't care
            break;
    }
    return;
}

/**********************************************************************************
 * @fn      zlProcessSDR
 *
 * @brief   Process the Send Data Reply as Client
 *
 * @param   input
 *            sdr  pointer to received Send Data Reply
 *
 * @return  none
 */
STATIC void zlProcessSDR(zlsdR_t *sdr)
{
    // is this the right packet?
    if (s_NextPacket > sdr->zlsdR_pktNum)  {
        // we've already seen this one. ignore it
        return;
    }
    else if (s_NextPacket < sdr->zlsdR_pktNum)  {
        // uh oh. we've missed one. kill session...
        osal_set_event(ZLOADApp_TaskID, ZLOAD_XFER_DONE_EVT);
        return;
    }
    else if (sdr->zlsdR_errorCode)  {
        // there are no data here. just ignaore the packet. if things
        // are relly screwed up the timer will expire and we'll quit.
        return;
    }

    // save data
    osal_memcpy(s_imgBuf+s_offset, sdr->zlsdR_data, s_blkSize);

    // flash if necessary
    if ((s_NextPacket & 0x7) == 0x7)  {
        if (!zlIsWriteFlashOK())  {
            // uh oh. can't program flash without errors. kill session
            osal_set_event(ZLOADApp_TaskID, ZLOAD_XFER_DONE_EVT);
            return;
        }
        s_XferBaseAddr += PAGESIZE;
        // reset address if we wrapped
        if (s_XferBaseAddr > s_wrapAddress)  {
            s_XferBaseAddr = s_DLImageBase;
        }
        s_offset = 0;
    }
    else  {
        s_offset += s_blkSize;
    }

    // OK to increment next packet number now.
    s_NextPacket++;

    // ask for next packet
    zlRequestNextDataPacket();

    return;
}

/**********************************************************************************
 * @fn      zlRequestNextDataPacket
 *
 * @brief   Set up the next data packet request. Takes care of knowing when
 *          transfer is done. Takes care of retry timer.
 *
 * @param   none
 *
 * @return  none
 */
STATIC void zlRequestNextDataPacket()
{
    if (s_NextPacket >= s_NumPktGet)  {
        osal_set_event(ZLOADApp_TaskID, ZLOAD_XFER_DONE_EVT);
        s_SubState = ZLSTATE_SUBSTATE_XFER_DONE;
        // kill response timeout check
        osal_stop_timerEx(ZLOADApp_TaskID, ZLOAD_SDRTIMER_EVT);
        return;
    }

    s_sdcpayload->zlsdC_pktNum = s_NextPacket;
    s_sdcmd->zlhdr_seqnum      = ++s_lastSeqNum;

    {
        zahdrin_t *zain = zlBuildInternalInboundSerialMSG((uint8 *)s_sdcmd, sizeof(zlmhdr_t) + sizeof(zlsdC_t));

        if (zain)  {
            zlSendSerial((uint8 *)zain, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + sizeof(zlsdC_t));
            osal_mem_free(zain);
        }
    }
    osal_start_timerEx(ZLOADApp_TaskID, ZLOAD_SDRTIMER_EVT, SDR_WAIT_TO);
    s_SDCRetryCount = SDC_RETRY_COUNT;

    // set timeout timer
    // need osal timer resource and set event flag and need a method to process timeout event
    return;
}

/**********************************************************************************
 * @fn      zlgetPreamble
 *
 * @brief   Retrieve either dlownloaded image or active image preamble.
 *
 * @param   input
 *            which      which preamble to get
 *            preamble   pointer to receive retrieved information
 *
 * @return  0: premable retrieval succeeded
 *          1: preamble retrieval failed
 */
STATIC uint8 zlgetPreamble(image_t which, preamble_t *preamble)
{
    uint32 tmpul;

	// get base address of image
    if (IMAGE_DL == which)  {
        mbox.AccessZLOADStatus(ACTION_GET, SFIELD_DLFIRSTPAGE_ADDR, &tmpul);
        // is the base address valid...
        if (tmpul == NO_DL_IMAGE_ADDRESS)  {
            return 1;
        }
    }
    else if (IMAGE_ACTIVE == which)  {
        tmpul = OP_IMAGE_BASE_ADDRESS;
    }
    else  {
        return 1;
    }

	// go get preamble
    mbox.GetPreamble(which, tmpul, preamble);

    // do a quick sanity check
    if ((preamble->pre_Magic[0] != PREAMBLE_MAGIC1) || (preamble->pre_Magic[1] != PREAMBLE_MAGIC2))  {
        return 1;
    }

    return 0;
}


/**********************************************************************************
 * @fn      zlCodeEnable
 *
 * @brief   Enable downloaded code to be installed. Set up mailbox with the token
 *          telling the boot code to flash the downloaded image and reset board.
 *
 * @param   none
 *
 * @return  none
 */

STATIC void zlCodeEnable()
{
    // set up mail box to tell boot code to flash the downloaded image
    mboxMsg.BootRead = MBOX_OAD_ENABLE;

    zlResetBoard();
    return;
}

/**********************************************************************************
 * @fn      zlResetBoard
 *
 * @brief   Do a board reset by causing a watchdog timeout.
 *
 * @param   none
 *
 * @return  none
 */
// enable WDT and set about 50 msec TO. this gives the reply a chance to be sent
// set WD mode, enable, 250 msec
#ifdef __ICC8051__
#define   WDT_TIMEOUT    0x09
#define   WD_REGISTER    WDCTL

STATIC void zlResetBoard()
{
    // do reset by setting the WDT and waiting for it to timeout
    HAL_DISABLE_INTERRUPTS();
    WD_REGISTER = WDT_TIMEOUT;

    // enable watchdog and wait for timeout...
    while (1)  {
      asm("NOP");
    }
}
#else
#define   WDT_TIMEOUT    0x18
STATIC void zlResetBoard()
{
    // do reset by setting the WDT and waiting for it to timeout
    HAL_DISABLE_INTERRUPTS();  // prevent petting the dog...
    WDTCR = WDT_TIMEOUT;

    // enable watchdog and wait for timeout...
    while (1)  {
      asm("NOP");
    }
}
#endif  // __ICC8051__
/**********************************************************************************
 * @fn      zlCleanupOnReset
 *
 * @brief   ZLOAD reset occurred. Free resources and reset state machine and other
 *          supporting constructs
 *
 * @param   none
 *
 * @return  none
 */
STATIC void zlCleanupOnReset()
{
    s_State = ZLSTATE_IDLE;

     // free memory
    if (s_imgBuf)  {
        osal_mem_free(s_imgBuf);
        s_imgBuf = 0;
    }

    s_NextPacket      = 0;
    s_NumPktGet       = 0;

    return;
}


/**************************************************************************************
 * @fn      zlCleanupOnXferDone
 *
 * @brief   Data transfer session complete (not necessarily correctly). Free resources
 *          and reset state machine and other supporting constructs. Send Terminate
 *          Session command to Server.
 *
 * @param   none
 *
 * @return  none
 */
STATIC void  zlCleanupOnXferDone()
{
    if (ZLSTATE_PASS_THROUGH == s_State)  {
      s_State = ZLSTATE_IDLE;

      return;
    }
    // kill response timeout check
    osal_stop_timerEx(ZLOADApp_TaskID, ZLOAD_SDRTIMER_EVT);

    // check for flashing last block and send Session Terminate command
    if (s_State == ZLSTATE_CLIENT)  {
        zlmhdr_t *hdr = osal_mem_alloc(sizeof(zlmhdr_t) + sizeof(zlendsessC_t));

        if (hdr)  {
            zlendsessC_t *cmd = (zlendsessC_t *)((uint8 *)hdr + sizeof(zlmhdr_t));

            hdr->zlhdr_msgid  = ZLMSGID_SESSION_TERM;
            hdr->zlhdr_msglen = sizeof(zlendsessC_t);

            cmd->zlesC_sessionID = s_clientSessionID;
            hdr->zlhdr_seqnum    = ++s_lastSeqNum;
            {
                zahdrin_t *zain = zlBuildInternalInboundSerialMSG((uint8 *)hdr, sizeof(zlmhdr_t) + sizeof(zlendsessC_t));

                if (zain)  {
                    zlSendSerial((uint8 *)zain, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + sizeof(zlendsessC_t));
                    osal_mem_free(zain);
                }
            }
            osal_mem_free(hdr);
        }

        // the substate is set only if we terminated by receiving the last packet successfully
        if (s_SubState == ZLSTATE_SUBSTATE_XFER_DONE)  {
            uint8 flashOK = 1;

            // if offset is non-zero there's something in the buffer
            if (s_offset)  {
                // we need to flash a partially full buffer.
                flashOK = zlIsWriteFlashOK();
            }
            // if the flashing worked, check the image imtegrity. if it's
            // OK then we can update the downloaded image base address.

            // need to set the preamble offset before calling the code sanity code. it's a
            // don't care of this gets it and then the check fails. the existence of the
            // downloaded code is inferred based on whether the base and first page
            // addresses are there.
            mbox.AccessZLOADStatus(ACTION_SET, SFIELD_PREAMBLE_OFFSET, &s_DLPreambleOffset);
            if (flashOK && !mbox.CheckCodeSanity(IMAGE_DL, s_DLImageBase, s_DLFirstPageAddress))  {
                // OK to update location of downloaded image
                mbox.AccessZLOADStatus(ACTION_SET, SFIELD_DLBASE, &s_DLImageBase);
                mbox.AccessZLOADStatus(ACTION_SET, SFIELD_DLFIRSTPAGE_ADDR, &s_DLFirstPageAddress);
            }
            s_SubState = ZLSTATE_SUBSTATE_NONE;
        }
    }

    // free memory
    osal_mem_free(s_imgBuf);
    s_imgBuf     = 0;
    s_State      = ZLSTATE_IDLE;
    s_NextPacket = 0;
    s_NumPktGet  = 0;

    return;
}

/**************************************************************************************
 * @fn      zlStartClientSession
 *
 * @brief   We have been commanded to be in the Client role. Set up to start the
 *          session with the Server by sending the Begin Session command
 *
 * @param   none
 *
 * @return  none
 */
STATIC void zlStartClientSession()
{
    uint8 *buf;

    // we already have Server info. Contact Server to set up session
    if (buf=osal_mem_alloc(sizeof(zlmhdr_t) + sizeof(zlbegsessC_t)))  {
        zlmhdr_t     *hdr = (zlmhdr_t *)buf;
        zlbegsessC_t *cmd = (zlbegsessC_t *)((uint8 *)buf+sizeof(zlmhdr_t));

        // initialize session id as lsb of IEEE address
        if (!s_clientSessionID)  {
            uint8 buf[Z_EXTADDR_LEN];

            ZMacGetReq(ZMacExtAddr, buf);
            s_clientSessionID = buf[0];
        }

        s_State = ZLSTATE_CLIENT;
        s_clientSessionID++;

        hdr->zlhdr_msgid     = ZLMSGID_SESSION_START;
        hdr->zlhdr_seqnum    = ++s_lastSeqNum;
        hdr->zlhdr_msglen    = sizeof(zlbegsessC_t);
        cmd->zlbsC_ver       = s_clientInfo.zlclC_ver;
        cmd->zlbsC_manu      = s_clientInfo.zlclC_manu;
        cmd->zlbsC_prod      = s_clientInfo.zlclC_prod;
        cmd->zlbsC_sessionID = s_clientSessionID;
        s_NextPacket         = 0;

        // populate session ID element in the Send data payload
        s_sdcpayload->zlsdC_sessionID = s_clientSessionID;

        // set up SD command header
        s_sdcmd->zlhdr_msgid  = ZLMSGID_SEND_DATA;
        s_sdcmd->zlhdr_msglen = sizeof(zlsdC_t);
        {
            zahdrin_t *zain = zlBuildInternalInboundSerialMSG((uint8 *)hdr, sizeof(zlmhdr_t) + sizeof(zlbegsessC_t));

            if (zain)  {
                zlSendSerial((uint8 *)zain, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + sizeof(zlbegsessC_t));
                osal_mem_free(zain);
            }
        }
        osal_mem_free(buf);
    }
	else  {
		// we couldn't send the Begin Session command. go back to Idle state.
        osal_set_event(ZLOADApp_TaskID, ZLOAD_RESET_EVT);
	}

    return;
}

/********************************************************************************************
 * @fn      zlGetStartFlashAddress
 *
 * @brief   Compute the flash address at which to begin storing the downloaded image and
 *          the address of the first page. They will not be the same if the downloaded
 *          image is bigger than the currently active image. In this case the image will
 *          wrap within the flash space allocated to the image.
 *
 * @param   input
 *            dlimgsize  length of image to be downloaded
 *          output
 *            startAddress   address at which downloaded image will begin occupying space
 *            fpAddress      address of flash page one of the image
 *
 * @return  0: returned flash addresses are not valid. image will not fit in remaining space
 *          1: returned flash addresses are valid
 */
STATIC uint8 zlGetStartFlashAddress(uint32 dlimgsize, uint32 *startAddress, uint32 *fpAddress)
{
#ifndef CC2420DB
    uint16  tmpu16;

    mbox.AccessZLOADStatus(ACTION_GET, SFIELD_EXTERNAL_MEM_OS, &tmpu16);

    // if the image is too big to put into external flash or it is too big
    // for remaining memory on-chip then we can't download image
    if ((dlimgsize > (FLASHSIZE - tmpu16))  ||
        (dlimgsize > (FLASHSIZE - ((TOP_BOOT_PAGES+1)*CC2430_FLASH_PAGE_SIZE))))  {
      return 0;
    }

    *startAddress =
    *fpAddress    = tmpu16;

    return 1;
#else
    preamble_t preamble;
    uint32     imgsize;
    uint16     bootsize, num_pages;

    zlgetPreamble(IMAGE_ACTIVE, &preamble);
    mbox.AccessZLOADStatus(ACTION_GET, SFIELD_BOOTSIZE, &bootsize);

    // convert sizes to flash pages. don't use total -- do each individually
    // we're using the last page in application flash for persistent memory for
    // OAD support. reserve it by adding 1.
    // TODO: make the last page in application flash usable.

    imgsize   = GET_NUM_PAGES(preamble.pre_Length);
    dlimgsize = GET_NUM_PAGES(dlimgsize);
    num_pages = imgsize + dlimgsize + GET_NUM_PAGES(bootsize) + 1;

    if (num_pages > GET_NUM_PAGES(FLASHSIZE))  {
        return 0;
    }

    // return the flashing address at which to start
    *startAddress = imgsize * PAGESIZE;
    // return first page address
    *fpAddress = *startAddress;
    if (dlimgsize > imgsize)  {
        *fpAddress += (dlimgsize - imgsize) * PAGESIZE;
    }

    return 1;
#endif
}

/**************************************************************************************
 * @fn      zlIsDLImageSane
 *
 * @brief   Determine if downloaded image is sane. Checks both magic bytes and CRC
 *
 * @param   none
 *
 * @return  0: image is not sane
 *          1: image is OK.
 */
STATIC uint8 zlIsDLImageSane()
{
    uint32 baseAddr, firstPageAddr;

    mbox.AccessZLOADStatus(ACTION_GET, SFIELD_DLBASE, &baseAddr);
    mbox.AccessZLOADStatus(ACTION_GET, SFIELD_DLFIRSTPAGE_ADDR, &firstPageAddr);
    if (mbox.CheckCodeSanity(IMAGE_DL, baseAddr, firstPageAddr))  {
        return 0;
    }
    return 1;
}

/**************************************************************************************
 * @fn      zlIsWriteFlashOK
 *
 * @brief   Write flash and return status. Always succeeds in the write-once scenario.
 *          It can fail only if the write-reread method fails a fixed number of times.
 *
 * @param   none
 *
 * @return  0: flash write failed
 *          1: flash write is OK.
 */
STATIC uint8 zlIsWriteFlashOK()
{
    mbox.WriteFlash(IMAGE_DL, s_XferBaseAddr, s_imgBuf, PAGESIZE);

    return 1;
}

/**************************************************************************************
 * @fn      zlResendSDC
 *
 * @brief   Resend the previous Send Data Command because a repply was not received
 *          before a timeout.
 *
 * @param   none
 *
 * @return  none.
 */
STATIC void zlResendSDC()
{
    if (s_State != ZLSTATE_CLIENT)  {
        // late expiration -- don't care
        return;
    }

    if (--s_SDCRetryCount)  {
        // the static structure still contains the last packet number to be requested.
        // the only way the packet number gets bumped is if a reply is received.
        s_sdcmd->zlhdr_seqnum = ++s_lastSeqNum;
        {
            zahdrin_t *zain = zlBuildInternalInboundSerialMSG((uint8 *)s_sdcmd, sizeof(zlmhdr_t) + sizeof(zlsdC_t));

            if (zain)  {
                zlSendSerial((uint8 *)zain, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + sizeof(zlsdC_t));
                osal_mem_free(zain);
            }
        }
        osal_start_timerEx(ZLOADApp_TaskID, ZLOAD_SDRTIMER_EVT, SDR_WAIT_TO);
    }
    else  {
        // too many retries. abandon session.
        osal_set_event(ZLOADApp_TaskID, ZLOAD_XFER_DONE_EVT);
    }

    return;
}

// start a session with the host on behalf of the client from which
// the start session was received. do this by sending self a client
// command using the start session parameters. ZLOAD will think it
// is a client message coming from the host asking it to come get
// the image being requested by the real client.
STATIC uint8 zlPassOnStartSessionOK(uint8 *msg)
{
    zahdrin_t *zamsgin;

    // forward the Begin Session command
    zamsgin = zlBuildExternalInboundSerialMSG(&s_PTClientInfo, msg, sizeof(zlmhdr_t) + sizeof(zlbegsessC_t));

    if (!zamsgin)  {
      return 0;
    }

    zlSendSerial((uint8 *)zamsgin, SIZEOF_ZAIN_HDR + sizeof(zlmhdr_t) + sizeof(zlbegsessC_t));

    osal_mem_free(zamsgin);

    return 1;
}

// inbound messages can be built from address information gleaned from the client
// during a pass-through session setup. but there can also be inbound messages
// such as status replies that are not part of any ongoing client session. in this
// case the address information comes from the AF message. in either case this
// information is requried to populate the header for for the host.
STATIC zahdrin_t *zlBuildExternalInboundSerialMSG(afIncomingMSGPacket_t *addressInfo, uint8 *zlmsg, uint8 len)
{
    zahdrin_t *zamsgin;

    // build message
    zamsgin = (zahdrin_t *)osal_mem_alloc(SIZEOF_ZAIN_HDR + len);

    if (!zamsgin)  {
      return 0;
    }

    zamsgin->zaproxy_nwkAddr   = addressInfo->srcAddr.addr.shortAddr;
    zamsgin->zaproxy_endp      = addressInfo->endPoint;
    zamsgin->zaproxy_ClusterID = addressInfo->clusterId;
    zamsgin->zaproxy_msglen    = len;

    osal_memcpy(zamsgin->zaproxy_payload, zlmsg, len);

    return zamsgin;
}

STATIC zahdrin_t *zlBuildInternalInboundSerialMSG(uint8 *zlmsg, uint8 len)
{
    zahdrin_t *zamsgin;

    // build message
    zamsgin = (zahdrin_t *)osal_mem_alloc(SIZEOF_ZAIN_HDR + len);

    if (!zamsgin)  {
      return 0;
    }

    zamsgin->zaproxy_nwkAddr   = s_myNwkAddr;
    zamsgin->zaproxy_endp      = ZLOADAPP_ENDPOINT;
    zamsgin->zaproxy_ClusterID = ZLOADAPP_CLUSTERID_OADMESG;
    zamsgin->zaproxy_msglen    = len;

    osal_memcpy(zamsgin->zaproxy_payload, zlmsg, len);

    return zamsgin;
}
