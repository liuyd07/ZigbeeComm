/*********************************************************************
    Filename:       zcl_hvac.c
    Revised:        $Date: 2006-04-03 16:28:25 -0700 (Mon, 03 Apr 2006) $
    Revision:       $Revision: 10363 $

    Description:

    Zigbee Cluster Library - HVAC

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
#include "zcl_hvac.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */
typedef struct zclHVACCBRec
{
  struct zclHVACCBRec     *next;
  uint8                   endpoint; // Used to link it into the endpoint descriptor
  zclHVAC_AppCallbacks_t  *CBs;     // Pointer to Callback function
} zclHVACCBRec_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static zclHVACCBRec_t *zclHVACCBs = (zclHVACCBRec_t *)NULL;
static uint8 zclHVACPluginRegisted = FALSE;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static ZStatus_t zclHVAC_HdlIncoming( zclIncoming_t *msg, uint16 logicalClusterID );
static ZStatus_t zclHVAC_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID );
static zclHVAC_AppCallbacks_t *zclHVAC_FindCallbacks( uint8 endpoint );

static ZStatus_t zclHVAC_ProcessInPumpCmds( zclIncoming_t *pInMsg );
static ZStatus_t zclHVAC_ProcessInThermostatCmds( zclIncoming_t *pInMsg );

/*********************************************************************
 * @fn      zclHVAC_RegisterCmdCallbacks
 *
 * @brief   Register an applications command callbacks
 *
 * @param   endpoint - application's endpoint
 * @param   callbacks - pointer to the callback record.
 *
 * @return  ZMemError if not able to allocate
 */
ZStatus_t zclHVAC_RegisterCmdCallbacks( uint8 endpoint, zclHVAC_AppCallbacks_t *callbacks )
{
  zclHVACCBRec_t *pNewItem;
  zclHVACCBRec_t *pLoop;

  // Register as a ZCL Plugin
  if ( !zclHVACPluginRegisted )
  {
    zcl_registerPlugin( ZCL_HVAC_LOGICAL_CLUSTER_ID_PUMP_CONFIG_CONTROL,
                        ZCL_HVAC_LOGICAL_CLUSTER_ID_USER_INTERFACE_CONFIG,
                        zclHVAC_HdlIncoming );
    zclHVACPluginRegisted = TRUE;
  }

  // Fill in the new profile list
  pNewItem = osal_mem_alloc( sizeof( zclHVACCBRec_t ) );
  if ( pNewItem == NULL )
    return (ZMemError);

  pNewItem->next = (zclHVACCBRec_t *)NULL;
  pNewItem->endpoint = endpoint;
  pNewItem->CBs = callbacks;

  // Find spot in list
  if ( zclHVACCBs == NULL )
  {
    zclHVACCBs = pNewItem;
  }
  else
  {
    // Look for end of list
    pLoop = zclHVACCBs;
    while ( pLoop->next != NULL )
      pLoop = pLoop->next;

    // Put new item at end of list
    pLoop->next = pNewItem;
  }
  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zclHVAC_SendSetpointRaiseLower
 *
 * @brief   Call to send out a Setpoint Raise/Lower Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   mode - which setpoint is to be configured
 * @param   amount - amount setpoint(s) are to be increased (or decreased) by
 * @param   seqNum - transaction sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclHVAC_SendSetpointRaiseLower( uint8 srcEP, afAddrType_t *dstAddr,
                                          uint8 mode, uint8 amount, uint8 seqNum )
{
  uint8 buf[2];

  buf[0] = mode;
  buf[1] = amount;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_HVAC_LOGICAL_CLUSTER_ID_PUMP_CONFIG_CONTROL,
                          TRUE, COMMAND_THERMOSTAT_SETPOINT_RAISE_LOWER, TRUE, 
                          ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, 2, buf );
}

/*********************************************************************
 * @fn      zclHVAC_FindCallbacks
 *
 * @brief   Find the callbacks for an endpoint
 *
 * @param   endpoint
 *
 * @return  pointer to the callbacks
 */
static zclHVAC_AppCallbacks_t *zclHVAC_FindCallbacks( uint8 endpoint )
{
  zclHVACCBRec_t *pCBs;
  pCBs = zclHVACCBs;
  while ( pCBs )
  {
    if ( pCBs->endpoint == endpoint )
      return ( pCBs->CBs );
  }
  return ( (zclHVAC_AppCallbacks_t *)NULL );
}

/*********************************************************************
 * @fn      zclHVAC_HdlIncoming
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
static ZStatus_t zclHVAC_HdlIncoming( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  if ( zcl_ClusterCmd( pInMsg->hdr.fc.type ) )
  {
    // Is this a manufacturer specific command?
    if ( pInMsg->hdr.fc.manuSpecific == 0 ) 
    {
      stat = zclHVAC_HdlInSpecificCommands( pInMsg, logicalClusterID );
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
 * @fn      zclHVAC_HdlInSpecificCommands
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library

 * @param   pInMsg - pointer to the incoming message
 * @param   logicalClusterID
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclHVAC_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  switch ( logicalClusterID )				
  {
    case ZCL_HVAC_LOGICAL_CLUSTER_ID_PUMP_CONFIG_CONTROL:
      stat = zclHVAC_ProcessInPumpCmds( pInMsg );
      break;

    case ZCL_HVAC_LOGICAL_CLUSTER_ID_THERMOSTAT:
      stat = zclHVAC_ProcessInThermostatCmds( pInMsg );
      break;
 
    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclHVAC_ProcessInPumpCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis

 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclHVAC_ProcessInPumpCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZFailure;

  // there are no specific command for this cluster yet.
  // instead of suppressing a compiler warnings( for a code porting reasons )
  // fake unused call here and keep the code skeleton intact
  if ( stat != ZFailure )
    zclHVAC_FindCallbacks( 0 );

  return ( stat );
}

/*********************************************************************
 * @fn      zclHVAC_ProcessInThermostatCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis

 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclHVAC_ProcessInThermostatCmds( zclIncoming_t *pInMsg )
{
  zclHVAC_AppCallbacks_t *pCBs;

  if  ( pInMsg->hdr.commandID != COMMAND_THERMOSTAT_SETPOINT_RAISE_LOWER )
    return (ZFailure);   // Error ignore the command

  pCBs = zclHVAC_FindCallbacks( pInMsg->msg->endPoint );
  if ( pCBs && pCBs->pfnHVAC_SetpointRaiseLower )
    pCBs->pfnHVAC_SetpointRaiseLower( pInMsg->pData[0], pInMsg->pData[1] );
  
  return ( ZSuccess );
}

/*********************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */


/****************************************************************************
****************************************************************************/

