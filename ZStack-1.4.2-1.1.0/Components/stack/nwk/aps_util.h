#ifndef APSUTIL_H
#define APSUTIL_H
/*********************************************************************
    Filename:       aps_util.h
    Revised:        $Date: 2006-05-09 14:10:14 -0700 (Tue, 09 May 2006) $
    Revision:       $Revision: 10731 $

    Description:

    Application Support Sub Layer utility functions.

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
#include "APSMEDE.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Reflect the incoming message to it's bound device.
 */
extern byte apsReflectMsg( zAddrType_t *SrcAddress, 
                           aps_FrameFormat_t *aff, uint8 LinkQuality, 
                           uint8 ackReq, uint8 SecurityUse,
                           uint8 startingIndex );

extern void apsReReflectMsg( nwkDB_t *rec );

/*
 * Parse the APS message
 */
extern void APSDE_ParseMsg( NLDE_FrameFormat_t *ff, 
                            aps_FrameFormat_t *aff );

extern void apsGenerateAck( uint16 dstAddr, aps_FrameFormat_t *aff );

extern void apsProcessAck( uint16 srcAddr, aps_FrameFormat_t *aff );

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* APSUTIL_H */


