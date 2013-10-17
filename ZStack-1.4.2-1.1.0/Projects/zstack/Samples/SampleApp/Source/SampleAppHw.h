#ifndef SAMPLEAPPHW_H
#define SAMPLEAPPHW_H

/*********************************************************************
    Filename:       SampleAppHw.h
    Revised:        $Date: 2007-03-08 13:26:13 -0800 (Thu, 08 Mar 2007) $
    Revision:       $Revision: 13719 $

    Description:

       This file contains the Sample Application definitions.

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

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Read the Coordinator Jumper
 */
uint8 readCoordinatorJumper( void );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SAMPLEAPPHW_H */
