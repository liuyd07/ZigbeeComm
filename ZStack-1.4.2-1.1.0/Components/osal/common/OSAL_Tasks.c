/*********************************************************************
    Filename:       OSAL_Tasks.c
    Revised:        $Date: 2006-04-06 08:19:08 -0700 (Thu, 06 Apr 2006) $
    Revision:       $Revision: 10396 $

    Description:

       This file contains the OSAL Task definition and manipulation
       functions.

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

// 任务控制
osalTaskRec_t *tasksHead;

osalTaskRec_t *activeTask;

byte taskIDs;

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
 * @fn      osalTaskInit
 *
 * @brief   初始化任务系统
 *
 * @param   none
 *
 * @return
 */
void osalTaskInit( void )
{
  tasksHead = (osalTaskRec_t *)NULL;
  activeTask = (osalTaskRec_t *)NULL;
  taskIDs = 0;
}

/***************************************************************************
 * @fn      osalTaskAdd
 *
 * @brief   添加一个任务到任务列表. 保持任务队列的优先级.
 *
 * @param   none
 *
 * @return
 */
void osalTaskAdd( pTaskInitFn pfnInit,
                  pTaskEventHandlerFn pfnEventProcessor,
                  byte taskPriority)
{
  osalTaskRec_t *newTask;
  osalTaskRec_t *srchTask;
  osalTaskRec_t **ptr;

  newTask = osal_mem_alloc( sizeof( osalTaskRec_t ) );
  if ( newTask )
  {
      // 新任务
      newTask->pfnInit           = pfnInit;
      newTask->pfnEventProcessor = pfnEventProcessor;
      newTask->taskID            = taskIDs++;
      newTask->taskPriority      = taskPriority;
      newTask->events            = 0;
      newTask->next              = (osalTaskRec_t *)NULL;

      // 当一个新任务是嵌入的，‘ptr’是指向一个新任务的地址。它是在设置一个地址给‘taskshead’
      ptr      = &tasksHead;
      srchTask = tasksHead;
      while (srchTask)  {
          if (newTask->taskPriority > srchTask->taskPriority)  {
              // 嵌入到这里.新任务有一个递增型优先秩序，
              newTask->next = srchTask;
              *ptr          = newTask;
              return;
          }
          // set 'ptr' to address of the pointer to 'next' in the current
          // (soon to be previous) task control block
          ptr      = &srchTask->next;
          srchTask = srchTask->next;
      }

	  // We're at the end of the current queue. New task is not higher
	  // priority than any other already in the list. Make it the tail.
      // (It is also the head if the queue was initially empty.)
      *ptr = newTask;
  }
  return;
}

/*********************************************************************
 * @fn      osalInitTasks
 *
 * @brief   调用各自任务的初始化函数
 *
 * @param   none
 *
 * @return  none
 */
void osalInitTasks( void )
{
  // Start at the beginning
  activeTask = tasksHead;

  // Stop at the end
  while ( activeTask )
  {
    if (  activeTask->pfnInit  )
      activeTask->pfnInit( activeTask->taskID );

    activeTask = activeTask->next;
  }

  activeTask = (osalTaskRec_t *)NULL;
}

/*********************************************************************
 * @fn      osalNextActiveTask
 *
 * @brief   这个函数将返回下一个被激活的任务
 *
 *
 * @param   none
 *
 * @return  指向发生的事件的任务，NULL为没有发现事件
 */
osalTaskRec_t *osalNextActiveTask( void )
{
  osalTaskRec_t *srchTask;

  // Start at the beginning
  srchTask = tasksHead;

  // When found or not
  while ( srchTask )  {
      if (srchTask->events)  {
		  // 高优先级的任务准备
          return srchTask;
      }
      srchTask = srchTask->next;
  }
  return NULL;
}


/*********************************************************************
 * @fn      osalFindTask
 *
 * @brief   这个函数将返回一个指向任务的被通过的任务ID
 *
 * @param   taskID - task ID to look for
 *
 * @return  pointer to the found task, NULL if not found
 */
osalTaskRec_t *osalFindTask( byte taskID )
{
  osalTaskRec_t *srchTask;

  // Start at the beginning
  srchTask = tasksHead;

  // When found or not
  while ( srchTask )
  {
    // Look for any activity
    if (  srchTask->taskID == taskID  )
      return ( srchTask );

    srchTask = srchTask->next;
  }

  return ( (osalTaskRec_t *)NULL );
}

/*********************************************************************
*********************************************************************/
