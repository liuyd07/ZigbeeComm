#ifndef ZLOAD_IF_H
#define ZLOAD_IF_H

/*********************************************************************
    Filename:       ZLOAD_App.h
    Revised:        $Date: 2006-04-11 15:33:15 -0700 (Tue, 11 Apr 2006) $
    Revision:       $Revision: 10465 $

    Description:

         ZLOAD header declaring prototypes

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
 * MACROS
 */

#ifdef __ICC8051__
  #define  OAD_MAKE_IMAGE_ID()   \
    const __code __root unsigned short imgId[] @ "OAD_IMAGE_ID" = {IMAGE_VERSION, MANUFACTURER_ID, PRODUCT_ID}
#else
  #define  OAD_MAKE_IMAGE_ID()   \
    const __root __farflash unsigned short imgId[] @ "OAD_IMAGE_ID" = {IMAGE_VERSION, MANUFACTURER_ID, PRODUCT_ID}
#endif

/*********************************************************************
 * DEFINES
 */

// support to select callback event for which subscription is desired
// this information is transmitted as a bit map.
#define  ZLCB_EVENT_OADBEGIN_CLIENT     ((unsigned short)0x0001)
#define  ZLCB_EVENT_OADEND_CLIENT       ((unsigned short)0x0002)
#define  ZLCB_EVENT_OADBEGIN_SERVER     ((unsigned short)0x0004)
#define  ZLCB_EVENT_OADEND_SERVER       ((unsigned short)0x0008)
#define  ZLCB_EVENT_CODE_ENABLE_RESET   ((unsigned short)0x0010)
#define  ZLCB_EVENT_ALL                 (ZLCB_EVENT_OADBEGIN_CLIENT   | \
                                         ZLCB_EVENT_OADEND_CLIENT     | \
                                         ZLCB_EVENT_OADBEGIN_SERVER   | \
                                         ZLCB_EVENT_OADEND_SERVER     | \
                                         ZLCB_EVENT_CODE_ENABLE_RESET   \
                                        )


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
extern UINT16 ZLOADApp_ProcessEvent( byte task_id, unsigned short events );


/*
 * API for user to register pre-reset callback
 */
extern void ZLOADApp_RegisterOADEventCallback(void(*pCBFunction)(unsigned short), unsigned short eventMask);

#endif  // ZLOAD_IF_H