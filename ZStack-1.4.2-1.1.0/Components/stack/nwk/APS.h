#ifndef APS_H
#define APS_H
/*********************************************************************
    Filename:       APS.h
    Revised:        $Date: 2007-02-23 11:29:38 -0800 (Fri, 23 Feb 2007) $
    Revision:       $Revision: 13588 $

    Description:

    Primitives of the Application Support Sub Layer Task functions.

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
#include "APSMEDE.h"
#include "BindingTable.h"
#include "reflecttrack.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint16             nwkSrcAddr;
  uint8              nwkSecure;
  aps_FrameFormat_t* aff;
} APS_CmdInd_t;

typedef struct
{
  osal_event_hdr_t hdr;
  zAddrType_t SrcAddress;
  uint8 LinkQuality;
  byte SecurityUse;
  uint32 timestamp;
  aps_FrameFormat_t aff;
} apsInMsg_t;

typedef void (*pfnBindingTimeoutCB)( void );


/*********************************************************************
 * CONSTANTS
 */

// APS Command IDs - sent messages
#define APS_INCOMING_MSG                        0x01
#define APS_CMD_PKT                             0x02

// APS Message Fields
#define APS_MSG_ID                              0x00
#define APS_MSG_ID_LEN                          0x01

// APS Command Messages
#define APS_CMD_PKT_HDR ((uint8)                    \
                         (sizeof(APSME_CmdPkt_t) +  \
                          APS_MSG_ID_LEN          ))

// APS Events
#define APS_EDBIND_TIMEOUT_TIMER_ID             0x0001

// APS Relector Types
#define APS_REFLECTOR_PUBLIC  0
#define APS_REFLECTOR_PRIVATE 1

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern uint8 APS_Counter;
extern byte APS_TaskID;
extern uint16 AIB_MaxBindingTime;

/*********************************************************************
 * APS System Functions
 */

/*
 * Initialization function for the APS layer.
 */
extern void APS_Init( byte task_id );

/*
 * Event Loop Processor for APS.
 */
extern UINT16 APS_event_loop( byte task_id, UINT16 events );

/*
 * Setup the End Device Bind Timeout
 */
extern void APS_SetEndDeviceBindTimeout( uint16 timeout, pfnBindingTimeoutCB pfnCB );

/*
 * APS Command Indication
 */
extern void APS_CmdInd( APS_CmdInd_t* ind );

/*
 * APS Reflector Initiatialization 
 *   APS_REFLECTOR_PUBLIC
 *   APS_REFLECTOR_PRIVATE
 */
extern void APS_ReflectorInit( uint8 type );

/*********************************************************************
 * REFLECTOR FUNCTION POINTERS
 */

extern ZStatus_t (*pAPS_UnBind)( zAddrType_t *SrcAddr, byte SrcEndpInt,
                            uint16 ClusterId, zAddrType_t *DstAddr, byte DstEndpInt );

/*
 * Fill in pItem w/ info from the Nth valid binding table entry.
 */
extern ZStatus_t (*pAPS_GetBind)( uint16 Nth, apsBindingItem_t *pItem );

extern void (*pAPS_DataConfirmReflect)( nwkDB_t *rec, uint8 status );

extern void (*pAPS_DataIndReflect)( zAddrType_t *SrcAddress, aps_FrameFormat_t *aff,
                    uint8 LinkQuality, byte AckRequest, byte SecurityUse, uint32 timestamp  );

/****************************************************************************
****************************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* APS_H */


