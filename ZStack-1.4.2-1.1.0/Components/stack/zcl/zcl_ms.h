#ifndef ZCL_MS_H
#define ZCL_MS_H

/*********************************************************************
    Filename:       zcl_ms.h
    Revised:        $Date: 2006-03-28 16:28:48 -0800 (Tue, 28 Mar 2006) $
    Revision:       $Revision: 10266 $

    Description:

       This file contains the ZCL Measurement and Sensing Definitions

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


/*****************************************************************************/
/***    Illuminance Measurement Cluster Attributes                         ***/
/*****************************************************************************/
    // Illuminance Measurement Information attribute set
#define ATTRID_MS_ILLUMINANCE_MEASURED_VALUE                             0x0000
#define ATTRID_MS_ILLUMINANCE_MIN_MEASURED_VALUE                         0x0001
#define ATTRID_MS_ILLUMINANCE_MAX_MEASURED_VALUE                         0x0002
#define ATTRID_MS_ILLUMINANCE_TOLERANCE                                  0x0003
#define ATTRID_MS_ILLUMINANCE_LIGHT_SENSOR_TYPE                          0x0004
    // Illuminance Measurement Settings attribute set
// #define ATTRID_MS_ILLUMINANCE_MIN_PERCENT_CHANGE                         0x0100
// #define ATTRID_MS_ILLUMINANCE_MIN_ABSOLUTE_CHANGE                        0x0101
  /*** Light Sensor Type attribute values ***/
#define MS_ILLUMINANCE_LIGHT_SENSOR_PHOTODIODE                           0x00
#define MS_ILLUMINANCE_LIGHT_SENSOR_CMOS                                 0x01
#define MS_ILLUMINANCE_LIGHT_SENSOR_UNKNOWN                              0xFF
  
/*****************************************************************************/
/***    Illuminance Level Sensing Configuration Cluster Attributes         ***/
/*****************************************************************************/
    // Illuminance Level Sensing Information attribute set
#define ATTRID_MS_ILLUMINANCE_LEVEL_STATUS                               0x0000
#define ATTRID_MS_ILLUMINANCE_LEVEL_LIGHT_SENSOR_TYPE                    0x0001
/***  Level Status attribute values  ***/
#define MS_ILLUMINANCE_LEVEL_ON_TARGET                                   0x00
#define MS_ILLUMINANCE_LEVEL_BELOW_TARGET                                0x01
#define MS_ILLUMINANCE_LEVEL_ABOVE_TARGET                                0x02
/***  Light Sensor Type attribute values  ***/
#define MS_ILLUMINANCE_LEVEL_LIGHT_SENSOR_PHOTODIODE                     0x00
#define MS_ILLUMINANCE_LEVEL_LIGHT_SENSOR_CMOS                           0x01
#define MS_ILLUMINANCE_LEVEL_LIGHT_SENSOR_UNKNOWN                        0xFF
    // Illuminance Level Sensing Settings attribute set
#define ATTRID_MS_ILLUMINANCE_TARGET_LEVEL                               0x0010

/*****************************************************************************/
/***    Temperature Measurement Cluster Attributes                         ***/
/*****************************************************************************/
  // Temperature Measurement Information attributes set
#define ATTRID_MS_TEMPERATURE_MEASURED_VALUE                             0x0000
#define ATTRID_MS_TEMPERATURE_MIN_MEASURED_VALUE                         0x0001
#define ATTRID_MS_TEMPERATURE_MAX_MEASURED_VALUE                         0x0002
#define ATTRID_MS_TEMPERATURE_TOLERANCE                                  0x0003
  // Temperature Measurement Settings attributes set
#define ATTRID_MS_TEMPERATURE_MIN_PERCENT_CHANGE                         0x0010
#define ATTRID_MS_TEMPERATURE_MIN_ABSOLUTE_CHANGE                        0x0011

/*****************************************************************************/
/***    Pressure Measurement Cluster Attributes                            ***/
/*****************************************************************************/
  // Pressure Measurement Information attribute set
#define ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE                    0x0000
#define ATTRID_MS_PRESSURE_MEASUREMENT_MIN_MEASURED_VALUE                0x0001
#define ATTRID_MS_PRESSURE_MEASUREMENT_MAX_MEASURED_VALUE                0x0002
#define ATTRID_MS_PRESSURE_MEASUREMENT_TOLERANCE                         0x0003
  // Pressure Measurement Settings attribute set
// #define ATTRID_MS_PRESSURE_MEASUREMENT_MIN_PERCENT_CHANGE                0x0100
// #define ATTRID_MS_PRESSURE_MEASUREMENT_MIN_ABSOLUTE_CHANGE               0x0101

/*****************************************************************************/
/***        Flow Measurement Cluster Attributes                            ***/
/*****************************************************************************/
  // Flow Measurement Information attribute set
#define ATTRID_MS_FLOW_MEASUREMENT_MEASURED_VALUE                        0x0000
#define ATTRID_MS_FLOW_MEASUREMENT_MIN_MEASURED_VALUE                    0x0001
#define ATTRID_MS_FLOW_MEASUREMENT_MAX_MEASURED_VALUE                    0x0002
#define ATTRID_MS_FLOW_MEASUREMENT_TOLERANCE                             0x0003
  // Flow Measurement Settings attribute set
// #define ATTRID_MS_FLOW_MEASUREMENT_MIN_PERCENT_CHANGE                    0x0100
// #define ATTRID_MS_FLOW_MEASUREMENT_MIN_ABSOLUTE_CHANGE                   0x0101

/*****************************************************************************/
/***        Relative Humidity Cluster Attributes                           ***/
/*****************************************************************************/
  // Relative Humidity Information attribute set
#define ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE                       0x0000
#define ATTRID_MS_RELATIVE_HUMIDITY_MIN_MEASURED_VALUE                   0x0001
#define ATTRID_MS_RELATIVE_HUMIDITY_MAX_MEASURED_VALUE                   0x0002
#define ATTRID_MS_RELATIVE_HUMIDITY_TOLERANCE                            0x0003

/*****************************************************************************/
/***         Occupancy Sensing Cluster Attributes                          ***/
/*****************************************************************************/
    // Occupancy Sensor Configuration attribute set
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY                     0x0000
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY_SENSOR_TYPE         0x0001
/*** Occupancy Sensor Type Attribute values ***/
#define MS_OCCUPANCY_SENSOR_TYPE_PIR                                     0x00
#define MS_OCCUPANCY_SENSOR_TYPE_ULTRASONIC                              0x01
#define MS_OCCUPANCY_SENSOR_TYPE_PIR_AND_ULTRASONIC                      0x02
    // PIR Configuration attribute set
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_PIR_O_TO_U_DELAY              0x0010
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_PIR_U_TO_O_DELAY              0x0011
    // Ultrasonic Configuration attribute set
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_ULTRASONIC_O_TO_U_DELAY       0x0020
#define ATTRID_MS_OCCUPANCY_SENSING_CONFIG_ULTRASONIC_U_TO_O_DELAY       0x0021


/*****************************************************************************/
/***            Logical Cluster ID - for mapping only                      ***/
/***            These are not to be used over-the-air                      ***/
/*****************************************************************************/
#define ZCL_MS_LOGICAL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT                0x0040
#define ZCL_MS_LOGICAL_CLUSTER_ID_ILLUMINANCE_LEVEL_SENSING_CFG          0x0041
#define ZCL_MS_LOGICAL_CLUSTER_ID_TEMPERATURE_MEASUREMENT                0x0042
#define ZCL_MS_LOGICAL_CLUSTER_ID_PRESSURE_MEASUREMENT                   0x0043
#define ZCL_MS_LOGICAL_CLUSTER_ID_FLOW_MEASUREMENT                       0x0044
#define ZCL_MS_LOGICAL_CLUSTER_ID_RELATIVE_HUMIDITY                      0x0045 
#define ZCL_MS_LOGICAL_CLUSTER_ID_OCCUPANCY_SENSING                      0x0046


/************************************************************************************
 * MACROS
 */


/****************************************************************************
 * TYPEDEFS
 */

typedef void (*zclMS_PlaceHolder_t)( void );

// Register Callbacks table entry - enter function pointers for callbacks that
// the application would like to receive
typedef struct			
{
  zclMS_PlaceHolder_t               pfnMSPlaceHolder; // Place Holder
//  NULL
} zclMS_AppCallbacks_t;


/****************************************************************************
 * VARIABLES
 */


/****************************************************************************
 * FUNCTIONS
 */

 /*
  * Register for callbacks from this cluster library
  */
extern ZStatus_t zclMS_RegisterCmdCallbacks( uint8 endpoint, zclMS_AppCallbacks_t *callbacks );


#ifdef __cplusplus
}
#endif

#endif /* ZCL_MS_H */
