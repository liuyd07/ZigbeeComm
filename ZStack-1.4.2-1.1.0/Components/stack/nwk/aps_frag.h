/*********************************************************************
  Filename:       apsf.h
  Revised:        $Date: 2006-10-05 11:29:16 -0700 (Thu, 05 Oct 2006) $
  Revision:       $Revision: 12195 $

  Description:    Implements APS Application Data Unit Fragmentation 

  Notes:

  Copyright (c) 2005 by Figure 8 Wireless, Inc., All Rights Reserved.
  Permission to use, reproduce, copy, prepare derivative works,
  modify, distribute, perform, display or sell this software and/or
  its documentation for any purpose is prohibited without the express
  written consent of Figure 8 Wireless, Inc.
*********************************************************************/
#ifndef APSF_H
#define APSF_H

#include "AF.h"
#include "ZDApp.h"

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
#define APSF_SCHED_EVT             0x0001

/*********************************************************************
 * MACROS
 */
#define APSF_Enabled            (APSF_taskID != 0xff)    

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern uint8 APSF_taskID;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization
 */
extern void APSF_Init(uint8 task_id);

/*
 * Task Event Processor
 */
extern void APSF_Init(uint8 task_id);
extern UINT16 APSF_ProcessEvent( uint8 task_id, UINT16 events );
extern afStatus_t APSF_SendFragmented(APSDE_DataReq_t *pReq);
extern void APSF_ProcessAck(aps_FrameFormat_t *aff, uint16 srcAddr, uint8 status);
extern void APSF_SendOsalMsg(uint8 *msgPtr, uint16 msgLen);

typedef afStatus_t APSF_SendFragmented_t(APSDE_DataReq_t *pReq);
typedef void APSF_ProcessAck_t(aps_FrameFormat_t *aff, uint16 srcAddr, uint8 status);
typedef void APSF_SendOsalMsg_t(uint8 *msgPtr, uint16 msgLen);

extern APSF_SendFragmented_t *apsfSendFragmented;
extern APSF_ProcessAck_t *apsfProcessAck;
extern APSF_SendOsalMsg_t *apsfSendOsalMsg;

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif


#endif // APSF_H