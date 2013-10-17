/*********************************************************************
    Filename:       zcl_general.c
    Revised:        $Date: 2007-03-14 09:39:37 -0700 (Wed, 14 Mar 2007) $
    Revision:       $Revision: 13756 $

    Description:

    Zigbee Cluster Library - General.  This application receives all ZCL
    messages and initially parses them before passing to application.

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
#include "OSAL_Nv.h"
#include "zcl.h"
#include "zcl_ha.h"
#include "zcl_general.h"
#include "ZDApp.h"

/*********************************************************************
 * MACROS
 */
#define locationTypeAbsolute( a )          ( (a) & LOCATION_TYPE_ABSOLUTE )
#define locationType2D( a )                ( (a) & LOCATION_TYPE_2_D )
#define locationTypeCoordinateSystem( a )  ( (a) & LOCATION_TYPE_COORDINATE_SYSTEM )

#ifdef ZCL_SCENES
#define zclGeneral_ScenesRemaingCapacity() ( ZCL_GEN_MAX_SCENES - zclGeneral_CountAllScenes() )
#endif // ZCL_SCENES

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */
typedef struct zclGenCBRec
{
  struct zclGenCBRec        *next;
  uint8                     endpoint; // Used to link it into the endpoint descriptor
  zclGeneral_AppCallbacks_t *CBs;     // Pointer to Callback function
} zclGenCBRec_t;

typedef struct zclGenSceneItem
{
  struct zclGenSceneItem    *next;
  uint8                     endpoint; // Used to link it into the endpoint descriptor
  zclGeneral_Scene_t        scene;    // Scene info
} zclGenSceneItem_t;

typedef struct zclGenAlarmItem
{
  struct zclGenAlarmItem    *next;
  uint8                     endpoint; // Used to link it into the endpoint descriptor
  zclGeneral_Alarm_t        alarm;    // Alarm info
} zclGenAlarmItem_t;

// Scene NV types
typedef struct
{
  uint16                    numRecs;
} nvGenScenesHdr_t;

typedef struct zclGenSceneNVItem
{
  uint8                     endpoint;
  zclGeneral_Scene_t        scene;
} zclGenSceneNVItem_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static zclGenCBRec_t *zclGenCBs = (zclGenCBRec_t *)NULL;
static uint8 zclGenPluginRegisted = FALSE;
#ifdef ZCL_SCENES
static zclGenSceneItem_t *zclGenSceneTable = (zclGenSceneItem_t *)NULL;
#endif // ZCL_SCENES
#ifdef ZCL_ALARMS
static zclGenAlarmItem_t *zclGenAlarmTable = (zclGenAlarmItem_t *)NULL;
#endif // ZCL_ALARMS

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static ZStatus_t zclGeneral_HdlIncoming( zclIncoming_t *msg, uint16 logicalClusterID );
static ZStatus_t zclGeneral_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID );

#if defined(ZCL_BASIC) || defined(ZCL_IDENTIFY) || defined(ZCL_GROUPS) || defined(ZCL_SCENES) || \
    defined(ZCL_ON_OFF) || defined(ZCL_LEVEL_CTRL) || defined(ZCL_ALARMS) || defined(ZCL_LOCATION)
static zclGeneral_AppCallbacks_t *zclGeneral_FindCallbacks( uint8 endpoint );
#endif // any ZCL General command

// Device Configuration and Installation clusters
#ifdef ZCL_BASIC
static ZStatus_t zclGeneral_ProcessInBasic( zclIncoming_t *pInMsg );
#endif // ZCL_BASIC

#ifdef ZCL_IDENTIFY
static ZStatus_t zclGeneral_ProcessInIdentity( zclIncoming_t *pInMsg );
#endif // ZCL_IDENTIFY

// Groups and Scenes clusters
#ifdef ZCL_GROUPS
static ZStatus_t zclGeneral_ProcessInGroupsServer( zclIncoming_t *pInMsg );
static ZStatus_t zclGeneral_ProcessInGroupsClient( zclIncoming_t *pInMsg );
#endif // ZCL_GROUPS

#ifdef ZCL_SCENES
static ZStatus_t zclGeneral_ProcessInScenesServer( zclIncoming_t *pInMsg );
static ZStatus_t zclGeneral_ProcessInScenesClient( zclIncoming_t *pInMsg );
#endif // ZCL_SCENES

// On/Off and Level Control Configuration clusters
#ifdef ZCL_ON_OFF
static ZStatus_t zclGeneral_ProcessInOnOff( zclIncoming_t *pInMsg );
#endif // ZCL_ONOFF

#ifdef ZCL_LEVEL_CTRL
static ZStatus_t zclGeneral_ProcessInLevelControl( zclIncoming_t *pInMsg );
#endif // ZCL_LEVEL_CTRL

// Alarms cluster
#ifdef ZCL_ALARMS
static ZStatus_t zclGeneral_ProcessInAlarmsServer( zclIncoming_t *pInMsg );
static ZStatus_t zclGeneral_ProcessInAlarmsClient( zclIncoming_t *pInMsg );
#endif // ZCL_ALARMS

// Location cluster
#ifdef ZCL_LOCATION
static ZStatus_t zclGeneral_ProcessInLocationServer( zclIncoming_t *pInMsg );
static ZStatus_t zclGeneral_ProcessInLocationClient( zclIncoming_t *pInMsg );
#endif // ZCL_LOCATION

#ifdef ZCL_SCENES
static uint8 zclGeneral_ScenesInitNV( void );
static void zclGeneral_ScenesSetDefaultNV( void );
static void zclGeneral_ScenesWriteNV( void );
static uint16 zclGeneral_ScenesRestoreFromNV( void );
#endif // ZCL_SCENES

/*********************************************************************
 * @fn      zclGeneral_RegisterCmdCallbacks
 *
 * @brief   Register an applications command callbacks
 *
 * @param   endpoint - application's endpoint
 * @param   callbacks - pointer to the callback record.
 *
 * @return  ZMemError if not able to allocate
 */
ZStatus_t zclGeneral_RegisterCmdCallbacks( uint8 endpoint, zclGeneral_AppCallbacks_t *callbacks )
{
  zclGenCBRec_t *pNewItem;
  zclGenCBRec_t *pLoop;

  // Register as a ZCL Plugin
  if ( !zclGenPluginRegisted )
  {
    zcl_registerPlugin( ZCL_GEN_LOGICAL_CLUSTER_ID_BASIC,
                        ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION,
                        zclGeneral_HdlIncoming );
    
#ifdef ZCL_SCENES
    // Initialize NV items
    zclGeneral_ScenesInitNV();

    // Restore the Scene table
    zclGeneral_ScenesRestoreFromNV();
#endif // ZCL_SCENES
  
    zclGenPluginRegisted = TRUE;
  }

  // Fill in the new profile list
  pNewItem = osal_mem_alloc( sizeof( zclGenCBRec_t ) );
  if ( pNewItem == NULL )
    return (ZMemError);

  pNewItem->next = (zclGenCBRec_t *)NULL;
  pNewItem->endpoint = endpoint;
  pNewItem->CBs = callbacks;

  // Find spot in list
  if (  zclGenCBs == NULL )
  {
    zclGenCBs = pNewItem;
  }
  else
  {
    // Look for end of list
    pLoop = zclGenCBs;
    while ( pLoop->next != NULL )
      pLoop = pLoop->next;

    // Put new item at end of list
    pLoop->next = pNewItem;
  }

  return ( ZSuccess );
}

#ifdef ZCL_IDENTIFY
/*********************************************************************
 * @fn      zclGeneral_SendIdentifyQueryResponse
 *
 * @brief   Call to send out an Identify Query Response Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   timeout - how long the device will continue to identify itself (in seconds)
 * @param   seqNum - identification number for the transaction
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendIdentifyQueryResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                                uint16 timeout, uint8 seqNum )
{
  uint8 buf[2];

  buf[0] = LO_UINT16( timeout );
  buf[1] = HI_UINT16( timeout );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_IDENTIFY,
                          TRUE, COMMAND_IDENTIFY_QUERY_RSP, TRUE, 
                          ZCL_FRAME_SERVER_CLIENT_DIR, 0, seqNum, 2, buf );
}
#endif // ZCL_IDENTIFY

#ifdef ZCL_GROUPS
/*********************************************************************
 * @fn      zclGeneral_SendGroupRequest
 *
 * @brief   Send a Group Request to a device.  You can also use the
 *          appropriate macro.
 *
 * @param   srcEP - Sending Apps endpoint
 * @param   dstAddr - where to send the request
 * @param   cmd - one of the following:
 *              COMMAND_GROUP_VIEW
 *              COMMAND_GROUP_REMOVE
 * @param   groupID -
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendGroupRequest( uint8 srcEP, afAddrType_t *dstAddr,
                     uint8 cmd, uint16 groupID, uint8 seqNum )
{
  uint8 buf[2];

  buf[0] = LO_UINT16( groupID );
  buf[1] = HI_UINT16( groupID );

  return ( zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_GROUPS,
                            TRUE, cmd, TRUE, 
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, 2, buf ) );
}

/*********************************************************************
 * @fn      zclGeneral_SendAddGroupRequest
 *
 * @brief   Send the Add Group Request to a device
 *
 * @param   srcEP - Sending Apps endpoint
 * @param   dstAddr - where to send the request
 * @param   cmd - one of the following:
 *                COMMAND_GROUP_ADD
 *                COMMAND_GROUP_ADD_IF_IDENTIFYING
 * @param   groupID - pointer to the group structure
 * @param   groupName - pointer to Group Name.  This is a Zigbee
 *          string data type, so the first byte is the length of the
 *          name (in bytes), then the name.
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendAddGroupRequest( uint8 srcEP, afAddrType_t *dstAddr,
                                          uint8 cmd, uint16 groupID, 
                                          uint8 *groupName, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 len;
  ZStatus_t status;

  len = 2;    // Group ID
  len += groupName[0] + 1;  // String + 1 for length

  buf = osal_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    *pBuf++ = LO_UINT16( groupID );
    *pBuf++ = HI_UINT16( groupID );
    *pBuf++ = groupName[0]; // string length
    osal_memcpy( pBuf, &(groupName[1]), groupName[0] );

    status = zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_GROUPS,
                              TRUE, cmd, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR, 
                              0, seqNum, len, buf );
    osal_mem_free( buf );
  }
  else
    status = ZMemError;

  return ( status );
}

/*********************************************************************
 * @fn      zclGeneral_SendGroupGetMembershipRequest
 *
 * @brief   Send a Get Group Membership (Resposne) Command to a device
 *
 * @param   srcEP - Sending Apps endpoint
 * @param   dstAddr - where to send the request
 * @param   cmd - one of the following:
 *                COMMAND_GROUP_GET_MEMBERSHIP
 *                COMMAND_GROUP_GET_MEMBERSHIP_RSP
 * @param   groupID - pointer to the group structure
 * @param   groupName - pointer to Group Name.  This is a Zigbee
 *          string data type, so the first byte is the length of the
 *          name (in bytes), then the name.
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendGroupGetMembershipRequest( uint8 srcEP, afAddrType_t *dstAddr,
                              uint8 cmd, uint8 rspCmd, uint8 direction, uint8 capacity, 
                              uint8 grpCnt, uint16 *grpList, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 len = 0;
  uint8 i;
  ZStatus_t status;

  if ( rspCmd )
    len++;  // Capacity
 
  len++;  // Group Count
  len += sizeof ( uint16 ) * grpCnt;  // Group List

  buf = osal_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    if ( rspCmd )
      *pBuf++ = capacity;
  
    *pBuf++ = grpCnt;
    for ( i = 0; i < grpCnt; i++ )
    {
      *pBuf++ = LO_UINT16( grpList[i] );
      *pBuf++ = HI_UINT16( grpList[i] );
    }

    status = zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_GROUPS,
                        TRUE, cmd, TRUE, direction, 0, seqNum, len, buf );
    osal_mem_free( buf );
  }
  else
    status = ZMemError;

  return ( status );  
}  

/*********************************************************************
 * @fn      zclGeneral_SendGroupResponse
 *
 * @brief   Send Group Response (not Group View Response)
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   cmd - either COMMAND_GROUP_ADD_RSP or COMMAND_GROUP_REMOVE_RSP
 * @param   status - group command status
 * @param   groupID - what group
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendGroupResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                        uint8 cmd, uint8 status, uint16 groupID, 
                                        uint8 seqNum )
{
  uint8 buf[3];

  buf[0] = status;
  buf[1] = LO_UINT16( groupID );
  buf[2] = HI_UINT16( groupID );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_GROUPS, 
                          TRUE, cmd, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, 0, seqNum, 3, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendGroupViewResponse
 *
 * @brief   Call to send Group Response Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   cmd - either COMMAND_GROUP_ADD_RSP or COMMAND_GROUP_REMOVE_RSP
 * @param   status - group command status
 * @param   grp - group info
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendGroupViewResponse( uint8 srcEP, afAddrType_t *dstAddr,
                 uint8 status, aps_Group_t *grp, uint8 seqNum )
{
  uint8 buf[ 1 + sizeof( aps_Group_t ) ];
  uint8 len = 1 + 2; // Status + Group ID

  buf[0] = status;
  buf[1] = LO_UINT16( grp->ID );
  buf[2] = HI_UINT16( grp->ID );
  
  if ( status == ZCL_STATUS_SUCCESS )
  {
    buf[3] = grp->name[0]; // string length
    osal_memcpy( &buf[4], (&grp->name[1]), grp->name[0] );
    len += 1 + grp->name[0]; // length + string
  }

  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_GROUPS, 
                          TRUE, COMMAND_GROUP_VIEW_RSP, TRUE, 
                          ZCL_FRAME_SERVER_CLIENT_DIR, 0, seqNum, len, buf );
}
#endif // ZCL_GROUPS

#ifdef ZCL_SCENES
/*********************************************************************
 * @fn      zclGeneral_SendAddScene
 *
 * @brief   Send the Add Scene Request to a device
 *
 * @param   srcEP - Sending Apps endpoint
 * @param   dstAddr - where to send the request
 * @param   scene - pointer to the scene structure
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendAddScene( uint8 srcEP, afAddrType_t *dstAddr,
                      zclGeneral_Scene_t *scene, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 len;
  ZStatus_t status;

  len = 2 + 1 + 2;    // Group ID + Scene ID + transition time
  len += scene->name[0] + 1; // String + 1 for length
  
  // Add something for the extension field length
  len += scene->extLen;
 
  buf = osal_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    *pBuf++ = LO_UINT16( scene->groupID );
    *pBuf++ = HI_UINT16( scene->groupID );
    *pBuf++ = scene->ID;
    *pBuf++ = LO_UINT16( scene->transTime );
    *pBuf++ = HI_UINT16( scene->transTime );
    *pBuf++ = scene->name[0]; // string length
    osal_memcpy( pBuf, &(scene->name[1]), scene->name[0] );
    pBuf += scene->name[0]; // move pass name

    // Add the extension fields
    if ( scene->extLen > 0 )
      osal_memcpy( pBuf, scene->extField, scene->extLen );
     
    status = zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_SCENES, TRUE, 
                              COMMAND_SCENE_ADD, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
                              0, seqNum, len, buf );
    osal_mem_free( buf );
  }
  else
    status = ZMemError;

  return ( status );
}

/*********************************************************************
 * @fn      zclGeneral_SendSceneRequest
 *
 * @brief   Send a Scene Request to a device.  You can also use the
 *          appropriate macro.
 *
 * @param   srcEP - Sending Apps endpoint
 * @param   dstAddr - where to send the request
 * @param   cmd - one of the following:
 *              COMMAND_SCENE_VIEW
 *              COMMAND_SCENE_REMOVE
 *              COMMAND_SCENE_REMOVE_ALL
 *              COMMAND_SCENE_STORE
 *              COMMAND_SCENE_RECALL
 *              COMMAND_SCENE_GET_MEMBERSHIP
 * @param   groupID - group ID
 * @param   sceneID - scene ID (not applicable to COMMAND_SCENE_REMOVE_ALL and
 *                    COMMAND_SCENE_GET_MEMBERSHIP)
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendSceneRequest( uint8 srcEP, afAddrType_t *dstAddr,
                                       uint8 cmd, uint16 groupID, uint8 sceneID, 
                                       uint8 seqNum )
{
  uint8 buf[3];
  uint8 len = 2;

  buf[0] = LO_UINT16( groupID );
  buf[1] = HI_UINT16( groupID );
  
  if ( cmd != COMMAND_SCENE_REMOVE_ALL && cmd != COMMAND_SCENE_GET_MEMBERSHIP )
  {
    buf[2] = sceneID;
    len++;
  }

  return ( zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_SCENES,
                            TRUE, cmd, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR, 
                            0, seqNum, len, buf ) );
}

/*********************************************************************
 * @fn      zclGeneral_SendSceneResponse
 *
 * @brief   Send Group Response (not Group View Response)
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   cmd - either COMMAND_SCENE_ADD_RSP, COMMAND_SCENE_REMOVE_RSP
 *                COMMAND_SCENE_STORE_RSP, or COMMAND_SCENE_REMOVE_ALL_RSP
 * @param   status - scene command status
 * @param   groupID - what group
 * @param   sceneID - what scene (not applicable to COMMAND_SCENE_REMOVE_ALL_RSP)
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendSceneResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                  uint8 cmd, uint8 status, uint16 groupID, 
                                  uint8 sceneID, uint8 seqNum )
{
  uint8 buf[4];
  uint8 len = 1 + 2; // Status + Group ID

  buf[0] = status;
  buf[1] = LO_UINT16( groupID );
  buf[2] = HI_UINT16( groupID );
  
  if ( cmd != COMMAND_SCENE_REMOVE_ALL_RSP )
  {
    buf[3] = sceneID;
    len++;
  }

  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_SCENES,
                          TRUE, cmd, TRUE, ZCL_FRAME_SERVER_CLIENT_DIR, 
                          0, seqNum, len, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendSceneViewResponse
 *
 * @brief   Call to send Scene Response Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   status - scene command status
 * @param   scene - scene info
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendSceneViewResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                       uint8 status, zclGeneral_Scene_t *scene,
                                       uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 len = 1 + 2 + 1; // Status + Group ID + Scene ID
  ZStatus_t stat;
  
  if ( status == ZCL_STATUS_SUCCESS )
  {
    len += 2; // Transition Time
    len += scene->name[0] + 1; // string + 1 for length

    // Add something for the extension field length
    len += scene->extLen;
  }

  buf = osal_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    *pBuf++ = status;
    *pBuf++ = LO_UINT16( scene->groupID );
    *pBuf++ = HI_UINT16( scene->groupID );
    *pBuf++ = scene->ID;
    if ( status == ZCL_STATUS_SUCCESS )
    {
      *pBuf++ = LO_UINT16( scene->transTime );
      *pBuf++ = HI_UINT16( scene->transTime );
      *pBuf++ = scene->name[0]; // string length
      osal_memcpy( pBuf, &(scene->name[1]), scene->name[0] );
      pBuf += scene->name[0]; // move pass name
      
      // Add the extension fields
      if ( scene->extLen > 0 )
        osal_memcpy( pBuf, scene->extField, scene->extLen );
    }

    stat = zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_SCENES,
                            TRUE, COMMAND_SCENE_VIEW_RSP, TRUE, 
                            ZCL_FRAME_SERVER_CLIENT_DIR, 0, seqNum, len, buf );
    osal_mem_free( buf );
  }
  else
    stat = ZMemError;

  return ( stat );
}

/*********************************************************************
 * @fn      zclGeneral_SendSceneGetMembershipResponse
 *
 * @brief   Call to send Scene Get Membership Response Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   status - scene command status
 * @param   capacity - remaining capacity of the scene table
 * @param   sceneCnt - number of scenes in the scene list
 * @param   sceneList - list of scene IDs
 * @param   groupID - group ID that scene belongs to
 * @param   seqNum - sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendSceneGetMembershipResponse( uint8 srcEP, afAddrType_t *dstAddr,
                       uint8 status, uint8 capacity, uint8 sceneCnt, 
                       uint8 *sceneList, uint16 groupID, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 len = 1 + 1 + 2; // Status + Capacity + Group ID;
  uint8 i;
  ZStatus_t stat;
  
  if ( status == ZCL_STATUS_SUCCESS )
  {
    len++; // Scene Count
    len += sceneCnt; // Scene List (Scene ID is a single octet)
  }

  buf = osal_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    *pBuf++ = status;
    *pBuf++ = capacity;
    *pBuf++ = LO_UINT16( groupID );
    *pBuf++ = HI_UINT16( groupID );
    if ( status == ZCL_STATUS_SUCCESS )
    {
      *pBuf++ = sceneCnt;
      for ( i = 0; i < sceneCnt; i++ )
        *pBuf++ = sceneList[i];
    }

    stat = zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_SCENES,
                            TRUE, COMMAND_SCENE_GET_MEMBERSHIP_RSP, TRUE, 
                            ZCL_FRAME_SERVER_CLIENT_DIR, 0, seqNum, len, buf );
    osal_mem_free( buf );
  }
  else
    stat = ZMemError;

  return ( stat );
}
#endif // ZCL_SCENES

#ifdef ZCL_LEVEL_CTRL
/*********************************************************************
 * @fn      zclGeneral_SendLevelControlMoveToLevel
 *
 * @brief   Call to send out a Level Control - Move to Level Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   level - what level to move to
 * @param   transitionTime - how long to take to get to the level (in seconds)
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLevelControlMoveToLevel( uint8 srcEP, afAddrType_t *dstAddr,
                       uint8 level, uint16 transTime, uint8 seqNum )
{
  uint8 buf[3];

  buf[0] = level;
  buf[1] = LO_UINT16( transTime );
  buf[2] = HI_UINT16( transTime );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LEVEL_CTRL, 
                          TRUE, COMMAND_LEVEL_MOVE_TO_LEVEL, TRUE, 
                          ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, 3, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendLevelControlMove
 *
 * @brief   Call to send out a Level Control - Move Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   moveMode - LEVEL_MOVE_STOP,
 *                     LEVEL_MOVE_UP,
 *                     LEVEL_MOVE_ON_AND_UP,
 *                     LEVEL_MOVE_DOWN, or
 *                     LEVEL_MOVE_DOWN_AND_OFF
 * @param   rate - number of steps to take per second
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLevelControlMove( uint8 srcEP, afAddrType_t *dstAddr,
                     uint8 moveMode, uint8 rate, uint8 seqNum )
{
  uint8 buf[2];

  buf[0] = moveMode;
  buf[1] = rate;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LEVEL_CTRL, 
                          TRUE, COMMAND_LEVEL_MOVE, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, 2, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendLevelControlStep
 *
 * @brief   Call to send out a Level Control - Step Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   stepMode - LEVEL_STEP_UP,
 *                     LEVEL_STEP_ON_AND_UP,
 *                     LEVEL_STEP_DOWN, or
 *                     LEVEL_STEP_DOWN_AND_OFF
 * @param   amount - number of levels to step
 * @param   transitionTime - time, in 1/10ths of a second, to take to perform the step
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLevelControlStep( uint8 srcEP, afAddrType_t *dstAddr,
                                           uint8 stepMode, uint8 amount, uint16 transTime,
                                           uint8 seqNum )
{
  uint8 buf[4];

  buf[0] = stepMode;
  buf[1] = amount;
  buf[2] = LO_UINT16( transTime );
  buf[3] = HI_UINT16( transTime );
  
  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LEVEL_CTRL, 
                          TRUE, COMMAND_LEVEL_STEP, TRUE, 
                          ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, 4, buf );
}
#endif // ZCL_LEVEL_CTRL

#ifdef ZCL_ALARMS
/*********************************************************************
 * @fn      zclGeneral_SendAlarmRequest
 *
 * @brief   Call to send out an Alarm Request Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   cmd - either COMMAND_ALARMS_RESET or COMMAND_ALARMS_ALARM 
 * @param   alarmCode - code for the cause of the alarm
 * @param   clusterID - cluster whose attribute generate the alarm
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendAlarmRequest( uint8 srcEP, afAddrType_t *dstAddr,
                                       uint8 cmd, uint8 alarmCode, uint16 clusterID, 
                                       uint8 seqNum )
{
  uint8 buf[3];

  buf[0] = alarmCode;
  buf[1] = LO_UINT16( clusterID );
  buf[2] = HI_UINT16( clusterID );
  
  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_ALARMS,
                          TRUE, cmd, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR, 
                          0, seqNum, 3, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendAlarmGetRespnose
 *
 * @brief   Call to send out an Alarm Get Response Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   status - SUCCESS or NOT_FOUND
 * @param   alarmCode - code for the cause of the alarm
 * @param   clusterID - cluster whose attribute generate the alarm
 * @param   timeStamp - time at which the alarm occured
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendAlarmGetRespnose( uint8 srcEP, afAddrType_t *dstAddr,
                              uint8 status, uint8 alarmCode, uint16 clusterID,
                              uint32 timeStamp, uint8 seqNum )
{
  uint8 buf[8];
  uint8 len = 1; // Status
  
  buf[0] = status;
  if ( status == ZCL_STATUS_SUCCESS )
  {
    len += 1 + 2 + 4; // Alarm code + Cluster ID + Time stamp
    buf[1] = alarmCode;
    buf[2] = LO_UINT16( clusterID );
    buf[3] = HI_UINT16( clusterID );
    buf[4] = BREAK_UINT32( timeStamp, 0 );
    buf[5] = BREAK_UINT32( timeStamp, 1 );
    buf[6] = BREAK_UINT32( timeStamp, 2 );
    buf[7] = BREAK_UINT32( timeStamp, 3 );
  }
  
  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_ALARMS,
                          TRUE, COMMAND_ALARMS_GET_RSP, TRUE, 
                          ZCL_FRAME_SERVER_CLIENT_DIR, 0, seqNum, len, buf );
}
#endif // ZCL_ALARMS

#ifdef ZCL_LOCATION
/*********************************************************************
 * @fn      zclGeneral_SendLocationSetAbsolute
 *
 * @brief   Call to send out a Set Absolute Location Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   absLoc - absolute location info
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLocationSetAbsolute( uint8 srcEP, afAddrType_t *dstAddr, 
                                    zclLocationAbsolute_t *absLoc, uint8 seqNum )
{
   uint8 buf[10]; // 5 fields (2 octects each)
   
   buf[0] = LO_UINT16( absLoc->coordinate1 );
   buf[1] = HI_UINT16( absLoc->coordinate1 );
   buf[2] = LO_UINT16( absLoc->coordinate2 );
   buf[3] = HI_UINT16( absLoc->coordinate2 );
   buf[4] = LO_UINT16( absLoc->coordinate3 );
   buf[5] = HI_UINT16( absLoc->coordinate3 );
   buf[6] = LO_UINT16( absLoc->power );
   buf[7] = HI_UINT16( absLoc->power );
   buf[8] = LO_UINT16( absLoc->pathLossExponent );
   buf[9] = HI_UINT16( absLoc->pathLossExponent );
   
   return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION,
                           TRUE, COMMAND_LOCATION_SET_ABSOLUTE, TRUE, 
                           ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, 10, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendLocationSetDevCfg
 *
 * @brief   Call to send out a Set Device Configuration Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   devCfg - device configuration info
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLocationSetDevCfg( uint8 srcEP, afAddrType_t *dstAddr, 
                                     zclLocationDevCfg_t *devCfg, uint8 seqNum )
{
   uint8 buf[9];  // 4 fields (2 octects each) + 1 field with 1 octect
   
   buf[0] = LO_UINT16( devCfg->power );
   buf[1] = HI_UINT16( devCfg->power );
   buf[2] = LO_UINT16( devCfg->pathLossExponent );
   buf[3] = HI_UINT16( devCfg->pathLossExponent );
   buf[4] = LO_UINT16( devCfg->calcPeriod );
   buf[5] = HI_UINT16( devCfg->calcPeriod );
   buf[6] = devCfg->numMeasurements;
   buf[7] = LO_UINT16( devCfg->reportPeriod );
   buf[8] = HI_UINT16( devCfg->reportPeriod );
   
   return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION,
                           TRUE, COMMAND_LOCATION_SET_DEV_CFG, TRUE,
                           ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, 9, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendLocationGetDevCfg
 *
 * @brief   Call to send out a Get Device Configuration Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   targetAddr - device for which location parameters are being requested
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLocationGetDevCfg( uint8 srcEP, afAddrType_t *dstAddr, 
                                            uint8 *targetAddr, uint8 seqNum )
{
  uint8 buf[8];
  
  osal_cpyExtAddr( buf, targetAddr );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION,
                          TRUE, COMMAND_LOCATION_GET_DEV_CFG, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, 8, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendLocationGetData
 *
 * @brief   Call to send out a Get Location Data Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   locaData - location information and channel parameters that are requested.
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLocationGetData( uint8 srcEP, afAddrType_t *dstAddr, 
                                   zclLocationGetData_t *locData, uint8 seqNum )
{
  uint8 buf[10]; // bitmap (1) + number responses (1) + IEEE Address (8)
  uint8 *pBuf = buf;
  uint8 len = 2; // bitmap + number responses

  *pBuf  = locData->absoluteOnly;
  *pBuf |= locData->recalculate << 1;
  *pBuf |= locData->brdcastIndicator << 2;
  *pBuf |= locData->brdcastResponse << 3;
  *pBuf |= locData->compactResponse << 4;
  pBuf++;  // move past the bitmap field

  *pBuf++ = locData->numResponses;
  
  if ( locData->brdcastIndicator == 0 )
  {
    osal_cpyExtAddr( pBuf, locData->targetAddr );
    len += 8; // ieee addr
  }
 
  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION,
                          TRUE, COMMAND_LOCATION_GET_DATA, TRUE, 
                          ZCL_FRAME_CLIENT_SERVER_DIR, 0, seqNum, len, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendLocationDevCfgResponse
 *
 * @brief   Call to send out a Device Configuration Response Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   devCfg - device's location parameters that are requested
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLocationDevCfgResponse( uint8 srcEP, afAddrType_t *dstAddr, 
                                       zclLocationDevCfgRsp_t *devCfg, uint8 seqNum )
{
  uint8 buf[10]; // 4 fields (2 octects each) + 2 fields (1 octect each)
  uint8 len = 1; // Status
  
  buf[0] = devCfg->status;
  if ( devCfg->status == ZCL_STATUS_SUCCESS )
  {
    buf[1] = LO_UINT16( devCfg->data.power );
    buf[2] = HI_UINT16( devCfg->data.power );
    buf[3] = LO_UINT16( devCfg->data.pathLossExponent );
    buf[4] = HI_UINT16( devCfg->data.pathLossExponent );
    buf[5] = LO_UINT16( devCfg->data.calcPeriod );
    buf[6] = HI_UINT16( devCfg->data.calcPeriod );
    buf[7] = devCfg->data.numMeasurements;
    buf[8] = LO_UINT16( devCfg->data.reportPeriod );
    buf[9] = HI_UINT16( devCfg->data.reportPeriod );
    len += 9;
  }
 
  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION,
                          TRUE, COMMAND_LOCATION_DEV_CFG_RSP, TRUE, 
                          ZCL_FRAME_SERVER_CLIENT_DIR, 0, seqNum, len, buf );
}

/*********************************************************************
 * @fn      zclGeneral_SendLocationData
 *
 * @brief   Call to send out location data
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   status - indicates whether response to request was successful or not
 * @param   locData - location information and channel parameters being sent
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_SendLocationData( uint8 srcEP, afAddrType_t *dstAddr, uint8 cmd,
                                       uint8 status, zclLocationData_t *locData,
                                       uint8 seqNum )
{
  uint8 buf[16];
  uint8 *pBuf = buf;
  uint8 len = 0;
  
  if ( cmd == COMMAND_LOCATION_DATA_RSP )
  {
    // Only response command includes a status field
    *pBuf++ = status;
    len++;
  }

  if ( cmd != COMMAND_LOCATION_DATA_RSP || status == ZCL_STATUS_SUCCESS )
  {
    // Notification or Response with successful status
    *pBuf++ = locData->type;
    *pBuf++ = LO_UINT16( locData->absLoc.coordinate1 );
    *pBuf++ = HI_UINT16( locData->absLoc.coordinate1 );
    *pBuf++ = LO_UINT16( locData->absLoc.coordinate2 );
    *pBuf++ = HI_UINT16( locData->absLoc.coordinate2 );
    len += 5;

    if ( locationType2D(locData->type) == 0 )
    {
      // 2D location doesn't have coordinate 3
      *pBuf++ = LO_UINT16( locData->absLoc.coordinate3 );
      *pBuf++ = HI_UINT16( locData->absLoc.coordinate3 );
      len += 2;
    }
    
    if ( cmd != COMMAND_LOCATION_COMPACT_DATA_NOTIF )
    {
      // Compact notification doesn't include these fields
      *pBuf++ = LO_UINT16( locData->absLoc.power );
      *pBuf++ = HI_UINT16( locData->absLoc.power );
      *pBuf++ = LO_UINT16( locData->absLoc.pathLossExponent );
      *pBuf++ = HI_UINT16( locData->absLoc.pathLossExponent );
      len += 4;
    }
 
    if ( locationTypeAbsolute(locData->type) == 0 )
    {
      // Absolute location doesn't include these fields
      if ( cmd != COMMAND_LOCATION_COMPACT_DATA_NOTIF )
      {
        // Compact notification doesn't include this field
        *pBuf++ = locData->calcLoc.locationMethod;
        len++;
      }

      *pBuf++ = locData->calcLoc.qualityMeasure;
      *pBuf++ = LO_UINT16( locData->calcLoc.locationAge );
      *pBuf++ = HI_UINT16( locData->calcLoc.locationAge );
      len += 3;
    }
  }
 
  return zcl_SendCommand( srcEP, dstAddr, ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION,
                          TRUE, cmd, TRUE, ZCL_FRAME_SERVER_CLIENT_DIR, 
                          0, seqNum, len, buf );
}
#endif // ZCL_LOCATION

#if defined(ZCL_BASIC) || defined(ZCL_IDENTIFY) || defined(ZCL_GROUPS) || defined(ZCL_SCENES) || \
    defined(ZCL_ON_OFF) || defined(ZCL_LEVEL_CTRL) || defined(ZCL_ALARMS) || defined(ZCL_LOCATION)
/*********************************************************************
 * @fn      zclGeneral_FindCallbacks
 *
 * @brief   Find the callbacks for an endpoint
 *
 * @param   endpoint
 *
 * @return  pointer to the callbacks
 */
static zclGeneral_AppCallbacks_t *zclGeneral_FindCallbacks( uint8 endpoint )
{
  zclGenCBRec_t *pCBs;
  
  pCBs = zclGenCBs;
  while ( pCBs )
  {
    if ( pCBs->endpoint == endpoint )
      return ( pCBs->CBs );
  }
  return ( (zclGeneral_AppCallbacks_t *)NULL );
}
#endif // any ZCL General command

/*********************************************************************
 * @fn      zclGeneral_HdlIncoming
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library or Profile commands for attributes
 *          that aren't in the attribute list
 *
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   logicalClusterID
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_HdlIncoming( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  if ( zcl_ClusterCmd( pInMsg->hdr.fc.type ) )
  {
    // Is this a manufacturer specific command?
    if ( pInMsg->hdr.fc.manuSpecific == 0 ) 
    {
      stat = zclGeneral_HdlInSpecificCommands( pInMsg, logicalClusterID );
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
    switch ( logicalClusterID )
    {
      case ZCL_GEN_LOGICAL_CLUSTER_ID_BASIC:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_POWER_CFG:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_DEV_TEMP_CFG:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_IDENTIFY:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_GROUPS:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_SCENES:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_ON_OFF:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_ON_OFF_SWITCH_CFG:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_LEVEL_CTRL:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_ALARMS:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_TIME:
      case ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION:
      default:
        stat = ZFailure;
        break;
    }
  }
  return ( stat );
}

/*********************************************************************
 * @fn      zclGeneral_HdlInSpecificCommands
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library

 * @param   pInMsg - pointer to the incoming message
 * @param   logicalClusterID
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_HdlInSpecificCommands( zclIncoming_t *pInMsg, uint16 logicalClusterID )
{
  ZStatus_t stat = ZSuccess;

  switch ( logicalClusterID )
  {
#ifdef ZCL_BASIC
    case ZCL_GEN_LOGICAL_CLUSTER_ID_BASIC:
      stat = zclGeneral_ProcessInBasic( pInMsg );
      break;
#endif // ZCL_BASIC

#ifdef ZCL_IDENTIFY
    case ZCL_GEN_LOGICAL_CLUSTER_ID_IDENTIFY:
      stat = zclGeneral_ProcessInIdentity( pInMsg );
      break;
#endif // ZCL_IDENTIFY

#ifdef ZCL_GROUPS
    case ZCL_GEN_LOGICAL_CLUSTER_ID_GROUPS:
      if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
        stat = zclGeneral_ProcessInGroupsServer( pInMsg );
      else
        stat = zclGeneral_ProcessInGroupsClient( pInMsg );
      break;
#endif // ZCL_GROUPS

#ifdef ZCL_SCENES
    case ZCL_GEN_LOGICAL_CLUSTER_ID_SCENES:
      if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
        stat = zclGeneral_ProcessInScenesServer( pInMsg );
      else
        stat = zclGeneral_ProcessInScenesClient( pInMsg );
      break;
#endif // ZCL_SCENES
      
#ifdef ZCL_ON_OFF
    case ZCL_GEN_LOGICAL_CLUSTER_ID_ON_OFF:
      stat = zclGeneral_ProcessInOnOff( pInMsg );
      break;
#endif // ZCL_ON_OFF

#ifdef ZCL_LEVEL_CTRL      
    case ZCL_GEN_LOGICAL_CLUSTER_ID_LEVEL_CTRL:
      stat = zclGeneral_ProcessInLevelControl( pInMsg );
      break;
#endif // ZCL_LEVEL_CTRL

#ifdef ZCL_ALARMS
    case ZCL_GEN_LOGICAL_CLUSTER_ID_ALARMS:
      if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
        stat = zclGeneral_ProcessInAlarmsServer( pInMsg );
      else
        stat = zclGeneral_ProcessInAlarmsClient( pInMsg );
      break;
#endif // ZCL_ALARMS
      
#ifdef ZCL_LOCATION
    case ZCL_GEN_LOGICAL_CLUSTER_ID_LOCATION:
      if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
        stat = zclGeneral_ProcessInLocationServer( pInMsg );
      else
        stat = zclGeneral_ProcessInLocationClient( pInMsg );
      break;
#endif // ZCL_LOCATION
      
    case ZCL_GEN_LOGICAL_CLUSTER_ID_POWER_CFG:
    case ZCL_GEN_LOGICAL_CLUSTER_ID_DEV_TEMP_CFG:
    case ZCL_GEN_LOGICAL_CLUSTER_ID_ON_OFF_SWITCH_CFG:
    case ZCL_GEN_LOGICAL_CLUSTER_ID_TIME:
    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

#ifdef ZCL_BASIC
/*********************************************************************
 * @fn      zclGeneral_ProcessInBasic
 *
 * @brief   Process in the received Basic Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInBasic( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  
  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    if ( pInMsg->hdr.commandID > COMMAND_BASIC_RESET_FACT_DEFAULT )
      return ( ZFailure );   // Error ignore the command

    pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
    if ( pCBs && pCBs->pfnBasicReset )
      pCBs->pfnBasicReset();
  }
  // no Client command
  
  return ( ZSuccess );
}
#endif // ZCL_BASIC

#ifdef ZCL_IDENTIFY
/*********************************************************************
 * @fn      zclGeneral_ProcessInIdentity
 *
 * @brief   Process in the received Identity Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInIdentity( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  zclAttrRec_t *pAttr;
  uint16 identifyTime = 0;
  uint16 timeout;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    if ( pInMsg->hdr.commandID > COMMAND_IDENTIFY_QUERY )
      return ( ZFailure );   // Error ignore the command

    // Retrieve Identify Time
    pAttr = zclFindAttrRec( pInMsg->msg->endPoint, ZCL_HA_CLUSTER_ID_GEN_IDENTIFY, ATTRID_IDENTIFY_TIME );
    if ( pAttr )
      zclReadAttrData( (uint8 *)&identifyTime, pAttr );
    
    // Is device identifying itself?
    if ( identifyTime > 0 )
    {
      zclGeneral_SendIdentifyQueryResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, 
                                            identifyTime, pInMsg->hdr.transSeqNum );
    }
  }
  else // Client Command
  {
    if ( pInMsg->hdr.commandID > COMMAND_IDENTIFY_QUERY_RSP )
      return ( ZFailure );   // Error ignore the command
 
    timeout = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );
    pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
    if ( pCBs && pCBs->pfnIdentifyRsp )
      pCBs->pfnIdentifyRsp( &(pInMsg->msg->srcAddr), timeout );
  }
  
  return ( ZSuccess );
}
#endif // ZCL_IDENTIFY

#ifdef ZCL_GROUPS
/*********************************************************************
 * @fn      zclGeneral_ProcessInGroupsServer
 *
 * @brief   Process in the received Groups Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInGroupsServer( zclIncoming_t *pInMsg )
{
  zclAttrRec_t *pAttr;
  aps_Group_t group;
  aps_Group_t *pGroup;
  uint8 *pData;
  uint8 status;
  uint8 grpCnt;
  uint8 grpRspCnt = 0;
  uint16 grpList[APS_MAX_GROUPS];
  uint8 nameLen;
  uint16 identifyTime = 0;
  uint8 i;
  ZStatus_t stat = ZSuccess;

  osal_memset( (uint8*)&group, 0, sizeof( aps_Group_t ) );

  pData = pInMsg->pData;
  group.ID = BUILD_UINT16( pData[0], pData[1] );
  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_GROUP_ADD:
      pData += 2;   // Move past group ID
      nameLen = *pData++;
      if ( nameLen > (APS_GROUP_NAME_LEN-1) )
        nameLen = (APS_GROUP_NAME_LEN-1);
      group.name[0] = nameLen;
      osal_memcpy( &(group.name[1]), pData, nameLen );
  
      status = aps_AddGroup( pInMsg->msg->endPoint, &group );
      if ( status != ZSuccess )
      {
        if ( status == ZApsDuplicateEntry )
          status = ZCL_STATUS_DUPLICATE_EXISTS;
        else
          status = ZCL_STATUS_INSUFFICIENT_SPACE;
      }

      zclGeneral_SendGroupAddResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, 
                                       status, group.ID, pInMsg->hdr.transSeqNum );
      break;

    case COMMAND_GROUP_VIEW:
      pGroup = aps_FindGroup( pInMsg->msg->endPoint, group.ID );
      if ( pGroup )
      {
        status = ZCL_STATUS_SUCCESS;
      }
      else
      {
        // Group not found
        status = ZCL_STATUS_NOT_FOUND;
        pGroup = &group;
      }
      zclGeneral_SendGroupViewResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, 
                                        status, pGroup, pInMsg->hdr.transSeqNum );
      break;

    case COMMAND_GROUP_GET_MEMBERSHIP:
      grpCnt = *pData++;
      if ( grpCnt == 0 )
      {
        // Find out all the groups of which the endpoint is a member.
        grpRspCnt = aps_FindAllGroupsForEndpoint( pInMsg->msg->endPoint, grpList );
      }
      else
      {
        // Find out the groups (in the list) of which the endpoint is a member.
        for ( i = 0; i < grpCnt; i++ )
        {
          group.ID = BUILD_UINT16( pData[0], pData[1] );
          pData += 2;

          if ( aps_FindGroup( pInMsg->msg->endPoint, group.ID ) )
            grpList[grpRspCnt++] = group.ID;
        }
      }

      if ( grpCnt == 0 ||  grpRspCnt != 0 )
      {
        zclGeneral_SendGroupGetMembershipResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, 
                                                   aps_GroupsRemaingCapacity(), grpRspCnt, 
                                                   grpList, pInMsg->hdr.transSeqNum );
      }
      break;
      
    case COMMAND_GROUP_REMOVE:
      if ( aps_RemoveGroup( pInMsg->msg->endPoint, group.ID ) )
        status = ZCL_STATUS_SUCCESS;
      else
        status = ZCL_STATUS_NOT_FOUND;
      zclGeneral_SendGroupRemoveResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, 
                                          status, group.ID, pInMsg->hdr.transSeqNum );
      break;

    case COMMAND_GROUP_REMOVE_ALL:
      aps_RemoveAllGroup( pInMsg->msg->endPoint );
      break;
    
    case COMMAND_GROUP_ADD_IF_IDENTIFYING:
      // Retrieve Identify Time
      pAttr = zclFindAttrRec( pInMsg->msg->endPoint, ZCL_HA_CLUSTER_ID_GEN_IDENTIFY, ATTRID_IDENTIFY_TIME );
      if ( pAttr )
        zclReadAttrData( (uint8 *)&identifyTime, pAttr );  
      
      // Is device identifying itself?
      if ( identifyTime > 0 )
      {
        pData += 2;   // Move past group ID
        nameLen = *pData++;
        if ( nameLen > (APS_GROUP_NAME_LEN-1) )
          nameLen = (APS_GROUP_NAME_LEN-1);
        group.name[0] = nameLen;
        osal_memcpy( &(group.name[1]), pData, nameLen );
        
        aps_AddGroup( pInMsg->msg->endPoint, &group );
      }
      break;
      
    default:
      stat = ZFailure;
      break;
  }
  
  return ( stat );
} 
  
/*********************************************************************
 * @fn      zclGeneral_ProcessInGroupsClient
 *
 * @brief   Process in the received Groups Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInGroupsClient( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  aps_Group_t group;
  uint8 *pData = pInMsg->pData;
  uint8 capacity;
  uint8 grpCnt;
  uint8 *grpName = (uint8 *)NULL;
  uint16 grpList[APS_MAX_GROUPS];
  uint8 nameLen;
  uint8 status; 
  uint8 i;
  ZStatus_t stat = ZSuccess;

  osal_memset( (uint8*)&group, 0, sizeof( aps_Group_t ) );

  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_GROUP_ADD_RSP:
    case COMMAND_GROUP_VIEW_RSP:
    case COMMAND_GROUP_REMOVE_RSP:
      status = *pData++;
      group.ID = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );

      if ( status == ZCL_STATUS_SUCCESS && pInMsg->hdr.commandID == COMMAND_GROUP_VIEW_RSP )
      {
        pData += 2;   // Move past ID
        nameLen = *pData++;
        if ( nameLen > (APS_GROUP_NAME_LEN-1) )
          nameLen = (APS_GROUP_NAME_LEN-1);
        group.name[0] = nameLen;
        osal_memcpy( &(group.name[1]), pData, nameLen );
        grpName = group.name;
      }

      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnGroupRsp )
        pCBs->pfnGroupRsp( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID, 
                           status, 1, &group.ID, 0, grpName );
      break;
      
    case COMMAND_GROUP_GET_MEMBERSHIP_RSP:
      capacity = *pData++;
      grpCnt = *pData++;
 
      for ( i = 0; i < grpCnt; i++ )
      {
        grpList[i] = BUILD_UINT16( pData[0], pData[1] );
        pData += 2;
      }

      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnGroupRsp )
        pCBs->pfnGroupRsp( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID, 
                           0, grpCnt, grpList, capacity, NULL );
      break;
            
    default:
      stat = ZFailure;
      break;
  }
  
  return ( stat );
}
#endif // ZCL_GROUPS

#ifdef ZCL_SCENES
/*********************************************************************
 * @fn      zclGeneral_AddScene
 *
 * @brief   Add a scene for an endpoint
 *
 * @param   endpoint -
 * @param   scene - new scene item
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_AddScene( uint8 endpoint, zclGeneral_Scene_t *scene )
{
  zclGenSceneItem_t *pNewItem;
  zclGenSceneItem_t *pLoop;

  // Fill in the new profile list
  pNewItem = osal_mem_alloc( sizeof( zclGenSceneItem_t ) );
  if ( pNewItem == NULL )
    return ( ZMemError );

  // Fill in the plugin record.
  pNewItem->next = (zclGenSceneItem_t *)NULL;
  pNewItem->endpoint = endpoint;
  osal_memcpy( (uint8*)&(pNewItem->scene), (uint8*)scene, sizeof ( zclGeneral_Scene_t ));

  // Find spot in list
  if (  zclGenSceneTable == NULL )
  {
    zclGenSceneTable = pNewItem;
  }
  else
  {
    // Look for end of list
    pLoop = zclGenSceneTable;
    while ( pLoop->next != NULL )
      pLoop = pLoop->next;

    // Put new item at end of list
    pLoop->next = pNewItem;
  }

  // Update NV
  zclGeneral_ScenesWriteNV();
  
  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zclGeneral_FindScene
 *
 * @brief   Find a scene with endpoint and sceneID
 *
 * @param   endpoint -
 * @param   groupID - what group the scene belongs to
 * @param   sceneID - ID to look for scene
 *
 * @return  a pointer to the scene information, NULL if not found
 */
zclGeneral_Scene_t *zclGeneral_FindScene( uint8 endpoint, uint16 groupID, uint8 sceneID )
{
  zclGenSceneItem_t *pLoop;

  // Look for end of list
  pLoop = zclGenSceneTable;
  while ( pLoop )
  {
    if ( (pLoop->endpoint == endpoint || endpoint == 0xFF)
        && pLoop->scene.groupID == groupID && pLoop->scene.ID == sceneID )
    {
      return ( &(pLoop->scene) );
    }
    pLoop = pLoop->next;
  }

  return ( (zclGeneral_Scene_t *)NULL );
}

/*********************************************************************
 * @fn      aps_FindAllScensForGroup
 *
 * @brief   Find all the scenes with groupID
 *
 * @param   endpoint - endpoint to look for
 * @param   sceneList - List to hold scene IDs (should hold APS_MAX_SCENES entries)
 *
 * @return  number of scenes copied to sceneList
 */
uint8 zclGeneral_FindAllScenesForGroup( uint8 endpoint, uint16 groupID, uint8 *sceneList )
{
  zclGenSceneItem_t *pLoop;
  uint8 cnt = 0;

  // Look for end of list
  pLoop = zclGenSceneTable;
  while ( pLoop )
  {
    if ( pLoop->endpoint == endpoint && pLoop->scene.groupID == groupID )
      sceneList[cnt++] = pLoop->scene.ID;
    pLoop = pLoop->next;
  }
  return ( cnt );
}

/*********************************************************************
 * @fn      zclGeneral_RemoveScene
 *
 * @brief   Remove a scene with endpoint and sceneID
 *
 * @param   endpoint -
 * @param   groupID - what group the scene belongs to
 * @param   sceneID - ID to look for scene
 *
 * @return  TRUE if removed, FALSE if not found
 */
uint8 zclGeneral_RemoveScene( uint8 endpoint, uint16 groupID, uint8 sceneID )
{
  zclGenSceneItem_t *pLoop;
  zclGenSceneItem_t *pPrev;

  // Look for end of list
  pLoop = zclGenSceneTable;
  pPrev = NULL;
  while ( pLoop )
  {
    if ( pLoop->endpoint == endpoint
        && pLoop->scene.groupID == groupID && pLoop->scene.ID == sceneID )
    {
      if ( pPrev == NULL )
        zclGenSceneTable = pLoop->next;
      else
        pPrev->next = pLoop->next;

      // Free the memory
      osal_mem_free( pLoop );

      // Update NV
      zclGeneral_ScenesWriteNV();
  
      return ( TRUE );
    }
    pPrev = pLoop;
    pLoop = pLoop->next;
  }

  return ( FALSE );
}

/*********************************************************************
 * @fn      zclGeneral_RemoveAllScenes
 *
 * @brief   Remove all scenes with endpoint and group Id
 *
 * @param   endpoint -
 * @param   groupID - ID to look for group
 *
 * @return  none
 */
void zclGeneral_RemoveAllScenes( uint8 endpoint, uint16 groupID )
{
  zclGenSceneItem_t *pLoop;
  zclGenSceneItem_t *pPrev;
  zclGenSceneItem_t *pNext;

  // Look for end of list
  pLoop = zclGenSceneTable;
  pPrev = NULL;
  while ( pLoop )
  {
    if ( pLoop->endpoint == endpoint && pLoop->scene.groupID == groupID )
    {
      if ( pPrev == NULL )
        zclGenSceneTable = pLoop->next;
      else
        pPrev->next = pLoop->next;
      pNext = pLoop->next;

      // Free the memory
      osal_mem_free( pLoop );
      pLoop = pNext;
    }
    else
    {
      pPrev = pLoop;
      pLoop = pLoop->next;
    }
  }
  
  // Update NV
  zclGeneral_ScenesWriteNV();
}

/*********************************************************************
 * @fn      zclGeneral_CountScenes
 *
 * @brief   Count the number of scenes for an endpoint
 *
 * @param   endpoint -
 *
 * @return  number of scenes assigned to an endpoint
 */
uint8 zclGeneral_CountScenes( uint8 endpoint )
{
  zclGenSceneItem_t *pLoop;
  uint8 cnt = 0;

  // Look for end of list
  pLoop = zclGenSceneTable;
  while ( pLoop )
  {
    if ( pLoop->endpoint == endpoint  )
      cnt++;
    pLoop = pLoop->next;
  }
  return ( cnt );
}

/*********************************************************************
 * @fn      zclGeneral_CountAllScenes
 *
 * @brief   Count the total number of scenes
 *
 * @param   none
 *
 * @return  number of scenes
 */
uint8 zclGeneral_CountAllScenes( void )
{
  zclGenSceneItem_t *pLoop;
  uint8 cnt = 0;

  // Look for end of list
  pLoop = zclGenSceneTable;
  while ( pLoop )
  {
    cnt++;
    pLoop = pLoop->next;
  }
  return ( cnt );
}

/*********************************************************************
 * @fn      zclGeneral_ProcessInScenesServer
 *
 * @brief   Process in the received Scenes Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInScenesServer( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  zclGeneral_Scene_t scene;
  zclGeneral_Scene_t *pScene;
  uint8 *pData = pInMsg->pData;
  uint8 nameLen;
  uint8 status;
  uint8 sceneCnt = 0;
  uint8 sceneList[ZCL_GEN_MAX_SCENES];
  uint8 sendRsp = FALSE;
  ZStatus_t stat = ZSuccess;

  osal_memset( (uint8*)&scene, 0, sizeof( zclGeneral_Scene_t ) );

  scene.groupID = BUILD_UINT16( pData[0], pData[1] );
  pData += 2;   // Move past group ID
  scene.ID = *pData++;
  
  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_SCENE_ADD:
      // Parse the rest of the incoming message
      scene.transTime = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      nameLen= *pData++; // Name length
      if ( nameLen > (ZCL_GEN_SCENE_NAME_LEN-1) )
        nameLen = (ZCL_GEN_SCENE_NAME_LEN-1);
      scene.name[0] = nameLen;
      osal_memcpy( &(scene.name[1]), pData, nameLen );
      pData += nameLen; // move pass name

      scene.extLen = pInMsg->pDataLen - ( (uint8)( (uint16)pData - (uint16)pInMsg->pData ) );
      if ( scene.extLen > 0 )
      {
        // Copy the extention field(s)
        if ( scene.extLen > ZCL_GEN_SCENE_EXT_LEN )
          scene.extLen = ZCL_GEN_SCENE_EXT_LEN;
        osal_memcpy( scene.extField, pData, scene.extLen );
      }
      
      if ( scene.groupID == 0x0000 ||
           aps_FindGroup( pInMsg->msg->endPoint, scene.groupID ) != NULL )
      {
        // Either the Scene doesn't belong to a Group (Group ID = 0x0000) or it
        // does and the corresponding Group exits
        pScene = zclGeneral_FindScene( pInMsg->msg->endPoint, scene.groupID, scene.ID );
        if ( pScene || ( zclGeneral_CountAllScenes() < ZCL_GEN_MAX_SCENES ) )
        {
          status = ZCL_STATUS_SUCCESS;
          if ( pScene != NULL )
          {
            // The Scene already exists so update it
            pScene->transTime = scene.transTime;
            osal_memcpy( pScene->name, scene.name, ZCL_GEN_SCENE_NAME_LEN );

            // Use the new extention field(s) 
            osal_memcpy( pScene->extField, scene.extField, scene.extLen );
            pScene->extLen = scene.extLen;
            
            // Update NV
            zclGeneral_ScenesWriteNV();
          }
          else
          {
            // The Scene doesn't exist so add it
            zclGeneral_AddScene( pInMsg->msg->endPoint, &scene ); 
          }
        }
        else
          status = ZCL_STATUS_INSUFFICIENT_SPACE; // The Scene Table is full
      }
      else
        status = ZCL_STATUS_INVALID_FIELD; // The Group is not in the Group Table
      
      zclGeneral_SendSceneAddResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr,
                                       status, scene.groupID, scene.ID, 
                                       pInMsg->hdr.transSeqNum );
      break;

    case COMMAND_SCENE_VIEW:
      pScene = zclGeneral_FindScene( pInMsg->msg->endPoint, scene.groupID, scene.ID );
      if ( pScene != NULL )
      {
        status = ZCL_STATUS_SUCCESS;
      }
      else
      {
        // Scene not found
        if ( scene.groupID != 0x0000 && 
             aps_FindGroup( pInMsg->msg->endPoint, scene.groupID ) == NULL )
        {
          status = ZCL_STATUS_INVALID_FIELD; // The Group is not in the Group Table
        }
        else
          status = ZCL_STATUS_NOT_FOUND;
        pScene = &scene;
      }
      zclGeneral_SendSceneViewResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, 
                                        status, pScene, pInMsg->hdr.transSeqNum );
      break;

    case COMMAND_SCENE_REMOVE:
      if ( zclGeneral_RemoveScene( pInMsg->msg->endPoint, scene.groupID, scene.ID ) )
      {
        status = ZCL_STATUS_SUCCESS;
      }
      else
      {
        // Scene not found
        if ( aps_FindGroup( pInMsg->msg->endPoint, scene.groupID ) == NULL )
        {
          // The Group is not in the Group Table
          status = ZCL_STATUS_INVALID_FIELD;
        }
        else
          status = ZCL_STATUS_NOT_FOUND;
      }
      
      if ( pInMsg->msg->groupId == 0 )
      {
        // Addressed to this device (not to a group) - send a response back
        zclGeneral_SendSceneRemoveResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr,
                                            status, scene.groupID,
                                            scene.ID, pInMsg->hdr.transSeqNum );
      }
      break;

    case COMMAND_SCENE_REMOVE_ALL:
      if ( scene.groupID == 0x0000 || 
           aps_FindGroup( pInMsg->msg->endPoint, scene.groupID ) != NULL )
      {
        zclGeneral_RemoveAllScenes( pInMsg->msg->endPoint, scene.groupID );
        status = ZCL_STATUS_SUCCESS;
      }
      else
        status = ZCL_STATUS_INVALID_FIELD;
 
      if ( pInMsg->msg->groupId == 0 )
      {
        // Addressed to this device (not to a group) - send a response back
        zclGeneral_SendSceneRemoveAllResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr,
                                               status, scene.groupID, pInMsg->hdr.transSeqNum );
      } 
      break;

    case COMMAND_SCENE_STORE:      
      if ( scene.groupID == 0x0000 ||
           aps_FindGroup( pInMsg->msg->endPoint, scene.groupID ) != NULL )
      {
        // Either the Scene doesn't belong to a Group (Group ID = 0x0000) or it
        // does and the corresponding Group exits
        pScene = zclGeneral_FindScene( pInMsg->msg->endPoint, scene.groupID, scene.ID );
        if ( pScene || ( zclGeneral_CountAllScenes() < ZCL_GEN_MAX_SCENES ) )
        {
          uint8 sceneChanged = FALSE;

          status = ZCL_STATUS_SUCCESS;
          if ( pScene == NULL )
          {
            // Haven't been added yet
            pScene = &scene;
          }
      
          pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
          if ( pCBs && pCBs->pfnSceneStoreReq )
          {
            // Get the latest Scene info
            if ( pCBs->pfnSceneStoreReq( &(pInMsg->msg->srcAddr), pScene ) )
              sceneChanged = TRUE;
          }

          if ( pScene == &scene )
          {
            // The Scene doesn't exist so add it
            zclGeneral_AddScene( pInMsg->msg->endPoint, &scene );
          }
          else if ( sceneChanged )
          {
            // The Scene already exists so update only NV
            zclGeneral_ScenesWriteNV();
          }
        }
        else
          status = ZCL_STATUS_INSUFFICIENT_SPACE; // The Scene Table is full
      }
      else
        status = ZCL_STATUS_INVALID_FIELD; // The Group is not in the Group Table
      
      if ( pInMsg->msg->groupId == 0 )
      {
        // Addressed to this device (not to a group) - send a response back
        zclGeneral_SendSceneStoreResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr,
                                           status, scene.groupID, scene.ID, 
                                           pInMsg->hdr.transSeqNum );
      }
      break;

    case COMMAND_SCENE_RECALL:
      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      pScene = zclGeneral_FindScene( pInMsg->msg->endPoint, scene.groupID, scene.ID );
      if ( pScene && pCBs && pCBs->pfnSceneRecallReq )
      {
        pCBs->pfnSceneRecallReq( &(pInMsg->msg->srcAddr), pScene );
      }
      // No response
      break;

    case COMMAND_SCENE_GET_MEMBERSHIP:
      // Find all the Scenes corresponding to the Group ID
      if ( aps_FindGroup( pInMsg->msg->endPoint, scene.groupID ) != NULL )
      {
        sceneCnt = zclGeneral_FindAllScenesForGroup( pInMsg->msg->endPoint, 
                                                     scene.groupID, sceneList ); 
        status = ZCL_STATUS_SUCCESS;
        if ( pInMsg->msg->groupId == 0 )
        {
          // Addressed only to this device - send a response back
          sendRsp = TRUE;
        }
        else
        {
          // Addressed to the Group - ONLY send a response if an entry within the 
          // Scene Table corresponds to the Group ID
          if ( sceneCnt != 0 )
            sendRsp = TRUE;
        }
      }
      else
      {
        // The Group is not in the Group Table - send a response back
        status = ZCL_STATUS_INVALID_FIELD;
        sendRsp = TRUE;
      }

      if ( sendRsp )
      {
        zclGeneral_SendSceneGetMembershipResponse( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr, 
                                    status, zclGeneral_ScenesRemaingCapacity(), sceneCnt, sceneList, 
                                    scene.groupID, pInMsg->hdr.transSeqNum );
      }
      break;
      
    default:
      stat = ZFailure;
    break;
  }
  
  return ( stat );
}

/*********************************************************************
 * @fn      zclGeneral_ProcessInScenesClient
 *
 * @brief   Process in the received Scenes Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInScenesClient( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  zclGeneral_Scene_t scene;
  uint8 *pData = pInMsg->pData;
  uint8 nameLen;
  uint8 status;
  uint8 sceneCnt = 0;
  uint8 capacity;
  uint8 sceneList[ZCL_GEN_MAX_SCENES];
  uint8 i;
  ZStatus_t stat = ZSuccess;

  osal_memset( (uint8*)&scene, 0, sizeof( zclGeneral_Scene_t ) );

  // Get the status field first
  status = *pData++;

  if ( pInMsg->hdr.commandID == COMMAND_SCENE_GET_MEMBERSHIP_RSP )
    capacity = *pData++;

  scene.groupID = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );
  pData += 2;   // Move past group ID

  switch ( pInMsg->hdr.commandID )
  {   
    case COMMAND_SCENE_VIEW_RSP:
      // Parse the rest of the incoming message
      scene.ID = *pData++; // Not applicable to Remove All Response command
      scene.transTime = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );
      pData += 2;
      nameLen = *pData++; // Name length
      if ( nameLen > (ZCL_GEN_SCENE_NAME_LEN-1) )
        nameLen = (ZCL_GEN_SCENE_NAME_LEN-1);
      scene.name[0] = nameLen;
      osal_memcpy( &(scene.name[1]), pData, nameLen );
      pData += nameLen; // move pass name

      //*** Do something with the extension field(s)

      // Fall through to callback - break is left off intentionally

    case COMMAND_SCENE_ADD_RSP:
    case COMMAND_SCENE_REMOVE_RSP:
    case COMMAND_SCENE_REMOVE_ALL_RSP:
    case COMMAND_SCENE_STORE_RSP:
      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnSceneRsp )
      {
        pCBs->pfnSceneRsp( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID,
                           status, 0, (uint8 *)NULL, 0, &scene );
      }
      break;
      
    case COMMAND_SCENE_GET_MEMBERSHIP_RSP:
      if ( status == ZCL_STATUS_SUCCESS )
      {
        sceneCnt = *pData++;
        for ( i = 0; i < sceneCnt; i++ )
          sceneList[i] = *pData++;
      }

      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnSceneRsp )
      {
        pCBs->pfnSceneRsp( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID,
                           status, sceneCnt, sceneList, capacity, &scene );
      }
      break;
            
    default:
      stat = ZFailure;
      break;
  }
  
  return ( stat );
}
#endif // ZCL_SCENES

#ifdef ZCL_ON_OFF
/*********************************************************************
 * @fn      zclGeneral_ProcessInCmdOnOff
 *
 * @brief   Process in the received On/Off Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInOnOff( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    if ( pInMsg->hdr.commandID > COMMAND_TOGGLE )
      return ( ZFailure );   // Error ignore the command
   
    pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
    if ( pCBs && pCBs->pfnOnOff )
      pCBs->pfnOnOff( pInMsg->hdr.commandID );
  }
  // no Client command
  
  return ( ZSuccess );
}
#endif // ZCL_ON_OFF

#ifdef ZCL_LEVEL_CTRL
/*********************************************************************
 * @fn      zclGeneral_ProcessInLevelControl
 *
 * @brief   Process in the received Level Control Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInLevelControl( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  uint16 transTime;
  ZStatus_t stat = ZSuccess;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
    if ( pCBs )
    {
      switch ( pInMsg->hdr.commandID )
      {
        case COMMAND_LEVEL_MOVE_TO_LEVEL:
          if ( pCBs->pfnLevelControlMoveToLevel )
          {
            transTime = BUILD_UINT16( pInMsg->pData[1], pInMsg->pData[2] );
            pCBs->pfnLevelControlMoveToLevel( pInMsg->pData[0], transTime );
          }
          break;

        case COMMAND_LEVEL_MOVE:
          if ( pCBs->pfnLevelControlMove )
            pCBs->pfnLevelControlMove( pInMsg->pData[0], pInMsg->pData[1] );
          break;

        case COMMAND_LEVEL_STEP:
          if ( pCBs->pfnLevelControlStep )
          {
            transTime = BUILD_UINT16( pInMsg->pData[2], pInMsg->pData[3] );
            pCBs->pfnLevelControlStep( pInMsg->pData[0], pInMsg->pData[1], transTime );
          }
          break;
          
        default:
          stat = ZFailure;
          break;
      }
    }
  }
  // no Client command
  
  return ( stat );
}
#endif // ZCL_LEVEL_CTRL

#ifdef ZCL_ALARMS
/*********************************************************************
 * @fn      zclGeneral_AddAlarm
 *
 * @brief   Add an alarm for a cluster
 *
 * @param   endpoint -
 * @param   alarm - new alarm item
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGeneral_AddAlarm( uint8 endpoint, zclGeneral_Alarm_t *alarm )
{
  zclGenAlarmItem_t *pNewItem;
  zclGenAlarmItem_t *pLoop;

  // Fill in the new profile list
  pNewItem = osal_mem_alloc( sizeof( zclGenAlarmItem_t ) );
  if ( pNewItem == NULL )
    return ( ZMemError );

  // Fill in the plugin record.
  pNewItem->next = (zclGenAlarmItem_t *)NULL;
  pNewItem->endpoint =  endpoint;
  osal_memcpy( (uint8*)(&pNewItem->alarm), (uint8*)alarm, sizeof ( zclGeneral_Alarm_t ) );

  // Find spot in list
  if (  zclGenAlarmTable == NULL )
  {
    zclGenAlarmTable = pNewItem;
  }
  else
  {
    // Look for end of list
    pLoop = zclGenAlarmTable;
    while ( pLoop->next != NULL )
      pLoop = pLoop->next;

    // Put new item at end of list
    pLoop->next = pNewItem;
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zclGeneral_FindAlarm
 *
 * @brief   Find an alarm with alarmCode and clusterID
 *
 * @param   endpoint -
 * @param   groupID - what group the scene belongs to
 * @param   sceneID - ID to look for scene
 *
 * @return  a pointer to the alarm information, NULL if not found
 */
zclGeneral_Alarm_t *zclGeneral_FindAlarm( uint8 endpoint, uint8 alarmCode, uint16 clusterID )
{
  zclGenAlarmItem_t *pLoop;

  // Look for the alarm
  pLoop = zclGenAlarmTable;
  while ( pLoop )
  {
    if ( pLoop->endpoint == endpoint &&
         pLoop->alarm.code == alarmCode && pLoop->alarm.clusterID == clusterID )
    {
      return ( &(pLoop->alarm) );
    }
    pLoop = pLoop->next;
  }

  return ( (zclGeneral_Alarm_t *)NULL );
}

/*********************************************************************
 * @fn      zclGeneral_FindEarliestAlarm
 *
 * @brief   Find an alarm with the earliest timestamp
 *
 * @param   endpoint -
 *
 * @return  a pointer to the alarm information, NULL if not found
 */
zclGeneral_Alarm_t *zclGeneral_FindEarliestAlarm( uint8 endpoint )
{
  zclGenAlarmItem_t *pLoop;
  zclGenAlarmItem_t earliestAlarm;
  zclGenAlarmItem_t *pEarliestAlarm = &earliestAlarm;
  
  pEarliestAlarm->alarm.timeStamp = 0xFFFFFFFF;
  
  // Look for alarm with earliest time
  pLoop = zclGenAlarmTable;
  while ( pLoop )
  {
    if ( pLoop->endpoint == endpoint &&
         pLoop->alarm.timeStamp < pEarliestAlarm->alarm.timeStamp )
    {
      pEarliestAlarm = pLoop;
    }
    pLoop = pLoop->next;
  }

  if ( pEarliestAlarm->alarm.timeStamp != 0xFFFFFFFF )
    return ( &(pEarliestAlarm->alarm) );
  
  // No alarm
  return ( (zclGeneral_Alarm_t *)NULL );
}

/*********************************************************************
 * @fn      zclGeneral_ResetAlarm
 *
 * @brief   Remove a scene with endpoint and sceneID
 *
 * @param   endpoint -
 * @param   alarmCode - 
 * @param   clusterID - 
 *
 * @return  TRUE if removed, FALSE if not found
 */
void zclGeneral_ResetAlarm( uint8 endpoint, uint8 alarmCode, uint16 clusterID )
{
  zclGenAlarmItem_t *pLoop;
  zclGenAlarmItem_t *pPrev;

  // Look for end of list
  pLoop = zclGenAlarmTable;
  pPrev = NULL;
  while ( pLoop )
  {
    if ( pLoop->endpoint == endpoint &&
         pLoop->alarm.code == alarmCode && pLoop->alarm.clusterID == clusterID )
    {
      if ( pPrev == NULL )
        zclGenAlarmTable = pLoop->next;
      else
        pPrev->next = pLoop->next;

      // Free the memory
      osal_mem_free( pLoop );

      // Notify the Application so that if the alarm condition still active then
      // a new notification will be generated, and a new alarm record will be 
      // added to the alarm log
      // zclGeneral_NotifyReset( alarmCode, clusterID ); // callback function?
      return;
    }
    pPrev = pLoop;
    pLoop = pLoop->next;
  }
}

/*********************************************************************
 * @fn      zclGeneral_ResetAllAlarms
 *
 * @brief   Remove all alarms with endpoint
 *
 * @param   endpoint -
 * @param   notifyApp - 
 *
 * @return  none
 */
void zclGeneral_ResetAllAlarms( uint8 endpoint, uint8 notifyApp )
{
  zclGenAlarmItem_t *pLoop;
  zclGenAlarmItem_t *pPrev;
  zclGenAlarmItem_t *pNext;

  // Look for end of list
  pLoop = zclGenAlarmTable;
  pPrev = NULL;
  while ( pLoop )
  {
    if (  pLoop->endpoint == endpoint )
    {
      if ( pPrev == NULL )
        zclGenAlarmTable = pLoop->next;
      else
        pPrev->next = pLoop->next;

      pNext = pLoop->next;

      // Free the memory
      osal_mem_free( pLoop );

      pLoop = pNext;
    }
    else
    {
      pPrev = pLoop;
      pLoop = pLoop->next;
    }
  }
  
  if ( notifyApp )
  {
    // Notify the Application so that if any alarm conditions still active then
    // a new notification will be generated, and a new alarm record will be 
    // added to the alarm log
    // zclGeneral_NotifyResetAll(); // callback function?
  }
}

/*********************************************************************
 * @fn      zclGeneral_ProcessInAlarmsServer
 *
 * @brief   Process in the received Alarms Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInAlarmsServer( zclIncoming_t *pInMsg )
{
  zclGeneral_Alarm_t *pAlarm;
  uint8 *pData = pInMsg->pData;
  ZStatus_t stat = ZSuccess;

  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_ALARMS_RESET: 
      zclGeneral_ResetAlarm( pInMsg->msg->endPoint, pData[0], 
                             BUILD_UINT16( pData[1], pData[2] ) );
      break;

    case COMMAND_ALARMS_RESET_ALL: 
      zclGeneral_ResetAllAlarms( pInMsg->msg->endPoint, TRUE );
      break;

    case COMMAND_ALARMS_GET: 
      pAlarm = zclGeneral_FindEarliestAlarm( pInMsg->msg->endPoint );
      if ( pAlarm )
      {
        // Send a response back
        zclGeneral_SendAlarmGetRespnose( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr,
                                         ZCL_STATUS_SUCCESS, pAlarm->code, 
                                         pAlarm->clusterID, pAlarm->timeStamp,
                                         pInMsg->hdr.transSeqNum );
        // Remove the entry from the Alarm table
        zclGeneral_ResetAlarm( pInMsg->msg->endPoint, pAlarm->code, pAlarm->clusterID );
      }
      else
      {
        // Send a response back
        zclGeneral_SendAlarmGetRespnose( pInMsg->msg->endPoint, &pInMsg->msg->srcAddr,
                                         ZCL_STATUS_NOT_FOUND, 0, 0, 0, 
                                         pInMsg->hdr.transSeqNum );
      }
      break;
      
    case COMMAND_ALARMS_RESET_LOG: 
      zclGeneral_ResetAllAlarms( pInMsg->msg->endPoint, FALSE );
      break;
      
    default:            
      stat = ZFailure;
      break;
  }
  
  return ( stat );
}

/*********************************************************************
 * @fn      zclGeneral_ProcessInAlarmsClient
 *
 * @brief   Process in the received Alarms Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInAlarmsClient( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  uint8 *pData = pInMsg->pData;
  uint16 clusterID;
  uint8 alarmCode;
  uint32 timeStamp;
  uint8 status;
  ZStatus_t stat = ZSuccess;

  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_ALARMS_ALARM: 
      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnAlarm )
      {
        status = *pData++;
        alarmCode = *pData++;
        clusterID = BUILD_UINT16( pData[0], pData[1] );
        pData += 2;
        timeStamp = BUILD_UINT32( pData[0], pData[1], pData[2], pData[3] );
      
        pCBs->pfnAlarm( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID, 
                        status, alarmCode, clusterID, timeStamp );
      }
      break;  
      
    case COMMAND_ALARMS_GET_RSP: 
      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnAlarm )
      {
        alarmCode = *pData++;
        clusterID = BUILD_UINT16( pData[0], pData[1] );
      
        pCBs->pfnAlarm( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID, 
                        0, alarmCode, clusterID, 0 );
      }
      break;
   
    default:            
      stat = ZFailure;
      break;
  }
  
  return ( stat );
}
#endif // ZCL_ALARMS

#ifdef ZCL_LOCATION
/*********************************************************************
 * @fn      zclGeneral_ProcessInLocationServer
 *
 * @brief   Process in the received Location Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInLocationServer( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  uint8 *pData = pInMsg->pData;
  zclLocationAbsolute_t absLoc;
  zclLocationDevCfg_t devCfg;
  zclLocationGetData_t loc;
  ZStatus_t stat = ZSuccess;      

  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_LOCATION_SET_ABSOLUTE:      
      absLoc.coordinate1 = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      absLoc.coordinate2 = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      absLoc.coordinate3 = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      absLoc.power = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      absLoc.pathLossExponent = BUILD_UINT16( pData[0], pData[1] );

      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnLocation )
      {
        // Update the absolute location info
        pCBs->pfnLocation( &pInMsg->msg->srcAddr, pInMsg->hdr.commandID, 
                           &absLoc, NULL, NULL, NULL, 0 );
      }
      break;
      
    case COMMAND_LOCATION_SET_DEV_CFG:
      devCfg.power = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      devCfg.pathLossExponent = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      devCfg.calcPeriod = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      devCfg.numMeasurements = *pData++;
      devCfg.reportPeriod = BUILD_UINT16( pData[0], pData[1] );

      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnLocation )
      {
        // Update the device configuration info
        pCBs->pfnLocation( &pInMsg->msg->srcAddr, pInMsg->hdr.commandID, 
                           NULL, NULL, &devCfg, NULL, 0 );
      }
      break;
      
    case COMMAND_LOCATION_GET_DEV_CFG: 
      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnLocation )
      {
        // Retreive the Device Configuration 
        pCBs->pfnLocation( &pInMsg->msg->srcAddr, pInMsg->hdr.commandID, NULL, NULL,
                           NULL, pData, pInMsg->hdr.transSeqNum );
      }
      break;
      
    case COMMAND_LOCATION_GET_DATA: 
      loc.bitmap.locByte = *pData++;
      loc.numResponses = *pData++;
      
      if ( loc.brdcastResponse == 0 ) // command is sent as a unicast
        osal_cpyExtAddr( loc.targetAddr, pData );
      
      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnLocation )
      {
        // Retreive the Location Data
        pCBs->pfnLocation( &pInMsg->msg->srcAddr, pInMsg->hdr.commandID, NULL, &loc,
                           NULL, NULL, pInMsg->hdr.transSeqNum );
      }
      break;
           
    default:            
      stat = ZFailure;
      break;
  }
  
  return ( stat );
}

/*********************************************************************
 * @fn      zclGeneral_ProcessInLocationDataRsp
 *
 * @brief   Process in the received Location Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static void zclGeneral_ProcessInLocationDataRsp( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  uint8 *pData = pInMsg->pData;
  zclLocationDataRsp_t loc;

  pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
  if ( pCBs && pCBs->pfnLocationRsp )
  {
    if ( pInMsg->hdr.commandID == COMMAND_LOCATION_DATA_RSP )
      loc.status = *pData++;
    
    if ( pInMsg->hdr.commandID != COMMAND_LOCATION_DATA_RSP || 
         loc.status == ZCL_STATUS_SUCCESS )
    {
      loc.data.type = *pData++;
      loc.data.absLoc.coordinate1 = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
      loc.data.absLoc.coordinate2 = BUILD_UINT16( pData[0], pData[1] );
      pData += 2;
          
      if ( locationType2D( loc.data.type ) == 0 )
      { 
        loc.data.absLoc.coordinate3 = BUILD_UINT16( pData[0], pData[1] );
        pData += 2;
      }

      if ( pInMsg->hdr.commandID != COMMAND_LOCATION_COMPACT_DATA_NOTIF )
      {
        loc.data.absLoc.power = BUILD_UINT16( pData[0], pData[1] );
        pData += 2;
        loc.data.absLoc.pathLossExponent = BUILD_UINT16( pData[0], pData[1] );
        pData += 2;
      }

      if ( locationTypeAbsolute( loc.data.type ) == 0 )
      {
        if ( pInMsg->hdr.commandID != COMMAND_LOCATION_COMPACT_DATA_NOTIF )
          loc.data.calcLoc.locationMethod = *pData++;

        loc.data.calcLoc.qualityMeasure = *pData++;
        loc.data.calcLoc.locationAge = BUILD_UINT16( pData[0], pData[1] );
      }
    }
        
    // Notify the Application
    pCBs->pfnLocationRsp( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID, &loc, NULL, 0 );
  }
}
 
/*********************************************************************
 * @fn      zclGeneral_ProcessInLocationClient
 *
 * @brief   Process in the received Location Command.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGeneral_ProcessInLocationClient( zclIncoming_t *pInMsg )
{
  zclGeneral_AppCallbacks_t *pCBs;
  uint8 *pData = pInMsg->pData;
  ZStatus_t stat = ZSuccess;      
  
  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_LOCATION_DEV_CFG_RSP: 
      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnLocationRsp )
      {
        zclLocationDevCfgRsp_t devCfgRsp;

        devCfgRsp.status = *pData++;
        if ( devCfgRsp.status == ZCL_STATUS_SUCCESS )
        {
          devCfgRsp.data.power = BUILD_UINT16( pData[0], pData[1] );
          pData += 2;
          devCfgRsp.data.pathLossExponent = BUILD_UINT16( pData[0], pData[1] );
          pData += 2;
          devCfgRsp.data.calcPeriod = BUILD_UINT16( pData[0], pData[1] );
          pData += 2;
          devCfgRsp.data.numMeasurements = *pData++;
          devCfgRsp.data.reportPeriod = BUILD_UINT16( pData[0], pData[1] );
        
          // Notify the Application
          pCBs->pfnLocationRsp( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID, 
                                NULL, &devCfgRsp, 0 );
        }
      }
      break;
      
    case COMMAND_LOCATION_DATA_RSP: 
    case COMMAND_LOCATION_DATA_NOTIF: 
    case COMMAND_LOCATION_COMPACT_DATA_NOTIF: 
      zclGeneral_ProcessInLocationDataRsp( pInMsg );
      break;
     
    case COMMAND_LOCATION_RSSI_PING:
      pCBs = zclGeneral_FindCallbacks( pInMsg->msg->endPoint );
      if ( pCBs && pCBs->pfnLocationRsp )
      {
        uint8 locationType = *pData;

        // Notify the Application
        pCBs->pfnLocationRsp( &(pInMsg->msg->srcAddr), pInMsg->hdr.commandID,
                              NULL, NULL, locationType );
      }
      break;
          
    default:            
      stat = ZFailure;
      break;
  }
  
  return ( stat );
}
#endif // ZCL_LOCATION

#ifdef ZCL_SCENES
/*********************************************************************
 * @fn      zclGeneral_ScenesInitNV
 *
 * @brief   Initialize the NV Scene Table Items
 *
 * @param   none
 *
 * @return  number of scenes
 */
static uint8 zclGeneral_ScenesInitNV( void )
{
  uint8  status;
  uint16 size;

  size = (uint16)((sizeof ( nvGenScenesHdr_t )) 
                  + ( sizeof( zclGenSceneNVItem_t ) * ZCL_GEN_MAX_SCENES ));

  status = osal_nv_item_init( ZCD_NV_SCENE_TABLE, size, NULL );

  if ( status != ZSUCCESS )
  {
    zclGeneral_ScenesSetDefaultNV();
  }

  return status;
}

/*********************************************************************
 * @fn          zclGeneral_ScenesSetDefaultNV
 *
 * @brief       Write the defaults to NV
 *
 * @param       none
 *
 * @return      none
 */
static void zclGeneral_ScenesSetDefaultNV( void )
{
  nvGenScenesHdr_t hdr;

  // Initialize the header
  hdr.numRecs = 0;

  // Save off the header
  osal_nv_write( ZCD_NV_SCENE_TABLE, 0, sizeof( nvGenScenesHdr_t ), &hdr );
}

/*********************************************************************
 * @fn          zclGeneral_ScenesWriteNV
 *
 * @brief       Save the Scene Table in NV
 *
 * @param       none
 *
 * @return      none
 */
static void zclGeneral_ScenesWriteNV( void )
{
  nvGenScenesHdr_t hdr;
  zclGenSceneItem_t *pLoop;
  zclGenSceneNVItem_t item;
  
  hdr.numRecs = 0;
  
  // Look for end of list
  pLoop = zclGenSceneTable;
  while ( pLoop )
  {
    // Build the record
    item.endpoint = pLoop->endpoint;
    osal_memcpy( &(item.scene), &(pLoop->scene), sizeof ( zclGeneral_Scene_t ) );
    
    // Save the record to NV
    osal_nv_write( ZCD_NV_SCENE_TABLE,
            (uint16)((sizeof( nvGenScenesHdr_t )) + (hdr.numRecs * sizeof ( zclGenSceneNVItem_t ))),
                    sizeof ( zclGenSceneNVItem_t ), &item );
    
    hdr.numRecs++;
    
    pLoop = pLoop->next;
  }

  // Save off the header
  osal_nv_write( ZCD_NV_SCENE_TABLE, 0, sizeof( nvGenScenesHdr_t ), &hdr );
}

/*********************************************************************
 * @fn          zclGeneral_ScenesRestoreFromNV
 *
 * @brief       Restore the Scene table from NV
 *
 * @param       none
 *
 * @return      Number of entries restored
 */
static uint16 zclGeneral_ScenesRestoreFromNV( void )
{
  uint16 x;
  nvGenScenesHdr_t hdr;

  zclGenSceneNVItem_t item;
  uint16 numAdded = 0;

  if ( osal_nv_read( ZCD_NV_SCENE_TABLE, 0, sizeof(nvGenScenesHdr_t), &hdr ) == ZSuccess )
  {
    // Read in the device list
    for ( x = 0; x < hdr.numRecs; x++ )
    {
      if ( osal_nv_read( ZCD_NV_SCENE_TABLE,
                (uint16)(sizeof(nvGenScenesHdr_t) + (x * sizeof ( zclGenSceneNVItem_t ))),
                                  sizeof ( zclGenSceneNVItem_t ), &item ) == ZSUCCESS )
      {
        // Add the scene
        if ( zclGeneral_AddScene( item.endpoint, &(item.scene) ) == ZSuccess )
        {
          numAdded++;
        }
      }
    }
  }
  
  return ( numAdded );
}
#endif // ZCL_SCENES

/***************************************************************************
****************************************************************************/
