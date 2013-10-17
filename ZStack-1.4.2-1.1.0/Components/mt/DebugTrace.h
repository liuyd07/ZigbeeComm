#ifndef DEBUGTRACE_H
#define DEBUGTRACE_H

/*********************************************************************
    Filename:       DebugTrace.h
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

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */


/*********************************************************************
 * MACROS
 */

/*
 * Windows Print String
 */
// command id's		
#define CMDID_RTG_ADD					1
#define CMDID_RTG_EXP					0x81
#define CMDID_RREQ_SEND				2
#define CMDID_RREQ_DROP				0x82
#define CMDID_RREP_SEND				3
#define CMDID_RREP_DROP				0x83
#define CMDID_RREQ_EXP				4

#define CMDID_DATA_SEND					6
#define CMDID_DATA_FORWARD			7
#define CMDID_DATA_RECEIVE			8

#define CMDID_BCAST_RCV				0x10
#define CMDID_BCAST_ACK				0x11
#define CMDID_BCAST_RETX			0x12

#define CMDID_BCAST_EXP				0x13
#define CMDID_BCAST_ERR				0x15

#define WPRINTSTR( s )
 
#if defined ( MT_TASK )
  /*
   * Trace Message 
   *       - Same as debug_msg with SEVERITY_TRACE already filled in
   *       - Used to stand out in the source code.
   */
  #define TRACE_MSG( compID, nParams, p1, p2, p3 )  debug_msg( compID, SEVERITY_TRACE, nParams, p1, p2, p3 )


  /*
   * Debug Message (SEVERITY_INFORMATION)
   *      - Use this macro instead of calling debug_msg directly
   *      - So, it can be easily compiled out later
   */
  #define DEBUG_INFO( compID, subCompID, nParams, p1, p2, p3 )  debug_msg( compID, subCompID, nParams, p1, p2, p3 )


  /*** TEST MESSAGES ***/
  #define DBG_NWK_STARTUP           debug_msg( COMPID_TEST_NWK_STARTUP,         SEVERITY_INFORMATION, 0, 0, 0, 0 )
  #define DBG_SCAN_CONFIRM          debug_msg( COMPID_TEST_SCAN_CONFIRM,        SEVERITY_INFORMATION, 0, 0, 0, 0 )
  #define DBG_ASSOC_CONFIRM         debug_msg( COMPID_TEST_ASSOC_CONFIRM,       SEVERITY_INFORMATION, 0, 0, 0, 0 )
  #define DBG_REMOTE_DATA_CONFIRM   debug_msg( COMPID_TEST_REMOTE_DATA_CONFIRM, SEVERITY_INFORMATION, 0, 0, 0, 0 )

#else

  #define TRACE_MSG( compID, nParams, p1, p2, p3 )
  #define DEBUG_INFO( compID, subCompID, nParams, p1, p2, p3 )
  #define DBG_NWK_STARTUP
  #define DBG_SCAN_CONFIRM
  #define DBG_ASSOC_CONFIRM
  #define DBG_REMOTE_DATA_CONFIRM

#endif  

/*********************************************************************
 * CONSTANTS
 */

#define SEVERITY_CRITICAL     0x01
#define SEVERITY_ERROR        0x02
#define SEVERITY_INFORMATION  0x03
#define SEVERITY_TRACE        0x04

#define NO_PARAM_DEBUG_LEN   5
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

  /*
   * Debug Message - Sent out serial port
   */


extern void debug_msg( byte compID, byte severity, byte numParams, 
                       UINT16 param1, UINT16 param2, UINT16 param3 );
                       
extern void debug_str( byte *str_ptr );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* DEBUGTRACE_H */
