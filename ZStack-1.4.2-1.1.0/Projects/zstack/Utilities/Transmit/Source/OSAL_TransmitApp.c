/*********************************************************************
    Filename:       OSAL_TransmitApp.c
    Revised:        $Date: 2006-11-02 10:11:07 -0700 (Thu, 02 Nov 2006) $
    Revision:       $Revision: 12491 $

    Description:

    This file contains all the settings and other functions that
    the user should set and change.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/


/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Custom.h"

#if defined ( MT_TASK )
  #include "MTEL.h"
#endif

#if !defined( NONWK )
  #include "nwk.h"
  #include "APS.h"
  #include "ZDApp.h"
#endif

#include "TransmitApp.h"

#include "OnBoard.h"

#include "hal_drivers.h"


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * USER DEFINED TASK TABLE
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

 /*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      osalAddTasks
 *
 * @brief   This function adds all the tasks to the task list.
 *          This is where to add new tasks.
 *
 * @param   void
 *
 * @return  none
 */
void osalAddTasks( void )
{

/*
  This task must be loaded first because Hal_Init() initializes
  many things that other task_init functions may need.
*/
  osalTaskAdd (Hal_Init, Hal_ProcessEvent, OSAL_TASK_PRIORITY_LOW);

#if defined( ZMAC_F8W )
  osalTaskAdd( macTaskInit, macEventLoop, OSAL_TASK_PRIORITY_HIGH );
#endif

#if defined( MT_TASK )
  osalTaskAdd( MT_TaskInit, MT_ProcessEvent,  OSAL_TASK_PRIORITY_LOW);
#endif

  osalTaskAdd( nwk_init, nwk_event_loop, OSAL_TASK_PRIORITY_MED);
  osalTaskAdd( APS_Init, APS_event_loop, OSAL_TASK_PRIORITY_LOW);
  osalTaskAdd( ZDApp_Init, ZDApp_event_loop, OSAL_TASK_PRIORITY_LOW);

  osalTaskAdd( TransmitApp_Init, TransmitApp_ProcessEvent, OSAL_TASK_PRIORITY_LOW);
}

/*********************************************************************
*********************************************************************/
