/*********************************************************************
    Filename:       zcl_ms.c
    Revised:        $Date: 2006-04-03 16:28:25 -0700 (Mon, 03 Apr 2006) $
    Revision:       $Revision: 10363 $

    Description:

    Zigbee Cluster Library - Measurements and Sensing ( MS )

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved. Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/


/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */
typedef struct zclMSCBRec
{
  struct zclMSCBRec     *next;
  uint8                 endpoint; // Used to link it into the endpoint descriptor
  zclMS_AppCallbacks_t  *CBs;     // Pointer to Callback function
} zclMSCBRec_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static zclMSCBRec_t *zclMSCBs = (zclMSCBRec_t *)NULL;
static uint8 zclMSPluginRegisted = FALSE;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static ZStatus_t zclMS_HdlIncoming( zclIncoming_t *msg, uint16 logicalClusterID );
static ZStatus_t zclMS_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID );
static zclMS_AppCallbacks_t *zclMS_FindCallbacks( uint8 endpoint );

static ZStatus_t zclMS_ProcessIn_IlluminanceMeasurementCmds( zclIncoming_t *pInMsg );
static ZStatus_t zclMS_ProcessIn_IlluminanceLevelSensingCmds( zclIncoming_t *pInMsg );
static ZStatus_t zclMS_ProcessIn_TemperatureMeasurementCmds( zclIncoming_t *pInMsg );
static ZStatus_t zclMS_ProcessIn_PressureMeasurementCmds( zclIncoming_t *pInMsg );
static ZStatus_t zclMS_ProcessIn_FlowMeasurementCmds( zclIncoming_t *pInMsg );
static ZStatus_t zclMS_ProcessIn_RelativeHumidityCmds( zclIncoming_t *pInMsg );
static ZStatus_t zclMS_ProcessIn_OccupancySensingCmds( zclIncoming_t *pInMsg );

/*********************************************************************
 * @fn      zclMS_RegisterCmdCallbacks
 *
 * @brief   Register an applications command callbacks
 *
 * @param   endpoint - application's endpoint
 * @param   callbacks - pointer to the callback record.
 *
 * @return  ZMemError if not able to allocate
 */
ZStatus_t zclMS_RegisterCmdCallbacks( uint8 endpoint, zclMS_AppCallbacks_t *callbacks )
{
  zclMSCBRec_t *pNewItem;
  zclMSCBRec_t *pLoop;

  // Register as a ZCL Plugin
  if ( !zclMSPluginRegisted )
  {
    zcl_registerPlugin( ZCL_MS_LOGICAL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                        ZCL_MS_LOGICAL_CLUSTER_ID_OCCUPANCY_SENSING,
                        zclMS_HdlIncoming );
    zclMSPluginRegisted = TRUE;
  }

  // Fill in the new profile list
  pNewItem = osal_mem_alloc( sizeof( zclMSCBRec_t ) );
  if ( pNewItem == NULL )
    return (ZMemError);

  pNewItem->next = (zclMSCBRec_t *)NULL;
  pNewItem->endpoint = endpoint;
  pNewItem->CBs = callbacks;

  // Find spot in list
  if ( zclMSCBs == NULL )
  {
    zclMSCBs = pNewItem;
  }
  else
  {
    // Look for end of list
    pLoop = zclMSCBs;
    while ( pLoop->next != NULL )
      pLoop = pLoop->next;

    // Put new item at end of list
    pLoop->next = pNewItem;
  }
  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zclMS_FindCallbacks
 *
 * @brief   Find the callbacks for an endpoint
 *
 * @param   endpoint
 *
 * @return  pointer to the callbacks
 */
static zclMS_AppCallbacks_t *zclMS_FindCallbacks( uint8 endpoint )
{
  zclMSCBRec_t *pCBs;
  
  pCBs = zclMSCBs;
  while ( pCBs )
  {
    if ( pCBs->endpoint == endpoint )
      return ( pCBs->CBs );
  }
  return ( (zclMS_AppCallbacks_t *)NULL );
}

/*********************************************************************
 * @fn      zclMS_HdlIncoming
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library or Profile commands for attributes
 *          that aren't in the attribute list
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   logicalClusterID
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_HdlIncoming( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  if ( zcl_ClusterCmd( pInMsg->hdr.fc.type ) )
  {
    // Is this a manufacturer specific command?
    if ( pInMsg->hdr.fc.manuSpecific == 0 ) 
    {
      stat = zclMS_HdlInSpecificCommands( pInMsg, logicalClusterID );
    }
    else
    {
      // We don't support any manufacturer specific command -- ignore it.
      stat = ZFailure;
    }
  }
  else
  {
    // Handle all the normal (Read, Write...) commands
    stat = ZFailure;
  }
  return ( stat );
}

/*********************************************************************
 * @fn      zclMS_HdlInSpecificCommands
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   logicalClusterID
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  switch ( logicalClusterID )				
  {

    case ZCL_MS_LOGICAL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT:
      stat = zclMS_ProcessIn_IlluminanceMeasurementCmds( pInMsg );
      break;

    case ZCL_MS_LOGICAL_CLUSTER_ID_ILLUMINANCE_LEVEL_SENSING_CFG:
      stat = zclMS_ProcessIn_IlluminanceLevelSensingCmds( pInMsg );
      break;

    case ZCL_MS_LOGICAL_CLUSTER_ID_TEMPERATURE_MEASUREMENT:
      stat = zclMS_ProcessIn_TemperatureMeasurementCmds( pInMsg );
      break;


    case ZCL_MS_LOGICAL_CLUSTER_ID_PRESSURE_MEASUREMENT:
      stat = zclMS_ProcessIn_PressureMeasurementCmds( pInMsg );
      break;

    case ZCL_MS_LOGICAL_CLUSTER_ID_FLOW_MEASUREMENT:
      stat = zclMS_ProcessIn_FlowMeasurementCmds( pInMsg );
      break;

    case ZCL_MS_LOGICAL_CLUSTER_ID_RELATIVE_HUMIDITY:
      stat = zclMS_ProcessIn_RelativeHumidityCmds( pInMsg );
      break;
      
    case ZCL_MS_LOGICAL_CLUSTER_ID_OCCUPANCY_SENSING:
      stat = zclMS_ProcessIn_OccupancySensingCmds( pInMsg );
      break;

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclMS_ProcessIn_IlluminanceMeasurementCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_ProcessIn_IlluminanceMeasurementCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZFailure;

  // there are no specific command for this cluster yet.
  // instead of suppressing a compiler warnings( for a code porting reasons )
  // fake unused call here and keep the code skeleton intact
  if ( stat != ZFailure )
    zclMS_FindCallbacks( 0 );

  return ( stat );
}

/*********************************************************************
 * @fn      zclMS_ProcessIn_IlluminanceLevelSensingCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_ProcessIn_IlluminanceLevelSensingCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZSuccess;
  uint8 cmdID;

  cmdID = pInMsg->hdr.commandID;

  switch ( cmdID )				
  {

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclMS_ProcessIn_TemperatureMeasurementCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_ProcessIn_TemperatureMeasurementCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZSuccess;
  uint8 cmdID;

  cmdID = pInMsg->hdr.commandID;

  switch ( cmdID )				
  {

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclMS_ProcessIn_PressureMeasurementCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_ProcessIn_PressureMeasurementCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZSuccess;
  uint8 cmdID;

  cmdID = pInMsg->hdr.commandID;

  switch ( cmdID )				
  {
    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclMS_ProcessIn_FlowMeasurementCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_ProcessIn_FlowMeasurementCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZSuccess;
  uint8 cmdID;

  cmdID = pInMsg->hdr.commandID;

  switch ( cmdID )				
  {

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclMS_ProcessIn_RelativeHumidityCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_ProcessIn_RelativeHumidityCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZSuccess;
  uint8 cmdID;

  cmdID = pInMsg->hdr.commandID;

  switch ( cmdID )				
  {

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclMS_ProcessIn_OccupancySensingCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclMS_ProcessIn_OccupancySensingCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZSuccess;
  uint8 cmdID;

  cmdID = pInMsg->hdr.commandID;

  switch ( cmdID )				
  {

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */



/****************************************************************************
****************************************************************************/

