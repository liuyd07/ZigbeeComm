#ifndef ZCL_CLOSURES_H
#define ZCL_CLOSURES_H

/*********************************************************************
    Filename:       zcl_closures.h
    Revised:        $Date: 2006-04-03 16:27:20 -0700 (Mon, 03 Apr 2006) $
    Revision:       $Revision: 10362 $

    Description:

       This file contains the ZCL Closures definitions.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved. Permission to use, reproduce, copy, prepare
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
#include "zcl.h"

/*********************************************************************
 * CONSTANTS
 */

/**********************************************/
/*** Shade Configuration Cluster Attributes ***/
/**********************************************/
  // Shade information attributes set
#define ATTRID_CLOSURES_PHYSICAL_CLOSED_LIMIT                          0x0000
#define ATTRID_CLOSURES_MOTOR_STEP_SIZE                                0x0001
#define ATTRID_CLOSURES_STATUS                                         0x0002
/*** Status attribute bit values ***/
#define CLOSURES_STATUS_SHADE_IS_OPERATIONAL                           0x01
#define CLOSURES_STATUS_SHADE_IS_ADJUSTING                             0x02
#define CLOSURES_STATUS_SHADE_DIRECTION                                0x04
#define CLOSURES_STATUS_SHADE_MOTOR_FORWARD_DIRECTION                  0x08
  // Shade settings attributes set
#define ATTRID_CLOSURES_CLOSED_LIMIT                                   0x0010
#define ATTRID_CLOSURES_MODE                                           0x0012
/*** Mode attribute values ***/
#define CLOSURES_MODE_NORMAL_MODE                                      0x00
#define CLOSURES_MODE_CONFIGURE_MODE                                   0x01
// cluster has no specific commands

/**********************************************/
/*** Logical Cluster ID - for mapping only ***/
/***  These are not to be used over-the-air ***/
/**********************************************/
#define ZCL_CLOSURES_LOGICAL_CLUSTER_ID_SHADE_CONFIG                   0x0010

/*********************************************************************
 * TYPEDEFS
 */
// This callback is just a place holder
//

typedef void (*zclClosures_PlaceHolder_t)( void );

// Register Callbacks table entry - enter function pointers for callbacks that
// the application would like to receive
typedef struct			
{
  zclClosures_PlaceHolder_t               pfnPlaceHolder; // Place Holder
//  NULL
} zclClosures_AppCallbacks_t;


 /*
  * Register for callbacks from this cluster library
  */
extern ZStatus_t zclClosures_RegisterCmdCallbacks( uint8 endpoint, zclClosures_AppCallbacks_t *callbacks );



/*********************************************************************
 * VARIABLES
 */


/*********************************************************************
 * FUNCTIONS
 */



/*********************************************************************
 * FUNCTION MACROS
 */



#ifdef __cplusplus
}
#endif

#endif /* ZCL_CLOSURES_H */
