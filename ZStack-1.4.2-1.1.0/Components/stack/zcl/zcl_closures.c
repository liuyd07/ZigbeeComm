/*********************************************************************
    Filename:       zcl_closures.c
    Revised:        $Date: 2006-04-03 16:27:20 -0700 (Mon, 03 Apr 2006) $
    Revision:       $Revision: 10362 $

    Description:

    Zigbee Cluster Library - Closures.
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
#include "zcl_closures.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

typedef struct zclClosuresCBRec
{
  struct zclClosuresCBRec     *next;
  uint8                       endpoint; // Used to link it into the endpoint descriptor
  zclClosures_AppCallbacks_t  *CBs;     // Pointer to Callback function
} zclClosuresCBRec_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static zclClosuresCBRec_t *zclClosuresCBs = (zclClosuresCBRec_t *)NULL;
static uint8 zclClosuresPluginRegisted = FALSE;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static ZStatus_t zclClosures_HdlIncoming( zclIncoming_t *msg, uint16 logicalClusterID );
static ZStatus_t zclClosures_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID );
static zclClosures_AppCallbacks_t *zclClosures_FindCallbacks( uint8 endpoint );
static ZStatus_t zclClosures_ProcessInClosuresCmds( zclIncoming_t *pInMsg );

/*********************************************************************
 * @fn      zclClosures_RegisterCmdCallbacks
 *
 * @brief   Register an applications command callbacks
 *
 * @param   endpoint - application's endpoint
 * @param   callbacks - pointer to the callback record.
 *
 * @return  ZMemError if not able to allocate
 */
ZStatus_t zclClosures_RegisterCmdCallbacks( uint8 endpoint, zclClosures_AppCallbacks_t *callbacks )
{
  zclClosuresCBRec_t *pNewItem;
  zclClosuresCBRec_t *pLoop;

  // Register as a ZCL Plugin
  if ( !zclClosuresPluginRegisted )
  {
    zcl_registerPlugin( ZCL_CLOSURES_LOGICAL_CLUSTER_ID_SHADE_CONFIG,
                        ZCL_CLOSURES_LOGICAL_CLUSTER_ID_SHADE_CONFIG,
                        zclClosures_HdlIncoming );
    zclClosuresPluginRegisted = TRUE;
  }

  // Fill in the new profile list
  pNewItem = osal_mem_alloc( sizeof( zclClosuresCBRec_t ) );
  if ( pNewItem == NULL )
    return (ZMemError);

  pNewItem->next = (zclClosuresCBRec_t *)NULL;
  pNewItem->endpoint = endpoint;
  pNewItem->CBs = callbacks;

  // Find spot in list
  if ( zclClosuresCBs == NULL )
  {
    zclClosuresCBs = pNewItem;
  }
  else
  {
    // Look for end of list
    pLoop = zclClosuresCBs;
    while ( pLoop->next != NULL )
      pLoop = pLoop->next;

    // Put new item at end of list
    pLoop->next = pNewItem;
  }
  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zclClosures_FindCallbacks
 *
 * @brief   Find the callbacks for an endpoint
 *
 * @param   endpoint
 *
 * @return  pointer to the callbacks
 */
static zclClosures_AppCallbacks_t *zclClosures_FindCallbacks( uint8 endpoint )
{
  zclClosuresCBRec_t *pCBs;
  
  pCBs = zclClosuresCBs;
  while ( pCBs )
  {
    if ( pCBs->endpoint == endpoint )
      return ( pCBs->CBs );
  }
  return ( (zclClosures_AppCallbacks_t *)NULL );
}

/*********************************************************************
 * @fn      zclClosures_HdlIncoming
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
static ZStatus_t zclClosures_HdlIncoming( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  if ( zcl_ClusterCmd( pInMsg->hdr.fc.type ) )
  {
    // Is this a manufacturer specific command?
    if ( pInMsg->hdr.fc.manuSpecific == 0 ) 
    {
      stat = zclClosures_HdlInSpecificCommands( pInMsg, logicalClusterID );
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
 * @fn      zclClosures_HdlInSpecificCommands
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library

 * @param   pInMsg - pointer to the incoming message
 * @param   logicalClusterID
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclClosures_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  switch ( logicalClusterID )				
  {
    case ZCL_CLOSURES_LOGICAL_CLUSTER_ID_SHADE_CONFIG:
      stat = zclClosures_ProcessInClosuresCmds( pInMsg );
      break;

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSS_ProcessInClosuresCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis

 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclClosures_ProcessInClosuresCmds( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZFailure;

  // there are no specific command for this cluster yet.
  // instead of suppressing a compiler warnings( for a code porting reasons )
  // fake unused call here and keep the code skeleton intact
  if ( stat != ZFailure )
    zclClosures_FindCallbacks( 0 );

  return ( stat );
}



/********************************************************************************************
*********************************************************************************************/
