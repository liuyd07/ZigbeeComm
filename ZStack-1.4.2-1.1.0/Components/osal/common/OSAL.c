/*********************************************************************
    Filename:       OSAL.c
    Revised:        $Date: 2006-10-26 11:15:57 -0700 (Thu, 26 Oct 2006) $
    Revision:       $Revision: 12421 $

    Description:

       This API allows the software components in the Z-stack to be
       written independently of the specifics of the operating system,
       kernel or tasking environment (including control loops or
       connect-to-interrupt systems).

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/


/*********************************************************************
 * INCLUDES
 */
//#ifdef WIN32
//  #include <stdio.h>
//#endif

#include <stdlib.h>
#include <string.h>

#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Custom.h"
#include "OSAL_Memory.h"
#include "OSAL_PwrMgr.h"
#include "hal_mcu.h"

#include "OnBoard.h"

/* HAL */
#include "hal_drivers.h"

/*********************************************************************
 * MACROS
 */

#define OSAL_MSG_LEN(msg_ptr)      ((osal_msg_hdr_t *) (msg_ptr) - 1)->len

#define OSAL_MSG_ID(msg_ptr)      ((osal_msg_hdr_t *) (msg_ptr) - 1)->dest_id

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Message Pool Definitions
osal_msg_q_t osal_qHead;

#if defined( OSAL_TOTAL_MEM )
  UINT16 osal_msg_cnt;
#endif

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
 * HELPER FUNCTIONS
 */
/* very ugly stub so Keil can compile */
#ifdef __KEIL__
char *  itoa ( int value, char * buffer, int radix )
{
  return(buffer);
}
#endif

/*********************************************************************
 * @fn      osal_strlen
 *
 * @brief
 *
 *   计算一个字符串的长度.  这个字符串必须是NULL终止
 *
 * @param   char *pString - pointer to text string
 *
 * @return  int - number of characters
 */
int osal_strlen( char *pString )
{
  return strlen( pString );
}

/*********************************************************************
 * @fn      osal_memcpy
 *
 * @brief
 *
 *   Generic memory copy.
 *
 *   Note: This function differs from the standard memcpy(), since
 *         it returns the pointer to the next destination byte. The
 *         standard memcpy() returns the original destination address.
 *
 * @param   dst - destination address
 * @param   src - source address
 * @param   len - number of bytes to copy
 *
 * @return  pointer to end of destination buffer
 */
void *osal_memcpy( void *dst, const void GENERIC *src, unsigned int len )
{
  byte *pDst;
  const byte GENERIC *pSrc;

  pSrc = src;
  pDst = dst;

  while ( len-- )
    *pDst++ = *pSrc++;

  return ( pDst );
}

/*********************************************************************
 * @fn      osal_cpyExtAddr
 *
 * @brief
 *
 *   拷贝扩展地址这个函数将拷贝8字节
 *
 * @param   dest  目的地址
 * @param   src   源地址
 *
 * @return  dest + Z_EXTADDR_LEN
 */
void *osal_cpyExtAddr( void *dest, void *src )
{
  return osal_memcpy( dest, src, Z_EXTADDR_LEN );
}

/*********************************************************************
 * @fn      osal_memset
 *
 * @brief
 *
 *   设置存储缓冲器值
 *
 * @param   dest - 指向缓冲器
 * @param   value - 设置的值是多少
 * @param   size - 大小
 *
 * @return  value of next widget, 0 if no widget found
 */
void *osal_memset( void *dest, byte value, int len )
{
  return memset( dest, value, len );
}

/*********************************************************************
 * @fn      osal_build_uint16
 *
 * @brief
 *
 *   Build a uint16 out of 2 bytes (0 then 1).
 *
 * @param   swapped - 0 then 1
 *
 * @return  uint16
 */
uint16 osal_build_uint16( byte *swapped )
{
  return ( BUILD_UINT16( swapped[0], swapped[1] ) );
}

/*********************************************************************
 * @fn      osal_build_uint32
 *
 * @brief
 *
 *   Build a uint32 out of sequential bytes.
 *
 * @param   swapped - sequential bytes
 * @param   len - number of bytes in the byte array
 *
 * @return  uint32
 */
uint32 osal_build_uint32( byte *swapped, byte len )
{
  if ( len == 2 )
    return ( BUILD_UINT32( swapped[0], swapped[1], 0L, 0L ) );
  else if ( len == 3 )
    return ( BUILD_UINT32( swapped[0], swapped[1], swapped[2], 0L ) );
  else if ( len == 4 )
    return ( BUILD_UINT32( swapped[0], swapped[1], swapped[2], swapped[3] ) );
  else
    return ( (uint32)swapped[0] );
}

#if !defined ( ZBIT ) && !defined ( ZBIT2 )
/*********************************************************************
 * @fn      _ltoa
 *
 * @brief
 *
 *   convert a long unsigned int to a string.
 *
 * @param  l - long to convert
 * @param  buf - buffer to convert to
 * @param  radix - 10 dec, 16 hex
 *
 * @return  pointer to buffer
 */
unsigned char * _ltoa(unsigned long l, unsigned char *buf, unsigned char radix)
{
#if defined( __GNUC__ )
  return ( (char*)ltoa( l, buf, radix ) );
#else
  unsigned char tmp1[10] = "", tmp2[10] = "", tmp3[10] = "";
  unsigned short num1, num2, num3;
  unsigned char i;

  buf[0] = '\0';

  if ( radix == 10 )
  {
    num1 = l % 10000;
    num2 = (l / 10000) % 10000;
    num3 = (unsigned short)(l / 100000000);

    if (num3) _itoa(num3, tmp3, 10);
    if (num2) _itoa(num2, tmp2, 10);
    if (num1) _itoa(num1, tmp1, 10);

    if (num3)
    {
      strcpy((char*)buf, (char const*)tmp3);
      for (i = 0; i < 4 - strlen((char const*)tmp2); i++)
        strcat((char*)buf, "0");
    }
    strcat((char*)buf, (char const*)tmp2);
    if (num3 || num2)
    {
      for (i = 0; i < 4 - strlen((char const*)tmp1); i++)
        strcat((char*)buf, "0");
    }
    strcat((char*)buf, (char const*)tmp1);
    if (!num3 && !num2 && !num1)
      strcpy((char*)buf, "0");
  }
  else if ( radix == 16 )
  {
    num1 = l & 0x0000FFFF;
    num2 = l >> 16;

    if (num2) _itoa(num2, tmp2, 16);
    if (num1) _itoa(num1, tmp1, 16);

    if (num2)
    {
      strcpy((char*)buf,(char const*)tmp2);
      for (i = 0; i < 4 - strlen((char const*)tmp1); i++)
        strcat((char*)buf, "0");
    }
    strcat((char*)buf, (char const*)tmp1);
    if (!num2 && !num1)
      strcpy((char*)buf, "0");
  }
  else
    return NULL;

  return buf;
#endif
}
#endif // !defined(ZBIT) && !defined(ZBIT2)

/*********************************************************************
 * @fn          osal_AddrEqual
 *
 * @brief       比较两个地址.
 *
 * @param       a1 - 第一个地址
 * @param       a2 - 第二个地址
 *
 * @return      true - 地址相等, false - 不相等
 */
byte osal_AddrEqual( zAddrType_t *a1, zAddrType_t *a2 )
{
  if ( a1->addrMode != a2->addrMode )
    return ( false );

  else if (a1->addrMode == AddrNotPresent )
    return ( true );

  else if (a1->addrMode == Addr16Bit )
    return ( a1->addr.shortAddr == a2->addr.shortAddr );

  else if ( a1->addrMode == Addr64Bit )
    return ( osal_ExtAddrEqual( a1->addr.extAddr, a2->addr.extAddr ) );

  else
    return ( false );
}

/*********************************************************************
 * @fn      osal_ExtAddrEqual()
 *
 * @brief   比较扩展地址是否相同.
 *
 * @param   extAddr1 - extended address
 * @param   extAddr2 - extended address
 *
 * @return  TRUE - 地址相同
 *          FALSE  - 不同
 */
byte osal_ExtAddrEqual( byte *extAddr1, byte *extAddr2 )
{
  byte i;

  for ( i = Z_EXTADDR_LEN; i ; i-- )
  {
    if ( *extAddr1++ != *extAddr2++ )
      return ( false );
  }

  return ( true );
}

/*********************************************************************
 * @fn        osal_rand
 *
 * @brief    随机数
 *
 * @param   none
 *
 * @return  uint16 - 新随机数
 */
uint16 osal_rand( void )
{
  return ( Onboard_rand() );
}

/*********************************************************************
 * API FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      osal_msg_allocate
 *
 * @brief
 *
 *    This function is called by a task to allocate a message buffer
 *    into which the task will encode the particular message it wishes
 *    to send.  This common buffer scheme is used to strictly limit the
 *    creation of message buffers within the system due to RAM size
 *    limitations on the microprocessor.   Note that all message buffers
 *    are a fixed size (at least initially).  The parameter len is kept
 *    in case a message pool with varying fixed message sizes is later
 *    created (for example, a pool of message buffers of size LARGE,
 *    MEDIUM and SMALL could be maintained and allocated based on request
 *    from the tasks).
 *
 *
 * @param   byte len  - wanted buffer length
 *
 *
 * @return  pointer to allocated buffer or NULL if allocation failed.
 */
byte * osal_msg_allocate( uint16 len )
{
  osal_msg_hdr_t *hdr;

  if ( len == 0 )
    return ( NULL );

  hdr = (osal_msg_hdr_t *) osal_mem_alloc( (short)(len + sizeof( osal_msg_hdr_t )) );
  if ( hdr )
  {
    hdr->next = NULL;
    hdr->len = len;
    hdr->dest_id = TASK_NO_TASK;

#if defined( OSAL_TOTAL_MEM )
    osal_msg_cnt++;
#endif
    return ( (byte *) (hdr + 1) );
  }
  else
    return ( NULL );
}

/*********************************************************************
 * @fn      osal_msg_deallocate
 *
 * @brief
 *
 *    这个函数是用来分配任务缓冲区.
 *
 *
 * @param   byte *msg_ptr - 指向一个新的数据缓冲区
 *
 * @return  ZSUCCESS, INVALID_MSG_POINTER
 */
byte osal_msg_deallocate( byte *msg_ptr )
{
  byte *x;

  if ( msg_ptr == NULL )
    return ( INVALID_MSG_POINTER );

  // don't deallocate queued buffer
  if ( OSAL_MSG_ID( msg_ptr ) != TASK_NO_TASK )
    return ( MSG_BUFFER_NOT_AVAIL );

  x = (byte *)((byte *)msg_ptr - sizeof( osal_msg_hdr_t ));

  osal_mem_free( (void *)x );

#if defined( OSAL_TOTAL_MEM )
  if ( osal_msg_cnt )
    osal_msg_cnt--;
#endif

  return ( ZSUCCESS );
}

#if defined( OSAL_TOTAL_MEM )
/*********************************************************************
 * @fn      osal_num_msgs
 *
 * @brief
 *
 *    这个函数返回一个分配信息的编号
 *
 * @param   void
 *
 * @return  UINT16 - 信息的编号
 */
UINT16 osal_num_msgs( void )
{
  return ( osal_msg_cnt );
}
#endif

/*********************************************************************
 * @fn      osal_msg_send
 *
 * @brief
 *
 *    这个函数通过调用一个任务发送一个命令信息给另一个任务或处理器.发送的任务必须查阅到任务的任务ID，这个ID将用于相应信息，
       这个函数经同样社会自一个信息准备任务在目的任务中
 *
 *
 * @param   byte destination task - 任务ID发送信息给谁
 * @param   byte *msg_ptr - 指向一个新的数据缓冲区
 * @param   byte len - 信息的数据长度
 *
 * @return  ZSUCCESS, INVALID_SENDING_TASK, INVALID_DESTINATION_TASK,
 *          INVALID_MSG_POINTER, INVALID_LEN
 */
byte osal_msg_send( byte destination_task, byte *msg_ptr )
{
  if ( msg_ptr == NULL )
    return ( INVALID_MSG_POINTER );

  if ( osalFindTask( destination_task ) == NULL )
  {
    osal_msg_deallocate( msg_ptr );
    return ( INVALID_TASK );
  }

  // 效验信息包头
  if ( OSAL_MSG_NEXT( msg_ptr ) != NULL ||
       OSAL_MSG_ID( msg_ptr ) != TASK_NO_TASK )
  {
    osal_msg_deallocate( msg_ptr );
    return ( INVALID_MSG_POINTER );
  }

  OSAL_MSG_ID( msg_ptr ) = destination_task;

  // 消息队列
  osal_msg_enqueue( &osal_qHead, msg_ptr );

  // 发送信息给任务说明一个信息在等待
  osal_set_event( destination_task, SYS_EVENT_MSG );

  return ( ZSUCCESS );
}

/*********************************************************************
 * @fn      osal_msg_receive
 *
 * @brief
 *
 *    这个函数是调用一个任务，接收命令信息。调用这个任务必须分配一个信息缓冲区后处理，这个函数用于osal_msg_deallocate()调用。
 *
 * @param   byte task_id - 接收任务ID
 *
 * @return  *byte - 如果没有信息则位空，或者报告。
 */
byte *osal_msg_receive( byte task_id )
{
  osal_msg_hdr_t *listHdr;
  osal_msg_hdr_t *prevHdr=0;
  halIntState_t   intState;

  // 拖延中断
  HAL_ENTER_CRITICAL_SECTION(intState);

  // 指向队列顶部
  listHdr = osal_qHead;

  // Look through the queue for a message that belongs to the asking task
  while ( listHdr != NULL )
  {
    if ( (listHdr - 1)->dest_id == task_id )
    {
      break;
    }
    prevHdr = listHdr;
    listHdr = OSAL_MSG_NEXT( listHdr );
  }

  // Did we find a message?
  if ( listHdr == NULL )
  {
    // Release interrupts
    HAL_EXIT_CRITICAL_SECTION(intState);
    return NULL;
  }

  // Take out of the link list
  osal_msg_extract( &osal_qHead, listHdr, prevHdr );

  // Release interrupts
  HAL_EXIT_CRITICAL_SECTION(intState);

  return ( (byte*) listHdr );
}

/*********************************************************************
 * @fn      osal_msg_enqueue
 *
 * @brief
 *
 *    This function enqueues an OSAL message into an OSAL queue.
 *
 * @param   osal_msg_q_t *q_ptr - OSAL queue
 * @param   void *msg_ptr  - OSAL message
 *
 * @return  none
 */
void osal_msg_enqueue( osal_msg_q_t *q_ptr, void *msg_ptr )
{
  void *list;
  halIntState_t intState;

  // Hold off interrupts
  HAL_ENTER_CRITICAL_SECTION(intState);

  // If first message in queue
  if ( *q_ptr == NULL )
  {
    *q_ptr = msg_ptr;
  }
  else
  {
    // Find end of queue
    for ( list = *q_ptr; OSAL_MSG_NEXT( list ) != NULL; list = OSAL_MSG_NEXT( list ) );

    // Add message to end of queue
    OSAL_MSG_NEXT( list ) = msg_ptr;
  }

  // Re-enable interrupts
  HAL_EXIT_CRITICAL_SECTION(intState);
}

/*********************************************************************
 * @fn      osal_msg_dequeue
 *
 * @brief
 *
 *    This function dequeues an OSAL message from an OSAL queue.
 *
 * @param   osal_msg_q_t *q_ptr - OSAL queue
 *
 * @return  void * - pointer to OSAL message or NULL of queue is empty.
 */
void *osal_msg_dequeue( osal_msg_q_t *q_ptr )
{
  void *msg_ptr;
  halIntState_t intState;

  // Hold off interrupts
  HAL_ENTER_CRITICAL_SECTION(intState);

  if ( *q_ptr == NULL )
  {
    HAL_EXIT_CRITICAL_SECTION(intState);
    return NULL;
  }

  // Dequeue message
  msg_ptr = *q_ptr;
  *q_ptr = OSAL_MSG_NEXT( msg_ptr );
  OSAL_MSG_NEXT( msg_ptr ) = NULL;
  OSAL_MSG_ID( msg_ptr ) = TASK_NO_TASK;

  // Re-enable interrupts
  HAL_EXIT_CRITICAL_SECTION(intState);

  return msg_ptr;
}

/*********************************************************************
 * @fn      osal_msg_push
 *
 * @brief
 *
 *    This function pushes an OSAL message to the head of an OSAL
 *    queue.
 *
 * @param   osal_msg_q_t *q_ptr - OSAL queue
 * @param   void *msg_ptr  - OSAL message
 *
 * @return  none
 */
void osal_msg_push( osal_msg_q_t *q_ptr, void *msg_ptr )
{
  halIntState_t intState;

  // Hold off interrupts
  HAL_ENTER_CRITICAL_SECTION(intState);

  if ( *q_ptr == NULL )
  {
    *q_ptr = msg_ptr;
  }
  else
  {
    // Push message to head of queue
    OSAL_MSG_NEXT( msg_ptr ) = *q_ptr;
    *q_ptr = msg_ptr;
  }

  // Re-enable interrupts
  HAL_EXIT_CRITICAL_SECTION(intState);
}

/*********************************************************************
 * @fn      osal_msg_extract
 *
 * @brief
 *
 *    This function extracts and removes an OSAL message from the
 *    middle of an OSAL queue.
 *
 * @param   osal_msg_q_t *q_ptr - OSAL queue
 * @param   void *msg_ptr  - OSAL message to be extracted
 * @param   void *prev_ptr  - OSAL message before msg_ptr in queue
 *
 * @return  none
 */
void osal_msg_extract( osal_msg_q_t *q_ptr, void *msg_ptr, void *prev_ptr )
{
  halIntState_t intState;

  // Hold off interrupts
  HAL_ENTER_CRITICAL_SECTION(intState);

  if ( msg_ptr == *q_ptr )
  {
    // remove from first
    *q_ptr = OSAL_MSG_NEXT( msg_ptr );
  }
  else
  {
    // remove from middle
    OSAL_MSG_NEXT( prev_ptr ) = OSAL_MSG_NEXT( msg_ptr );
  }
  OSAL_MSG_NEXT( msg_ptr ) = NULL;
  OSAL_MSG_ID( msg_ptr ) = TASK_NO_TASK;

  // Re-enable interrupts
  HAL_EXIT_CRITICAL_SECTION(intState);
}

/*********************************************************************
 * @fn      osal_msg_enqueue_max
 *
 * @brief
 *
 *    This function enqueues an OSAL message into an OSAL queue if
 *    the length of the queue is less than max.
 *
 * @param   osal_msg_q_t *q_ptr - OSAL queue
 * @param   void *msg_ptr  - OSAL message
 * @param   byte max - maximum length of queue
 *
 * @return  TRUE if message was enqueued, FALSE otherwise
 */
byte osal_msg_enqueue_max( osal_msg_q_t *q_ptr, void *msg_ptr, byte max )
{
  void *list;
  byte ret = FALSE;
  halIntState_t intState;

  // Hold off interrupts
  HAL_ENTER_CRITICAL_SECTION(intState);

  // If first message in queue
  if ( *q_ptr == NULL )
  {
    *q_ptr = msg_ptr;
    ret = TRUE;
  }
  else
  {
    // Find end of queue or max
    list = *q_ptr;
    max--;
    while ( (OSAL_MSG_NEXT( list ) != NULL) && (max > 0) )
    {
      list = OSAL_MSG_NEXT( list );
      max--;
    }

    // Add message to end of queue if max not reached
    if ( max != 0 )
    {
      OSAL_MSG_NEXT( list ) = msg_ptr;
      ret = TRUE;
    }
  }

  // Re-enable interrupts
  HAL_EXIT_CRITICAL_SECTION(intState);

  return ret;
}

/*********************************************************************
 * @fn      osal_set_event
 *
 * @brief
 *
 *    This function is called to set the event flags for a task.  The
 *    event passed in is OR'd into the task's event variable.
 *
 * @param   byte task_id - receiving tasks ID
 * @param   byte event_flag - what event to set
 *
 * @return  ZSUCCESS, INVALID_TASK
 */
byte osal_set_event( byte task_id, UINT16 event_flag )
{
  osalTaskRec_t *srchTask;
  halIntState_t   intState;

  srchTask = osalFindTask( task_id );
  if ( srchTask ) {
    // Hold off interrupts
    HAL_ENTER_CRITICAL_SECTION(intState);
    // Stuff the event bit(s)
    srchTask->events |= event_flag;
    // Release interrupts
    HAL_EXIT_CRITICAL_SECTION(intState);
  }
   else
    return ( INVALID_TASK );

  return ( ZSUCCESS );
}

/*********************************************************************
 * @fn      osal_isr_register
 *
 * @brief
 *
 *   This function is called to register a service routine with an
 *   interrupt. When the interrupt occurs, this service routine is called.
 *
 * @param   byte interrupt_id - Interrupt number
 * @param   void (*isr_ptr)( byte* ) - function pointer to ISR
 *
 * @return  ZSUCCESS, INVALID_INTERRUPT_ID, or INVALID_ISR_PTR
 */
byte osal_isr_register( byte interrupt_id, void (*isr_ptr)( byte* ) )
{
  return ( ZSUCCESS );
}

/*********************************************************************
 * @fn      osal_int_enable
 *
 * @brief
 *
 *   This function is called to enable an interrupt. Once enabled,
 *   occurrence of the interrupt causes the service routine associated
 *   with that interrupt to be called.
 *
 *   If INTS_ALL is the interrupt_id, interrupts (in general) are enabled.
 *   If a single interrupt is passed in, then interrupts still have
 *   to be enabled with another call to INTS_ALL.
 *
 * @param   byte interrupt_id - Interrupt number
 *
 * @return  ZSUCCESS or INVALID_INTERRUPT_ID
 */
byte osal_int_enable( byte interrupt_id )
{

  if ( interrupt_id == INTS_ALL )
  {
    HAL_ENABLE_INTERRUPTS();
  }
  else
    return ( INVALID_INTERRUPT_ID );

  return ( ZSUCCESS );
}

/*********************************************************************
 * @fn      osal_int_disable
 *
 * @brief
 *
 *   This function is called to disable an interrupt. When a disabled
 *   interrupt occurs, the service routine associated with that
 *   interrupt is not called.
 *
 *   If INTS_ALL is the interrupt_id, interrupts (in general) are disabled.
 *   If a single interrupt is passed in, then just that interrupt is disabled.
 *
 * @param   byte interrupt_id - Interrupt number
 *
 * @return  ZSUCCESS or INVALID_INTERRUPT_ID
 */
byte osal_int_disable( byte interrupt_id )
{

  if ( interrupt_id == INTS_ALL )
  {
    HAL_DISABLE_INTERRUPTS();
  }
  else
    return ( INVALID_INTERRUPT_ID );

  return ( ZSUCCESS );
}

/*********************************************************************
 * @fn      osal_init_system
 *
 * @brief
 *
 *   这个函数在人物表中初始化一个任务，并创建。T
 *
 * @param   void
 *
 * @return  ZSUCCESS
 */
byte osal_init_system( void )
{
  // 初始化内存分配系统
  osal_mem_init();

  // 初始化消息队列
  osal_qHead = NULL;

#if defined( OSAL_TOTAL_MEM )
  osal_msg_cnt = 0;
#endif

  // 初始化 timers
  osalTimerInit();

  // 初始化电源管理系统
  osal_pwrmgr_init();

  //初始化任务系统
  osalTaskInit();
  osalAddTasks();
  osalInitTasks();

  // Setup efficient search for the first free block of heap.
  osal_mem_kick();

  return ( ZSUCCESS );
}

/*********************************************************************
 * @fn      osal_start_system
 *
 * @brief
 *
 *   这个函数是任务系统的主循环.  它将浏览所有的任务事件和调用合适的任务事件
 *   如果没有事件 (包括所有的任务), 这个函数将让处理器进入睡眠模式.
 *   T这个函数没有返回值。
 *
 * @param   void
 *
 * @return  none
 */
void osal_start_system( void )
{
  uint16 events;
  uint16 retEvents;
  byte activity;
  halIntState_t intState;

  // Forever Loop
#if !defined ( ZBIT )
  for(;;)
#endif
  {

    /* This replaces MT_SerialPoll() and osal_check_timer() */
    Hal_ProcessPoll();

    activity = false;

    activeTask = osalNextActiveTask();
    if ( activeTask )
    {
      HAL_ENTER_CRITICAL_SECTION(intState);
      events = activeTask->events;
      // 清楚这个任务的事件
      activeTask->events = 0;
      HAL_EXIT_CRITICAL_SECTION(intState);

      if ( events != 0 )
      {
        // 调用任务处理的事件
        if ( activeTask->pfnEventProcessor )
        {
          retEvents = (activeTask->pfnEventProcessor)( activeTask->taskID, events );

          // 添加后面没有加工的事件到现在的任务中
          HAL_ENTER_CRITICAL_SECTION(intState);
          activeTask->events |= retEvents;
          HAL_EXIT_CRITICAL_SECTION(intState);

          activity = true;
        }
      }
    }

    // Complete pass through all task events with no activity?
    if ( activity == false )
    {
#if defined( POWER_SAVING )
      // Put the processor/system into sleep
      osal_pwrmgr_powerconserve();
#endif
    }
  }
}

/*********************************************************************
 * @fn      osal_self
 *
 * @brief
 *
 *   这个函数返回当前任务的任务ID
 *
 * @param   void
 *
 * @return  byte task ID, 0xFF bad task ID
 */
byte osal_self( void )
{
  if ( activeTask )
    return ( activeTask->taskID );
  else
    return ( TASK_NO_TASK );
}

/*********************************************************************
*********************************************************************/
