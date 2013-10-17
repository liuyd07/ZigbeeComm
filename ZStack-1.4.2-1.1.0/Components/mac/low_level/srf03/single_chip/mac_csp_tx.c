/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-03-28 18:21:19 -0700 (Wed, 28 Mar 2007) $
    Revision:       $Revision: 13888 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/



/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */

/* hal */
#include "hal_types.h"
#include "hal_mcu.h"

/* high-level */
#include "mac_spec.h"
#include "mac_pib.h"

/* exported low-level */
#include "mac_low_level.h"

/* low-level specific */
#include "mac_csp_tx.h"
#include "mac_tx.h"
#include "mac_rx.h"
#include "mac_rx_onoff.h"

/* target specific */
#include "mac_radio_defs.h"

/* debug */
#include "mac_assert.h"


/* ------------------------------------------------------------------------------------------------
 *                                   CSP Defines / Macros
 * ------------------------------------------------------------------------------------------------
 */
/* immediate strobe commands */
#define ISSTART     0xFE
#define ISSTOP      0xFF

/* strobe processor instructions */
#define SKIP(s,c)   (0x00 | (((s) & 0x07) << 4) | ((c) & 0x0F))   /* skip 's' instructions if 'c' is true  */
#define WHILE(c)    SKIP(0,c)              /* pend while 'c' is true (derived instruction)        */
#define WAITW(w)    (0x80 | ((w) & 0x1F))  /* wait for 'w' number of MAC timer overflows          */
#define WEVENT      (0xB8)                 /* wait for MAC timer compare                          */
#define WAITX       (0xBB)                 /* wait for CPSX number of MAC timer overflows         */
#define LABEL       (0xBA)                 /* set next instruction as start of loop               */
#define RPT(c)      (0xA0 | ((c) & 0x0F))  /* if condition is true jump to last label             */
#define INT         (0xB9)                 /* assert IRQ_CSP_INT interrupt                        */
#define INCY        (0xBD)                 /* increment CSPY                                      */
#define INCMAXY(m)  (0xB0 | ((m) & 0x07))  /* increment CSPY but not above maximum value of 'm'   */
#define DECY        (0xBE)                 /* decrement CSPY                                      */
#define DECZ        (0xBF)                 /* decrement CSPZ                                      */
#define RANDXY      (0xBC)                 /* load the lower CSPY bits of CSPX with random value  */

/* strobe processor command instructions */
#define SSTOP       (0xDF)    /* stop program execution                                      */
#define SNOP        (0xC0)    /* no operation                                                */
#define STXCALN     (0xC1)    /* enable and calibrate frequency synthesizer for TX           */
#define SRXON       (0xC2)    /* turn on receiver                                            */
#define STXON       (0xC3)    /* transmit after calibration                                  */
#define STXONCCA    (0xC4)    /* transmit after calibration if CCA indicates clear channel   */
#define SRFOFF      (0xC5)    /* turn off RX/TX                                              */
#define SFLUSHRX    (0xC6)    /* flush receive FIFO                                          */
#define SFLUSHTX    (0xC7)    /* flush transmit FIFO                                         */
#define SACK        (0xC8)    /* send ACK frame                                              */
#define SACKPEND    (0xC9)    /* send ACK frame with pending bit set                         */

/* conditions for use with instructions SKIP and RPT */
#define C_CCA_IS_VALID        0x00
#define C_SFD_IS_ACTIVE       0x01
#define C_CPU_CTRL_IS_ON      0x02
#define C_END_INSTR_MEM       0x03
#define C_CSPX_IS_ZERO        0x04
#define C_CSPY_IS_ZERO        0x05
#define C_CSPZ_IS_ZERO        0x06

/* negated conditions for use with instructions SKIP and RPT */
#define C_NEGATE(c)   ((c) | 0x08)
#define C_CCA_IS_INVALID      C_NEGATE(C_CCA_IS_VALID)
#define C_SFD_IS_INACTIVE     C_NEGATE(C_SFD_IS_ACTIVE)
#define C_CPU_CTRL_IS_OFF     C_NEGATE(C_CPU_CTRL_IS_ON)
#define C_NOT_END_INSTR_MEM   C_NEGATE(C_END_INSTR_MEM)
#define C_CSPX_IS_NON_ZERO    C_NEGATE(C_CSPX_IS_ZERO)
#define C_CSPY_IS_NON_ZERO    C_NEGATE(C_CSPY_IS_ZERO)
#define C_CSPZ_IS_NON_ZERO    C_NEGATE(C_CSPZ_IS_ZERO)

///////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : part of a workaround to help ameliorate chip bug #273.
//  This bug could actually be the source of extant every-once-in-a-while problems.
//  When Rev B is obsolete, delete these defines and all references to them.
//  Compile errors should flag all related fixes.
#define __SNOP_SKIP__     1
#define __SNOP__          SNOP
///////////////////////////////////////////////////////////////////////////////////////


/* ------------------------------------------------------------------------------------------------
 *                                         Defines
 * ------------------------------------------------------------------------------------------------
 */

/* CSPY return values from CSP program */
#define CSPY_RXTX_COLLISION         0
#define CSPY_NO_RXTX_COLLISION      1

/* CSPZ return values from CSP program */
#define CSPZ_CODE_TX_DONE           0
#define CSPZ_CODE_CHANNEL_BUSY      1
#define CSPZ_CODE_TX_ACK_TIME_OUT   2


/* ------------------------------------------------------------------------------------------------
 *                                          Macros
 * ------------------------------------------------------------------------------------------------
 */
#define CSP_STOP_AND_CLEAR_PROGRAM()          st( RFST = ISSTOP;  )
#define CSP_START_PROGRAM()                   st( RFST = ISSTART; )
////////////////////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : workaround for chip bug #574, delete this whole mess when Rev B is obsolete.
//  Compile errors will flag all instances of macro call.  Delete those too.
#ifndef _REMOVE_REV_B_WORKAROUNDS
#define __FIX_T2CMP_BUG__()   st( if (T2CMP == 0) { T2CMP = 1;} )
#else
#define __FIX_T2CMP_BUG__()
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *  These macros improve readability of using T2CMP in conjunction with WEVENT.
 *
 *  The timer2 compare, T2CMP, only compares one byte of the 16-bit timer register.
 *  It is configurable and has been set to compare against the upper byte of the timer value.
 *  The CSP instruction WEVENT waits for the timer value to be greater than or equal
 *  the value of T2CMP.
 *
 *  Reading the timer value is done by reading the low byte first.  This latches the
 *  high byte.  A trick with the ternary operator is used by a macro below to force a
 *  read of the low byte when returning the value of the high byte.
 *
 *  CSP_WEVENT_SET_TRIGGER_NOW()      - sets the WEVENT trigger point at the current timer count
 *  CSP_WEVENT_SET_TRIGGER_SYMBOLS(x) - sets the WEVENT trigger point in symbols
 *  CSP_WEVENT_READ_COUNT_SYMBOLS()   - reads the current timer count in symbols
 */
#define T2THD_TICKS_PER_SYMBOL                (MAC_RADIO_TIMER_TICKS_PER_SYMBOL() >> 8)

#define CSP_WEVENT_SET_TRIGGER_NOW()          st( uint8 temp=T2TLD;  T2CMP = T2THD;  __FIX_T2CMP_BUG__(); )
#define CSP_WEVENT_SET_TRIGGER_SYMBOLS(x)     st( MAC_ASSERT(x <= MAC_A_UNIT_BACKOFF_PERIOD); \
                                                  T2CMP = (x) * T2THD_TICKS_PER_SYMBOL; \
                                                  __FIX_T2CMP_BUG__(); )
#define CSP_WEVENT_READ_COUNT_SYMBOLS()       (((T2TLD) ? T2THD : T2THD) / T2THD_TICKS_PER_SYMBOL)

/*
 *  Number of bits used for aligning a slotted transmit to the backoff count (plus
 *  derived values).  There are restrictions on this value.  Compile time integrity
 *  checks will catch an illegal setting of this value.  A full explanation accompanies
 *  this compile time check (see bottom of this file).
 */
#define SLOTTED_TX_MAX_BACKOFF_COUNTDOWN_NUM_BITS     4
#define SLOTTED_TX_MAX_BACKOFF_COUNTDOWN              (1 << SLOTTED_TX_MAX_BACKOFF_COUNTDOWN_NUM_BITS)
#define SLOTTED_TX_BACKOFF_COUNT_ALIGN_BIT_MASK       (SLOTTED_TX_MAX_BACKOFF_COUNTDOWN - 1)



/* ------------------------------------------------------------------------------------------------
 *                                     Local Programs
 * ------------------------------------------------------------------------------------------------
 */
static void cspPrepForTxProgram(void);


/**************************************************************************************************
 * @fn          macCspTxReset
 *
 * @brief       Reset the CSP.  Immediately halts any running program.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxReset(void)
{
  MAC_MCU_CSP_STOP_DISABLE_INTERRUPT();
  MAC_MCU_CSP_INT_DISABLE_INTERRUPT();
  CSP_STOP_AND_CLEAR_PROGRAM();
}


/*=================================================================================================
 * @fn          cspPrepForTxProgram
 *
 * @brief       Prepare and initialize for transmit CSP program.
 *              Call *before* loading the CSP program!
 *
 * @param       none
 *
 * @return      none
 *=================================================================================================
 */
static void cspPrepForTxProgram(void)
{
  MAC_ASSERT(!(RFIM & IM_CSP_STOP)); /* already an active CSP program */

  /* set up parameters for CSP transmit program */
  CSPY = CSPY_NO_RXTX_COLLISION;
  CSPZ = CSPZ_CODE_CHANNEL_BUSY;

  /* clear the currently loaded CSP, this generates a stop interrupt which must be cleared */
  CSP_STOP_AND_CLEAR_PROGRAM();
  MAC_MCU_CSP_STOP_CLEAR_INTERRUPT();
  MAC_MCU_CSP_INT_CLEAR_INTERRUPT();
}


/**************************************************************************************************
 * @fn          macCspTxPrepCsmaUnslotted
 *
 * @brief       Prepare CSP for "Unslotted CSMA" transmit.  Load CSP program and set CSP parameters.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxPrepCsmaUnslotted(void)
{
  cspPrepForTxProgram();

  /*----------------------------------------------------------------------
   *  Load CSP program :  Unslotted CSMA transmit 
   */

  /*
   *  Wait for X number of backoffs, then wait for intra-backoff count
   *  to reach value set for WEVENT.
   */
  RFST = WAITX;
  RFST = WEVENT;

  /* wait for one backoff to guarantee receiver has been on at least that long */
  RFST = WAITW(1);
  RFST = WEVENT;
  
  /* sample CCA, if it fails exit from here, CSPZ indicates result */
  RFST = SKIP(1+__SNOP_SKIP__, C_CCA_IS_VALID);
  RFST = SSTOP;
  RFST = __SNOP__;
  
  /* CSMA has passed so transmit */
  RFST = STXON;

  /*
   *  If the SFD pin is high at this point, there was an RX-TX collision.
   *  In other words, TXON was strobed while receiving.  The CSP variable
   *  CSPY is decremented to indicate this happened.  The rest of the transmit
   *  continues normally.
   */
  RFST = SKIP(2+__SNOP_SKIP__, C_SFD_IS_INACTIVE);
  RFST = WHILE(C_SFD_IS_ACTIVE);
  RFST = DECY;
  RFST = __SNOP__;

  /*
   *  Watch the SFD pin to determine when the transmit has finished going out.
   *  The INT instruction causes an interrupt to fire.  The ISR for this interrupt
   *  handles the record the timestamp (which was just captured when SFD went high).
   *  Decrement CSPZ at the last step to indicate transmit was successful.
   */
  RFST = WHILE(C_SFD_IS_INACTIVE);
  RFST = INT;
  RFST = WHILE(C_SFD_IS_ACTIVE);
  RFST = DECZ;
}


/**************************************************************************************************
 * @fn          macCspTxPrepCsmaSlotted
 *
 * @brief       Prepare CSP for "Slotted CSMA" transmit.  Load CSP program and set CSP parameters.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxPrepCsmaSlotted(void)
{
  cspPrepForTxProgram();

  /*----------------------------------------------------------------------
   *  Load CSP program :  Slotted CSMA transmit 
   */
  
  /* wait for X number of backoffs */
  RFST = WAITX;

  /* wait for one backoff to guarantee receiver has been on at least that long */
  RFST = WAITW(1);

  /* sample CCA, if it fails exit from here, CSPZ indicates result */
  RFST = SKIP(1+__SNOP_SKIP__, C_CCA_IS_VALID);
  RFST = SSTOP;
  RFST = __SNOP__;
  
  /* per CSMA in specification, wait one backoff */
  RFST = WAITW(1);
  
  /* sample CCA again, if it fails exit from here, CSPZ indicates result */
  RFST = SKIP(1+__SNOP_SKIP__, C_CCA_IS_VALID);
  RFST = SSTOP;
  RFST = __SNOP__;
  
  /* CSMA has passed so transmit */
  RFST = STXON;

  /*
   *  If the SFD pin is high at this point, there was an RX-TX collision.
   *  In other words, TXON was strobed while receiving.  The CSP variable
   *  CSPY is decremented to indicate this happened.  The rest of the transmit
   *  continues normally.
   */
  RFST = SKIP(2+__SNOP_SKIP__, C_SFD_IS_INACTIVE);
  RFST = WHILE(C_SFD_IS_ACTIVE);
  RFST = DECY;
  RFST = __SNOP__;

  /*
   *  Watch the SFD pin to determine when the transmit has finished going out.
   *  The INT instruction causes an interrupt to fire.  The ISR for this interrupt
   *  handles the record the timestamp (which was just captured when SFD went high).
   *  Decrement CSPZ at the last step to indicate transmit was successful.
   */
  RFST = WHILE(C_SFD_IS_INACTIVE);
  RFST = INT;
  RFST = WHILE(C_SFD_IS_ACTIVE);
  RFST = DECZ;
}


/**************************************************************************************************
 * @fn          macCspTxGoCsma
 *
 * @brief       Run previously loaded CSP program for CSMA transmit.  Handles either
 *              slotted or unslotted CSMA transmits.  When CSP program has finished,
 *              an interrupt occurs and macCspTxStopIsr() is called.  This ISR will in
 *              turn call macTxDoneCallback().
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxGoCsma(void)
{
  /*
   *  Set CSPX with the countdown time of the CSMA delay.  Subtract one because there is
   *  a built-in one backoff delay in the CSP program to make sure receiver has been 'on'
   *  for at least one backoff.  Don't subtract though if CSPX is already zero!
   */
  CSPX = macTxCsmaBackoffDelay;
  if (CSPX != 0)
  {
    CSPX--;
  }

  /*
   *  Set WEVENT to trigger at the current value of the timer.  This allows
   *  unslotted CSMA to transmit just a little bit sooner.
   */
  CSP_WEVENT_SET_TRIGGER_NOW();

  /*
   *  Enable interrupt that fires when CSP program stops.
   *  Also enable interrupt that fires when INT instruction
   *  is executed.
   */
  MAC_MCU_CSP_STOP_ENABLE_INTERRUPT();
  MAC_MCU_CSP_INT_ENABLE_INTERRUPT();
  
  /*
   *  Turn on the receiver if it is not already on.  Receiver must be 'on' for at
   *  least one backoff before performing clear channel assessment (CCA).
   */
  macRxOn();

  /* start the CSP program */
  CSP_START_PROGRAM();
}


/**************************************************************************************************
 * @fn          macCspTxPrepSlotted
 *
 * @brief       Prepare CSP for "Slotted" (non-CSMA) transmit.
 *              Load CSP program and set CSP parameters.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxPrepSlotted(void)
{
  cspPrepForTxProgram();

  /*----------------------------------------------------------------------
   *  Load CSP program :  Slotted transmit (no CSMA)
   */
  
  /* wait for X number of backoffs */
  RFST = WAITX;

  /* just transmit, no CSMA required */
  RFST = STXON;

  /*
   *  If the SFD pin is high at this point, there was an RX-TX collision.
   *  In other words, TXON was strobed while receiving.  The CSP variable
   *  CSPY is decremented to indicate this happened.  The rest of the transmit
   *  continues normally.
   */
  RFST = SKIP(2+__SNOP_SKIP__, C_SFD_IS_INACTIVE);
  RFST = WHILE(C_SFD_IS_ACTIVE);
  RFST = DECY;
  RFST = __SNOP__;
  
  /*
   *  Watch the SFD pin to determine when the transmit has finished going out.
   *  The INT instruction causes an interrupt to fire.  The ISR for this interrupt
   *  handles the record the timestamp (which was just captured when SFD went high).
   *  Decrement CSPZ at the last step to indicate transmit was successful.
   */
  RFST = WHILE(C_SFD_IS_INACTIVE);
  RFST = INT;
  RFST = WHILE(C_SFD_IS_ACTIVE);
  RFST = DECZ;
}


/**************************************************************************************************
 * @fn          macCspTxGoSlotted
 *
 * @brief       Run previously loaded CSP program for non-CSMA slotted transmit.   When CSP
 *              program has finished, an interrupt occurs and macCspTxStopIsr() is called.
 *              This ISR will in turn call macTxDoneCallback().
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxGoSlotted(void)
{
  halIntState_t  s;
  uint8 lowByteOfBackoffCount;
  uint8 backoffCountdown;
  
  /*
   *  Enable interrupt that fires when CSP program stops.
   *  Also enable interrupt that fires when INT instruction
   *  is executed.
   */
  MAC_MCU_CSP_STOP_ENABLE_INTERRUPT();
  MAC_MCU_CSP_INT_ENABLE_INTERRUPT();

  /* critical section needed for timer accesses */
  HAL_ENTER_CRITICAL_SECTION(s);

  /* store lowest byte of backoff count (same as lowest byte of overflow count) */
  lowByteOfBackoffCount = T2OF0;

  /*
   *  Compute the number of backoffs until time to strobe transmit.  The strobe should
   *  occur one backoff before the SFD pin is expected to go high.  So, the forumla for the
   *  countdown value is to determine when the lower bits would rollover and become zero,
   *  and then subtract one.
   */
  backoffCountdown = SLOTTED_TX_MAX_BACKOFF_COUNTDOWN - (lowByteOfBackoffCount & SLOTTED_TX_BACKOFF_COUNT_ALIGN_BIT_MASK) - 1;
  
  /*
   *  Store backoff countdown value into CSPX.
   *
   *  Note: it is OK if this value is zero.  The WAITX instruction at the top of the
   *  CSP program will immediately continue if CSPX is zero when executed.  However,
   *  if the countdown is zero, it means the transmit function was not called early
   *  enough for a properly timed slotted transmit.  The transmit will be late.
   */
  CSPX = backoffCountdown;

  /*
   *  The receiver will be turned on during CSP execution, guaranteed.
   *  Since it is not possible to update C variables within the CSP,
   *  the new "on" state of the receiver must be set a little early
   *  here before the CSP is started.
   */
  MAC_RX_WAS_FORCED_ON();

  /* start the CSP program */
  CSP_START_PROGRAM();
  
  /*
   *  If the previous stored low byte of the backoff count is no longer equal to
   *  the current value, a rollover has occurred.  This means the backoff countdown
   *  stored in CSPX may not be correct.
   *
   *  In this case, the value of CSPX is reloaded to reflect the correct backoff
   *  countdown value (this is one less than what was just used as a rollover has
   *  occurred).  Since it is certain a rollover *just* occurred, there is no danger
   *  of another rollover occurring.  This means the value written to CSPX is guaranteed
   *  to be accurate.
   *
   *  Also, the logic below ensures that the value written to CSPX is at least one.
   *  This is needed for correct operation of the WAITX instruction.  As with an
   *  initial backoff countdown value of zero, if this case does occur, it means the
   *  transmit function was not called early enough for a properly timed slotted transmit.
   *  The transmit will be late.
   *
   *  Finally, worth noting, writes to CSPX may not work if the CSP is executing the WAITX
   *  instruction and a timer rollover occurs.  In this case, however, there is no possibility
   *  of that happening.  If CSPX is updated here, a rollover has just occurred so a
   *  collision is not possible (still within a critical section here too).
   */
  if ((lowByteOfBackoffCount != T2OF0) && (backoffCountdown > 1))
  {
    CSPX = backoffCountdown - 1;
  }
  
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macCspForceTxDoneIfPending
 *
 * @brief       The function clears out any pending TX done logic.  Used by receive logic
 *              to make sure its ISR does not prevent transmit from completing in a reasonable
 *              amount of time.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspForceTxDoneIfPending(void)
{
///////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : work workaround for chip bug #273.  The instruction DECZ might
//  be incorrectly executed twice, resulting an illegal value for CSPZ.
//  Delete when Rev B is obsolete.
///////////////////////////////////////////////////////////////////////////////////////
#ifndef _REMOVE_REV_B_WORKAROUNDS
  if (CSPZ >= 0xF8) { CSPZ = 0; }
#endif
///////////////////////////////////////////////////////////////////////////////////////
  
  if ((CSPZ == CSPZ_CODE_TX_DONE) &&  MAC_MCU_CSP_STOP_INTERRUPT_IS_ENABLED())
  {
    MAC_MCU_CSP_STOP_DISABLE_INTERRUPT();
    if (MAC_MCU_CSP_INT_INTERRUPT_IS_ENABLED())
    {
      macCspTxIntIsr();
    }
    macTxDoneCallback();
  }
}


/**************************************************************************************************
 * @fn          macCspTxRequestAckTimeoutCallback
 *
 * @brief       Requests a callback after the ACK timeout period has expired.  At that point,
 *              the function macTxAckTimeoutCallback() is called via an interrupt.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxRequestAckTimeoutCallback(void)
{
  uint8 startSymbol;
  uint8 symbols;
  uint8 rollovers;

  MAC_ASSERT(!(RFIM & IM_CSP_STOP)); /* already an active CSP program */

  /* record current symbol count */
  startSymbol = CSP_WEVENT_READ_COUNT_SYMBOLS();

  /* set symbol timeout from PIB */
  symbols = macPib.ackWaitDuration;

  /* make sure delay value is not too small for logic to handle */
  MAC_ASSERT(symbols > MAC_A_UNIT_BACKOFF_PERIOD);  /* symbols timeout period must be great than a backoff */

  /* subtract out symbols left in current backoff period */
  symbols = symbols - (MAC_A_UNIT_BACKOFF_PERIOD - startSymbol);

  /* calculate rollovers needed for remaining symbols */
  rollovers = symbols / MAC_A_UNIT_BACKOFF_PERIOD;

  /* calculate symbols that still need counted after last rollover */
  symbols = symbols - (rollovers * MAC_A_UNIT_BACKOFF_PERIOD);

  /* add one to rollovers to account for symbols remaining in the current backoff period */
  rollovers++;

  /* set up parameters for CSP program */
  CSPZ = CSPZ_CODE_TX_ACK_TIME_OUT;
  CSPX = rollovers;
  CSP_WEVENT_SET_TRIGGER_SYMBOLS(symbols);

  /* clear the currently loaded CSP, this generates a stop interrupt which must be cleared */
  CSP_STOP_AND_CLEAR_PROGRAM();
  MAC_MCU_CSP_STOP_CLEAR_INTERRUPT();
  
  /*--------------------------
   * load CSP program
   */
  RFST = WAITX;
  RFST = WEVENT;
  /*--------------------------
   */
  
  /* run CSP program */
  MAC_MCU_CSP_STOP_ENABLE_INTERRUPT();
  CSP_START_PROGRAM();

  /*
   *  For bullet proof operation, must account for the boundary condition
   *  where a rollover occurs after count was read but before CSP program
   *  was started.
   *
   *  If current symbol count is less that the symbol count recorded at the
   *  start of this function, a rollover has occurred.
   */
  if (CSP_WEVENT_READ_COUNT_SYMBOLS() < startSymbol)
  {
    /* a rollover has occurred, make sure it was accounted for */
    if (CSPX == rollovers)
    {
      /*
       *  Rollover event missed, manually decrement CSPX to adjust.
       *
       *  Note : there is a very small chance that CSPX does not
       *  get decremented.  This would occur if CSPX were written
       *  at exactly the same time a timer overflow is occurring (which
       *  causes the CSP instruction WAITX to decrement CSPX).  This
       *  would be extremely rare, but if it does happen, the only
       *  consequence is that the ACK timeout period is extended
       *  by one backoff.
       */
      CSPX--;
    }
  }
}


/**************************************************************************************************
 * @fn          macCspTxCancelAckTimeoutCallback
 *
 * @brief       Cancels previous request for ACK timeout callback.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxCancelAckTimeoutCallback(void)
{
  MAC_MCU_CSP_STOP_DISABLE_INTERRUPT();
  CSP_STOP_AND_CLEAR_PROGRAM();
}


/**************************************************************************************************
 * @fn          macCspTxIntIsr
 *
 * @brief       Interrupt service routine for handling INT type interrupts from CSP.
 *              This interrupt happens when the CSP instruction INT is executed.  It occurs
 *              once the SFD signal goes high indicating that transmit has successfully
 *              started.  The timer value has been captured at this point and timestamp
 *              can be stored.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxIntIsr(void)
{
///////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : work workaround for chip bug #273.  The instruction DECY might
//  be incorrectly executed twice, resulting an illegal value for CSPZ.
//  Delete when Rev B is obsolete.
///////////////////////////////////////////////////////////////////////////////////////
#ifndef _REMOVE_REV_B_WORKAROUNDS
  if (CSPY >= 0xF8) { CSPY = 0; }
#endif
///////////////////////////////////////////////////////////////////////////////////////

  MAC_MCU_CSP_INT_DISABLE_INTERRUPT();

  if (CSPY == CSPY_RXTX_COLLISION)
  {
    macRxHaltCleanup();
  }

  /* execute callback function that records transmit timestamp */
  macTxTimestampCallback();
}


/**************************************************************************************************
 * @fn          macCspTxStopIsr
 *
 * @brief       Interrupt service routine for handling STOP type interrupts from CSP.
 *              This interrupt occurs when the CSP program stops by 1) reaching the end of the
 *              program, 2) executing SSTOP within the program, 3) executing immediate
 *              instruction ISSTOP.
 *
 *              The value of CSPZ indicates if interrupt is being used for ACK timeout or
 *              is the end of a transmit.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macCspTxStopIsr(void)
{
  MAC_MCU_CSP_STOP_DISABLE_INTERRUPT();

///////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : work workaround for chip bug #273.  The instruction DECZ might
//  be incorrectly executed twice, resulting an illegal value for CSPZ.
//  Delete when Rev B is obsolete.
///////////////////////////////////////////////////////////////////////////////////////
#ifndef _REMOVE_REV_B_WORKAROUNDS
  if (CSPZ >= 0xF8) { CSPZ = 0; }
#endif
///////////////////////////////////////////////////////////////////////////////////////

  if (CSPZ == CSPZ_CODE_TX_DONE)
  {
    macTxDoneCallback();
  }
  else if (CSPZ == CSPZ_CODE_CHANNEL_BUSY)
  {
    macTxChannelBusyCallback();
  }
  else
  {
    MAC_ASSERT(CSPZ == CSPZ_CODE_TX_ACK_TIME_OUT); /* unexpected CSPZ value */
    macTxAckNotReceivedCallback();
  }
}



/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */
#if ((CSPY_RXTX_COLLISION != 0) || (CSPY_NO_RXTX_COLLISION != 1))
#error "ERROR!  The CSPY return values are very specific and tied into the actual CSP program."
#endif

#if ((CSPZ_CODE_TX_DONE != 0) || (CSPZ_CODE_CHANNEL_BUSY != 1))
#error "ERROR!  The CSPZ return values are very specific and tied into the actual CSP program."
#endif

#if (MAC_TX_TYPE_SLOTTED_CSMA != 0)
#error "WARNING!  This define value changed.  It was selected for optimum performance."
#endif

#if (T2THD_TICKS_PER_SYMBOL == 0)
#error "ERROR!  Timer compare will not work on high byte.  Clock speed is probably too slow."
#endif

#define BACKOFFS_PER_BASE_SUPERFRAME  (MAC_A_BASE_SLOT_DURATION * MAC_A_NUM_SUPERFRAME_SLOTS)
#if (((BACKOFFS_PER_BASE_SUPERFRAME - 1) & SLOTTED_TX_BACKOFF_COUNT_ALIGN_BIT_MASK) != SLOTTED_TX_BACKOFF_COUNT_ALIGN_BIT_MASK)
#error "ERROR!  The specified bit mask for backoff alignment of slotted transmit does not rollover 'cleanly'."
/*
 *  In other words, the backoff count for the number of superframe rolls over before the
 *  specified number of bits rollover.  For example, if backoff count for a superframe
 *  rolls over at 48, the binary number immediately before a rollover is 00101111.
 *  In this case four bits would work as an alignment mask.  Five would not work though as
 *  the lower five bits would go from 01111 to 00000 (instead of the value 10000 which
 *  would be expected) because it a new superframe is starting.
 */
#endif
#if (SLOTTED_TX_MAX_BACKOFF_COUNTDOWN_NUM_BITS < 2)
#error "ERROR!  Not enough backoff countdown bits to be practical."
#endif


/**************************************************************************************************
*/
