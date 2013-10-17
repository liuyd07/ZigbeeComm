/*********************************************************************
    Filename:       MT_AF.h
    Revised:        $Date: 2006-09-07 17:17:04 -0700 (Thu, 07 Sep 2006) $
    Revision:       $Revision: 11918 $

    Description:

        MonitorTest functions for APS.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/
#ifndef MT_AF_H
#define MT_AF_H

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "MTEL.h"
#include "AF.h"

#include "OnBoard.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#if defined ( MT_AF_CB_FUNC )
#define CB_ID_AF_DATA_IND               0x0008
#define SPI_AF_CB_TYPE                  0x0900
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

#if defined ( MT_AF_CB_FUNC )
extern uint16 _afCallbackSub;
#endif

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

#if defined ( MT_AF_FUNC )
/*********************************************************************
 *
 */
void MT_afCommandProcessing( uint16 cmd_id , byte len , byte *pData );
#endif

#if defined ( MT_AF_CB_FUNC )
/*
 * Process the callback subscription for AF Incoming data.
 */
void af_MTCB_IncomingData( void *pkt );
#endif

/*********************************************************************
*********************************************************************/
#endif
