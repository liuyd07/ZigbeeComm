#ifndef OSAL_NV_H
#define OSAL_NV_H

/*********************************************************************
    Filename:       OSAL_Nv.h
    Revised:        $Date: 2007-03-30 14:31:06 -0700 (Fri, 30 Mar 2007) $
    Revision:       $Revision: 13904 $

    Description:

       This module defines the OSAL non-volatile memory functions.

    Notes:

    Copyright (c) 2007 by Texas Instruments, Inc.
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

// FCTL bit definitions
#define FERASE  0x01  // Page erase: erase=1
#define FWRITE  0x02  // Page write: write=1
#define FCONRD  0x10  // Continuous read: enable=1
#define FSWBSY  0x40  // Single write: busy=1
#define FBUSY   0x80  // Write/erase: busy=1

#define FWBUSY (FSWBSY | FBUSY)

#define OSAL_NV_PAGE_SIZE       2048

#ifndef OSAL_NV_PAGES_USED
  #define OSAL_NV_PAGES_USED    2       // Minimum = 2
#endif

/* The 1st block of pages of the 3rd bank are reserved for NV, any changes to the constants below
 * must be accompanied by the corresponding pages in the linker file, currently f8w2430.xcl
 */
#define OSAL_NV_PAGE_BEG        60
#define OSAL_NV_PAGE_END       (OSAL_NV_PAGE_BEG + OSAL_NV_PAGES_USED - 1)

/*********************************************************************
 * MACROS
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
 * Initialize NV service
 */
extern void osal_nv_init( void *p );

/*
 * Initialize an item in NV
 */
extern byte osal_nv_item_init( uint16 id, uint16 len, void *buf );

/*
 * Read an NV attribute
 */
extern byte osal_nv_read( uint16 id, uint16 offset, uint16 len, void *buf );

/*
 * Write an NV attribute
 */
extern byte osal_nv_write( uint16 id, uint16 offset, uint16 len, void *buf );

/*
 * Get the length of an NV item.
 */
extern uint16 osal_nv_item_len( uint16 id );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OSAL_NV.H */
