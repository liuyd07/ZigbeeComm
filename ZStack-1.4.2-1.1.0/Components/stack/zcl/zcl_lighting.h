#ifndef ZCL_LIGHTING_H
#define ZCL_LIGHTING_H

/******************************************************************************
    Filename:       zcl_lighting.h
    Revised:        $Date: 2006-04-03 16:27:20 -0700 (Mon, 03 Apr 2006) $
    Revision:       $Revision: 10362 $

    Description:

       This file contains the ZCL Lighting library definitions.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved. Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 * INCLUDES
 */
#include "zcl.h"

/******************************************************************************
 * CONSTANTS
 */
					
/*****************************************/
/***  Color Control Cluster Attributes ***/
/*****************************************/
  // Color Information attributes set
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE                        0x0000
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION                 0x0001
#define ATTRID_LIGHTING_COLOR_CONTROL_REMAINING_TIME                     0x0002
  // Color Settings attributes set: currently none
  // commands:
#define COMMAND_LIGHTING_MOVE_TO_HUE                                     0x00
#define COMMAND_LIGHTING_MOVE_HUE                                        0x01
#define COMMAND_LIGHTING_STEP_HUE                                        0x02
#define COMMAND_LIGHTING_MOVE_TO_SATURATION                              0x03
#define COMMAND_LIGHTING_MOVE_SATURATION                                 0x04
#define COMMAND_LIGHTING_STEP_SATURATION                                 0x05
#define COMMAND_LIGHTING_MOVE_TO_HUE_AND_SATURATION                      0x06
  /***  Move To Hue Cmd payload: direction field values  ***/
#define LIGHTING_MOVE_TO_HUE_DIRECTION_SHORTEST_DISTANCE                 0x00
#define LIGHTING_MOVE_TO_HUE_DIRECTION_LONGEST_DISTANCE                  0x01
#define LIGHTING_MOVE_TO_HUE_DIRECTION_UP                                0x02
#define LIGHTING_MOVE_TO_HUE_DIRECTION_DOWN                              0x03
  /***  Move Hue Cmd payload: moveMode field values   ***/
#define LIGHTING_MOVE_HUE_STOP                                           0x00
#define LIGHTING_MOVE_HUE_UP                                             0x01
#define LIGHTING_MOVE_HUE_DOWN                                           0x03
  /***  Step Hue Cmd payload: stepMode field values ***/
#define LIGHTING_STEP_HUE_UP                                             0x01
#define LIGHTING_STEP_HUE_DOWN                                           0x03
  /***  Move Saturation Cmd payload: moveMode field values ***/
#define LIGHTING_MOVE_SATURATION_STOP                                    0x00
#define LIGHTING_MOVE_SATURATION_UP                                      0x01
#define LIGHTING_MOVE_SATURATION_DOWN                                    0x03
  /***  Step Saturation Cmd payload: stepMode field values ***/
#define LIGHTING_STEP_SATURATION_UP                                      0x01
#define LIGHTING_STEP_SATURATION_DOWN                                    0x03

/*****************************************************************************/
/***          Ballast Configuration Cluster Attributes                     ***/
/*****************************************************************************/
  // Ballast Information attribute set
#define ATTRID_LIGHTING_BALLAST_CONFIG_PHYSICAL_MIN_LEVEL                0x0000
#define ATTRID_LIGHTING_BALLAST_CONFIG_PHYSICAL_MAX_LEVEL                0x0001
#define ATTRID_LIGHTING_BALLAST_BALLAST_STATUS                           0x0002
/*** Ballast Status Attribute values (by bit number) ***/
#define LIGHTING_BALLAST_STATUS_NON_OPERATIONAL                          1 // bit 0 is set
#define LIGHTING_BALLAST_STATUS_LAMP_IS_NOT_IN_SOCKET                    2 // bit 1 is set
  // Ballast Settings attributes set
#define ATTRID_LIGHTING_BALLAST_MIN_LEVEL                                0x0010
#define ATTRID_LIGHTING_BALLAST_MAX_LEVEL                                0x0011
#define ATTRID_LIGHTING_BALLAST_POWER_ON_LEVEL                           0x0012
#define ATTRID_LIGHTING_BALLAST_POWER_ON_FADE_TIME                       0x0013
#define ATTRID_LIGHTING_BALLAST_INTRISTIC_BALLAST_FACTOR                 0x0014
#define ATTRID_LIGHTING_BALLAST_BALLAST_FACTOR_ADJUSTMENT                0x0015
  // Lamp Information attributes set
#define ATTRID_LIGHTING_BALLAST_LAMP_QUANTITY                            0x0020
  // Lamp Settings attributes set
#define ATTRID_LIGHTING_BALLAST_LAMP_TYPE                                0x0030
#define ATTRID_LIGHTING_BALLAST_LAMP_MANUFACTURER                        0x0031
#define ATTRID_LIGHTING_BALLAST_LAMP_RATED_HOURS                         0x0032
#define ATTRID_LIGHTING_BALLAST_LAMP_BURN_HOURS                          0x0033
#define ATTRID_LIGHTING_BALLAST_LAMP_ALARM_MODE                          0x0034
#define ATTRID_LIGHTING_BALLAST_LAMP_BURN_HOURS_TRIP_POINT               0x0035
/*** Lamp Alarm Mode attribute values  ***/
#define LIGHTING_BALLAST_LAMP_ALARM_MODE_BIT_0_NO_ALARM                  0
#define LIGHTING_BALLAST_LAMP_ALARM_MODE_BIT_0_ALARM                     1


/******************************************************************************/
/***            Logical Cluster ID - for mapping only                       ***/
/***           These are not to be used over-the-air                        ***/
/******************************************************************************/
#define ZCL_LIGHTING_LOGICAL_CLUSTER_ID_COLOR_CONTROL                    0x0030
#define ZCL_LIGHTING_LOGICAL_CLUSTER_ID_BALLAST_CONFIG                   0x0031


/*******************************************************************************
 * TYPEDEFS
 */

/*** ZCL Color Control Cluster: Move To Hue Cmd payload ***/
typedef struct
{
  uint8  hue;
  uint8  direction;
  uint16 transitionTime;
} zclCmdLightingMoveToHuePayload_t;

/*** ZCL Color Control Cluster: Move Hue Cmd payload ***/
typedef struct
{
  uint8 moveMode;
  uint8 rate;
} zclCmdLightingMoveHuePayload_t;

/*** ZCL Color Control Cluster: Step Hue Cmd payload ***/
typedef struct
{
  uint8 stepMode;
  uint8 transitionTime;
} zclCmdLightingStepHuePayload_t;

/*** ZCL Color Control Cluster: Move to Saturation Cmd payload ***/
typedef struct
{
  uint8  saturation;
  uint16 transitionTime;
} zclCmdLightingMoveToSaturationPayload_t;

/*** ZCL Color Control Cluster: Move Saturation Cmd payload ***/
typedef struct
{
  uint8 moveMode;
  uint8 rate;
} zclCmdLightingMoveSaturationPayload_t;

/*** ZCL Color Control Cluster: Step Saturation Cmd payload ***/
typedef struct
{
  uint8 stepMode;
  uint8 transitionTime;
} zclCmdLightingStepSaturationPayload_t;

/*** ZCL Color Control Cluster: Move To Hue and Saturation Cmd payload ***/
typedef struct
{
  uint8  hue;
  uint8  saturation;
  uint16 transitionTime;
} zclCmdLightingMoveToHueAndSaturationPayload_t;


// This callback is called to process a Move To Hue command
// hue - target hue value
// direction
// transitionTime - tame taken to move to the target hue in 1/10 sec increments
typedef void (*zclLighting_ColorControl_MoveToHue_t)( uint8 hue, uint8 direction, uint16 transitionTime );

// This callback is called to process a Move Hue command
// moveMode - LIGHTING_MOVE_HUE_STOP, LIGHTING_MOVE_HUE_UP, LIGHTING_MOVE_HUE_DOWN
// rate - the movement in steps per second, where step is a change in the device's hue of one unit
typedef void (*zclLighting_ColorControl_MoveHue_t)( uint8 moveMode, uint8 rate );

// This callback is called to process a Step Hue command
// stepMode -	LIGHTING_STEP_HUE_UP, LIGHTING_STEP_HUE_DOWN
// amount - number of hue units to step
// transitionTime - the movement in steps per 1/10 second
typedef void (*zclLighting_ColorControl_StepHue_t)( uint8 stepMode, uint8 amount, uint16 transitionTime );

// This callback is called to process a Move To Saturation command
// saturation - target saturation value
// transitionTime - time taken move to the target saturation, in 1/10 second units
typedef void (*zclLighting_ColorControl_MoveToSaturation_t)( uint8 saturation, uint16 transitionTime );

// This callback is called to process a Move Saturation command
// moveMode - LIGHTING_MOVE_SATURATION_STOP, LIGHTING_MOVE_SATURATION_UP, LIGHTING_MOVE_SATURATION_DOWN
// rate - rate of movement in step/sec; step is the device's saturation of one unit
typedef void (*zclLighting_ColorControl_MoveSaturation_t)( uint8 moveMode, uint8 rate );

// This callback is called to process a Step Saturation command
// stepMode -  LIGHTING_STEP_SATURATION_UP, LIGHTING_STEP_SATURATION_DOWN
// amount - number of units to change the saturation level by
// transitionTime - time to perform a single step in 1/10 of second
typedef void (*zclLighting_ColorControl_StepSaturation_t)( uint8 stepMode, uint8 amount, uint16 transitionTime );

// This callback is called to process a Move to Hue and Saturation command
// hue - a target hue
// saturation - a target saturation
// transitionTime -  time to move, equal of the value of the field in 1/10 seconds
typedef void (*zclLighting_ColorControl_MoveToHueAndSaturation_t)( uint8 hue, uint8 saturation, uint16 transitionTime );


// Register Callbacks table entry - enter function pointers for callbacks that
// the application would like to receive
typedef struct			
{
  zclLighting_ColorControl_MoveToHue_t                pfnColorControl_MoveToHue;
  zclLighting_ColorControl_MoveHue_t                  pfnColorControl_MoveHue;
  zclLighting_ColorControl_StepHue_t                  pfnColorControl_StepHue;
  zclLighting_ColorControl_MoveToSaturation_t         pfnColorControl_MoveToSaturation;
  zclLighting_ColorControl_MoveSaturation_t           pfnColorControl_MoveSaturation;
  zclLighting_ColorControl_StepSaturation_t           pfnColorControl_StepSaturation;
  zclLighting_ColorControl_MoveToHueAndSaturation_t   pfnColorControl_MoveToHueAndSaturation;

} zclLighting_AppCallbacks_t;


/******************************************************************************
 * FUNCTION MACROS
 */

/******************************************************************************
 * VARIABLES
 */

/******************************************************************************
 * FUNCTIONS
 */

/*
 * Register for callbacks from this cluster library
 */
extern ZStatus_t zclLighting_RegisterCmdCallbacks( uint8 endpoint, zclLighting_AppCallbacks_t *callbacks );


/*
 * Call to send out a Move To Hue Command
 *      hue - target hue value
 *      direction - direction of hue change
 *      transitionTime - tame taken to move to the target hue in 1/10 sec increments
 */
extern ZStatus_t zclLighting_ColorControl_Send_MoveToHueCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                             uint8 hue, uint8 direction, 
                                                             uint16 transitionTime, uint8 seqNum );

/*
 * Call to send out a Move Hue Command
 *      moveMode - LIGHTING_MOVE_HUE_STOP, LIGHTING_MOVE_HUE_UP, LIGHTING_MOVE_HUE_DOWN
 *      rate - the movement in steps per second (step is a change in the device's hue 
 *             of one unit)
 */
extern ZStatus_t zclLighting_ColorControl_Send_MoveHueCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                           uint8 moveMode, uint8 rate, uint8 seqNum );

/*
 * Call to send out a Step Hue Command
 *      stepMode - LIGHTING_STEP_HUE_UP, LIGHTING_STEP_HUE_DOWN
 *      amount - number of hue units to step
 *      transitionTime - the movement in steps per 1/10 second
 */
extern ZStatus_t zclLighting_ColorControl_Send_StepHueCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                           uint8 stepMode, uint8 amount, 
                                                           uint16 transitionTime, uint8 seqNum );

/*
 * Call to send out a Move To Saturation Command
 *      saturation - target saturation value
 *      transitionTime - time taken move to the target saturation, in 1/10 second units
 */
extern ZStatus_t zclLighting_ColorControl_Send_MoveToSaturationCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                uint8 saturation, uint16 transitionTime, uint8 seqNum );

/*
 * Call to send out a Move Saturation Command
 *      moveMode - LIGHTING_MOVE_SATURATION_STOP, LIGHTING_MOVE_SATURATION_UP, 
 *                 LIGHTING_MOVE_SATURATION_DOWN
 *      rate -  rate of movement in step per second; step is the device's 
 *              saturation of one unit
 */
extern ZStatus_t zclLighting_ColorControl_Send_MoveSaturationCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                                  uint8 moveMode, uint8 rate, uint8 seqNum );

/*
 * Call to send out a Step Saturation Command
 *      stepMode -  LIGHTING_STEP_SATURATION_UP, LIGHTING_STEP_SATURATION_DOWN
 *      amount -  number of units to change the saturation level by
 *      transitionTime - time to perform a single step in 1/10 of second
 */
extern ZStatus_t zclLighting_ColorControl_Send_StepSaturationCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                                  uint8 stepMode, uint8 amount, 
                                                                  uint16 transitionTime, uint8 seqNum );

/*
 * Call to send out a Move To Hue And Saturation  Command
 *      hue - 	target hue
 *      saturation -  target saturation
 *      transitionTime -  time to move, equal of the value of the field in 1/10 seconds
 */
extern ZStatus_t zclLighting_ColorControl_Send_MoveToHueAndSaturationCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                                          uint8 hue, uint8 saturation, 
                                                                          uint16 transitionTime, uint8 seqNum );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZCL_LIGHTING_H */
