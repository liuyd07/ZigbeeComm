/*********************************************************************
    Filename:       DebugTrace.c
    Revised:        $Date: 2006-08-03 11:44:57 -0700 (Thu, 03 Aug 2006) $
    Revision:       $Revision: 11593 $

    Description:

       This interface provides quick one-function-call functions to
       Monitor and Test reporting mechanisms.
          *	Log errors into non-volatile memory
          * Debugging mechanism to "print/track" progress and data
            in real-time

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

#if defined( MT_TASK ) || defined( APP_DEBUG )

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "MTEL.h"
#include "DebugTrace.h"

#if defined ( APP_DEBUG )
  #include "DebugApp.h"
#endif

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
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

 /*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      debug_msg
 *
 * @brief
 *
 *   This feature allows modules to display debug information as
 *   applications execute in real-time.  This feature will work similar
 *   to "printf()" but will output to the serial port for display in
 *   the Z-Test tool.
 *
 *   This feature will most likely be compiled out in the production code
 *   to save code space.
 *
 * @param   byte compID - Component ID
 * @param   byte severity - CRITICAL(0x01), ERROR(0x02), INFORMATION(0x03)
 *                          or TRACE(0x04)
 * @param   byte numParams - number of parameter fields (param1-3)
 * @param   UINT16 param1 - user defined data
 * @param   UINT16 param2 - user defined data
 * @param   UINT16 param3 - user defined data
 *
 * @return  void
 */
void debug_msg( byte compID, byte severity, byte numParams, UINT16 param1,
																								UINT16 param2, UINT16 param3 )
{

  mtDebugMsg_t *mtDebugMsg;
  UINT16 timestamp;

#if defined ( APP_DEBUG ) && !defined (ZDO_COORDINATOR)
  DebugApp_BuildMsg( compID, severity, numParams, param1, param2, param3 );
  return;
#endif

  if ( debugThreshold == 0 || debugCompId != compID )
    return;

  // Fill in the timestamp
  timestamp = 0;

  // Get a message buffer to build the debug message
  mtDebugMsg = (mtDebugMsg_t *)osal_msg_allocate( sizeof( mtDebugMsg_t ) );
  if ( mtDebugMsg )
  {
      mtDebugMsg->hdr.event = CMD_DEBUG_MSG;
      mtDebugMsg->compID = compID;
      mtDebugMsg->severity = severity;
      mtDebugMsg->numParams = numParams;

      mtDebugMsg->param1 = param1;
      mtDebugMsg->param2 = param2;
      mtDebugMsg->param3 = param3;
      mtDebugMsg->timestamp = timestamp;
      
      osal_msg_send( MT_TaskID, (uint8 *)mtDebugMsg );
  }

} /* debug_msg() */

/*********************************************************************
 * @fn      debug_str
 *
 * @brief
 *
 *   This feature allows modules to display a debug text string as
 *   applications execute in real-time. This feature will output to
 *   the serial port for display in the Z-Test tool.
 *
 *   This feature will most likely be compiled out in the production
 *   code in order to save code space.
 *
 * @param   byte *str_ptr - pointer to null-terminated string
 *
 * @return  void
 */
void debug_str( byte *str_ptr )
{
  mtDebugStr_t *msg;
  byte mln;
  byte sln;

  // Text string length
  sln = (byte)osal_strlen( (void*)str_ptr );

  // Debug string message length
  mln = sizeof ( mtDebugStr_t ) + sln;

  // Get a message buffer to build the debug message
  msg = (mtDebugStr_t *)osal_msg_allocate( mln );
  if ( msg )
  {
    // Message type, length
    msg->hdr.event = CMD_DEBUG_STR;
    msg->sln = sln;

    // Append message, no terminator
    msg->pString = (uint8 *)(msg+1);
    osal_memcpy ( msg->pString, str_ptr, sln );

    osal_msg_send( MT_TaskID, (uint8 *)msg );
  }
} // debug_str()

/*********************************************************************
*********************************************************************/
#endif  // MT_TASK
