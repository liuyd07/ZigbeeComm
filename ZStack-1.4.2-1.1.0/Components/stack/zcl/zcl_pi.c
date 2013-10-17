/*********************************************************************
    Filename:       zcl_pi.c
    Revised:        $Date: 2006-04-03 16:28:25 -0700 (Mon, 03 Apr 2006) $
    Revision:       $Revision: 10363 $

    Description:

    Zigbee Cluster Library - Protocol Interface ( PI )

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
#include "zcl_pi.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */
typedef struct zclPICBRec
{
  struct zclPICBRec     *next;
  uint8                 endpoint; // Used to link it into the endpoint descriptor
  zclPI_AppCallbacks_t  *CBs;     // Pointer to Callback function
} zclPICBRec_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static zclPICBRec_t *zclPICBs = (zclPICBRec_t *)NULL;
static uint8 zclPIPluginRegisted = FALSE;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static ZStatus_t zclPI_HdlIncoming( zclIncoming_t *msg, uint16 logicalClusterID );
static ZStatus_t zclPI_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID );
static zclPI_AppCallbacks_t *zclPI_FindCallbacks( uint8 endpoint );

static ZStatus_t zclPI_ProcessIn_BACnetCmds( zclIncoming_t *pInMsg );


/*********************************************************************
 * @fn      zclPI_RegisterCmdCallbacks
 *
 * @brief   Register an applications command callbacks
 *
 * @param   endpoint - application's endpoint
 * @param   callbacks - pointer to the callback record.
 *
 * @return  ZMemError if not able to allocate
 */
ZStatus_t zclPI_RegisterCmdCallbacks( uint8 endpoint, zclPI_AppCallbacks_t *callbacks )
{
  zclPICBRec_t *pNewItem;
  zclPICBRec_t *pLoop;

  // Register as a ZCL Plugin
  if ( !zclPIPluginRegisted )
  {
    zcl_registerPlugin( ZCL_PI_LOGICAL_CLUSTER_ID_BACNET_INTERFACE,
                        ZCL_PI_LOGICAL_CLUSTER_ID_BACNET_INTERFACE,
                        zclPI_HdlIncoming );
    zclPIPluginRegisted = TRUE;
  }

  // Fill in the new profile list
  pNewItem = osal_mem_alloc( sizeof( zclPICBRec_t ) );
  if ( pNewItem == NULL )
    return (ZMemError);

  pNewItem->next = (zclPICBRec_t *)NULL;
  pNewItem->endpoint = endpoint;
  pNewItem->CBs = callbacks;

  // Find spot in list
  if ( zclPICBs == NULL )
  {
    zclPICBs = pNewItem;
  }
  else
  {
    // Look for end of list
    pLoop = zclPICBs;
    while ( pLoop->next != NULL )
      pLoop = pLoop->next;

    // Put new item at end of list
    pLoop->next = pNewItem;
  }
  return ( ZSuccess );
}

/*******************************************************************************
 * @fn      zclPI_Send_RawBACnetDataCmd
 *
 * @brief   Call to send out a Raw BACnet Data Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   len - length of BACnet data
 * @param   BACnet - BACnet data
 *
 * @return  ZStatus_t
 */
ZStatus_t zclPI_Send_RawBACnetDataCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                       uint8 len, uint8 *BACnet, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t stat;

  len++; // 1 for length field
  buf = osal_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    
    *pBuf++ = len;
    osal_memcpy( pBuf, BACnet, len );

    stat = zcl_SendCommand( srcEP, dstAddr, ZCL_PI_LOGICAL_CLUSTER_ID_BACNET_INTERFACE,
                            TRUE, COMMAND_PI_RAW_BACNET_DATA, TRUE, 
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, len, buf );
    osal_mem_free( buf );
  }
  else
    stat = ZFailure;
  
  return ( stat );
}

/*********************************************************************
 * @fn      zclPI_FindCallbacks
 *
 * @brief   Find the callbacks for an endpoint
 *
 * @param   endpoint
 *
 * @return  pointer to the callbacks
 */
static zclPI_AppCallbacks_t *zclPI_FindCallbacks( uint8 endpoint )
{
  zclPICBRec_t *pCBs;
  
  pCBs = zclPICBs;
  while ( pCBs )
  {
    if ( pCBs->endpoint == endpoint )
      return ( pCBs->CBs );
  }
  return ( (zclPI_AppCallbacks_t *)NULL );
}

/*********************************************************************
 * @fn      zclPI_HdlIncoming
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
static ZStatus_t zclPI_HdlIncoming( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  if ( zcl_ClusterCmd( pInMsg->hdr.fc.type ) )
  {
    // Is this a manufacturer specific command?
    if ( pInMsg->hdr.fc.manuSpecific == 0 ) 
    {
      stat = zclPI_HdlInSpecificCommands( pInMsg, logicalClusterID );
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
 * @fn      zclPI_HdlInSpecificCommands
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   logicalClusterID
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclPI_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  switch ( logicalClusterID )				
  {
    case ZCL_PI_LOGICAL_CLUSTER_ID_BACNET_INTERFACE:
      stat = zclPI_ProcessIn_BACnetCmds( pInMsg );
      break;

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclPI_ProcessIn_BACnetCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclPI_ProcessIn_BACnetCmds( zclIncoming_t *pInMsg )
{
  zclPI_AppCallbacks_t *pCBs;

  if  ( pInMsg->hdr.commandID != COMMAND_PI_RAW_BACNET_DATA )
    return (ZFailure);   // Error ignore the command

  pCBs = zclPI_FindCallbacks( pInMsg->msg->endPoint );
  if ( pCBs && pCBs->pfnPI_BACnet )
    pCBs->pfnPI_BACnet( pInMsg->pData[0], &(pInMsg->pData[1]) );
  
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

