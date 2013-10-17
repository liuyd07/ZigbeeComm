#ifndef APSGROUPS_H
#define APSGROUPS_H
/*********************************************************************
    Filename:       aps_groups.h
    Revised:        $Date: 2006-09-08 15:51:21 -0700 (Fri, 08 Sep 2006) $
    Revision:       $Revision: 11934 $

    Description:

    Application Support Sub Layer group management functions.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/*********************************************************************
 * MACROS
 */
#define aps_GroupsRemaingCapacity() ( APS_MAX_GROUPS - aps_CountAllGroups() )
  
/*********************************************************************
 * CONSTANTS
 */
#define APS_GROUPS_FIND_FIRST           0xFE
#define APS_GROUPS_EP_NOT_FOUND         0xFE

#define APS_GROUP_NAME_LEN              16
  
/*********************************************************************
 * TYPEDEFS
 */

// Group Table Element
typedef struct
{
  uint16 ID;                       // Unique to this table
  uint8  name[APS_GROUP_NAME_LEN]; // Human readable name of group
} aps_Group_t;

typedef struct apsGroupItem
{
  struct apsGroupItem  *next;
  uint8                endpoint;
  aps_Group_t          group;
} apsGroupItem_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Add a group for an endpoint
 */
extern ZStatus_t aps_AddGroup( uint8 endpoint, aps_Group_t *group );

/*
 * Find a group with endpoint and groupID
 *  - returns a pointer to the group information, NULL if not found
 */
extern aps_Group_t *aps_FindGroup( uint8 endpoint, uint16 groupID );

/*
 * Find a group for an endpoint
 *  - returns endpoint found, or 0xFF for not found
 */
extern uint8 aps_FindGroupForEndpoint( uint16 groupID, uint8 lastEP );

/*
 * Find all groups for an endpoint
 *  - returns number of groups copied to groupList
 */
extern uint8 aps_FindAllGroupsForEndpoint( uint8 endpoint, uint16 *groupList );

/*
 * Remove a group with endpoint and groupID
 *  - returns TRUE if removed, FALSE if not found
 */
extern uint8 aps_RemoveGroup( uint8 endpoint, uint16 groupID );

/*
 * Remove all groups for endpoint
 */
extern void aps_RemoveAllGroup( uint8 endpoint );

/*
 * Count the number of groups for an endpoint
 */
extern uint8 aps_CountGroups( uint8 endpoint );

/*
 * Count the number of groups
 */
extern uint8 aps_CountAllGroups( void );

/*
 * Initialize the Group Table NV Space
 */
extern uint8 aps_GroupsInitNV( void );

/*
 * Initialize the Group Table NV Space to default (no entries)
 */
extern void aps_GroupsSetDefaultNV( void );

/*
 * Write the group table to NV
 */
extern void aps_GroupsWriteNV( void );

/*
 * Read the group table from NV
 */
extern uint16 aps_GroupsRestoreFromNV( void );

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* APSGROUPS_H */


