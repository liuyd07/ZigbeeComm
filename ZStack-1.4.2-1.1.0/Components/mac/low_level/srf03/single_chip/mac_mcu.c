/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-05-15 15:37:47 -0700 (Tue, 15 May 2007) $
    Revision:       $Revision: 14304 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

   ///////////////////////////////////////////////////////////////////////////////////
   //  REV_B_WORKAROUND : Place holder as a reminder to optimize the bank select code
   //  once Rev B obsoleted.  Currently the bank select bits share a register with
   //  other functionality.  This will be *mirrored* to a new location where it
   //  is the only thing in the register.
   ///////////////////////////////////////////////////////////////////////////////////


/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

/* hal */
#include "hal_defs.h"
#include "hal_mcu.h"

/* low-level specific */
#include "mac_rx.h"
#include "mac_tx.h"
#include "mac_backoff_timer.h"
#include "mac_csp_tx.h"
#include "mac_rx_onoff.h"

/* target specific */
#include "mac_mcu.h"
#include "mac_radio_defs.h"

/* debug */
#include "mac_assert.h"


/* ------------------------------------------------------------------------------------------------
 *                                           Defines
 * ------------------------------------------------------------------------------------------------
 */
#define T2CNF_BASE_VALUE    (RUN | SYNC)

/* for optimized indexing of uint32's */
#if HAL_MCU_LITTLE_ENDIAN()
#define UINT32_NDX0   0
#define UINT32_NDX1   1
#define UINT32_NDX2   2
#define UINT32_NDX3   3
#else
#define UINT32_NDX0   3
#define UINT32_NDX1   2
#define UINT32_NDX2   1
#define UINT32_NDX3   0
#endif

/* 32 kHz clock source select in CLKCON */
#if !defined (OSC32K_CRYSTAL_INSTALLED) || (defined (OSC32K_CRYSTAL_INSTALLED) && (OSC32K_CRYSTAL_INSTALLED == TRUE))
#define OSC_32KHZ  0x00 /* external 32 KHz xosc */
#else
#define OSC_32KHZ  0x80 /* internal 32 KHz rcosc */
#endif


/* ------------------------------------------------------------------------------------------------
 *                                        Local Variables
 * ------------------------------------------------------------------------------------------------
 */
static int8 maxRssi;

/*
 *  A shadow variable is required for the lower four bits of T2PEROF2.  These bits have different
 *  functionality for read versus write.  This would fine except the upper bits of T2PEROF2 are
 *  used to enable and disable interrupts and must be set independently of the lower four bits.
 *  Since the original value of the lower four bits is lost (it reads as something else) these
 *  bits must be stored in a shadow variable.  This shadow variable is declared here and is
 *  used where needed in this module.
 */
static uint8 shadowPerof2;


/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
static void mcuRecordMaxRssiIsr(void);


/**************************************************************************************************
 * @fn          macMcuInit
 *
 * @brief       Initialize the MCU.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuInit(void)
{
  uint16 i;

  MAC_ASSERT(CHVER != 0x00); /* chip version A is obsolete */
///////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : replace with line below when Rev B is obsoleted
#ifndef _REMOVE_REV_B_WORKAROUNDS
#else
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//  keep this code, delete the rest
  MAC_ASSERT(CHVER != 0x01); /* chip version B is obsolete */
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif
///////////////////////////////////////////////////////////////////////////////////
 // MAC_ASSERT(CHVER != 0x02); /* Ð¾Æ¬°æ±¾Ñ¡Ôñ*/

///////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : workaround for chip bug #51, remove when fixed
#ifndef _REMOVE_REV_B_WORKAROUNDS
  FSCTRLH = 0x41;
  FSTST1L = 0x32;
  FSTST2H = 0x20;
#endif
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : workaround for chip bug #267, remove when fixed
#ifndef _REMOVE_REV_B_WORKAROUNDS
  MDMCTRL0L |= AUTOACK;  /* enable autoack */
#endif
///////////////////////////////////////////////////////////////////////////////////

  /* tuning adjustments for optimal radio performance; details available in datasheet */
  RXCTRL0H = 0x32;
  RXCTRL0L = 0xF5;

///////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : workaround for chip bug #267, update when fixed
#ifndef _REMOVE_REV_B_WORKAROUNDS
  /* do nothing */
#else
// this part can go away once new .h file is shipping
#ifndef IRQSRC
#define IRQSRC    XREG( 0xDF64 )
#endif
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//  keep this code, delete the rest
  /* enable TX_DONE interrupts for ACK transmits */
  IRQSRC = TXACK;
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif
///////////////////////////////////////////////////////////////////////////////////

  /* disable the CSPT register compare function */
  CSPT = 0xFF;

  /* enable general RF interrupts */
  IEN2 |= RFIE;

  /* intialize shadow register */
  shadowPerof2 = 0;

  /* set RF interrupts one notch above lowest priority (four levels available) */
  IP0 |=  IP_RFERR_RF_DMA_BV;
  IP1 &= ~IP_RFERR_RF_DMA_BV;

  /*-------------------------------------------------------------------------------
   *  Initialize MAC timer.
   */

  /* set timer rollover */
  T2CAPLPL = MAC_RADIO_TIMER_TICKS_PER_BACKOFF() & 0xFF;
  T2CAPHPH = MAC_RADIO_TIMER_TICKS_PER_BACKOFF() >> 8;

  /* start timer */
  T2CNF |= RUN;

  /* enable timer interrupts */
  T2IE = 1;

  /* configure clock to use XOSC */
  SLEEP &= ~OSC_PD;                       /* turn on 16MHz RC and 32MHz XOSC */
  while (!(SLEEP & XOSC_STB));            /* wait for 32MHz XOSC stable */
  asm("NOP");                             /* chip bug workaround */
  for (i=0; i<504; i++) asm("NOP");       /* Require 63us delay for Rev B */
  CLKCON = (0x00 | OSC_32KHZ);            /* 32MHz XOSC */
  while (CLKCON != (0x00 | OSC_32KHZ));
  SLEEP |= OSC_PD;                        /* turn off 16MHz RC */


 /*----------------------------------------------------------------------------------------------
  *  Initialize random seed value.
  */

  /* turn on radio power */
  RFPWR &= ~RREG_RADIO_PD;
  while((RFPWR & ADI_RADIO_PD));

  /*
   *  Set radio for infinite reception.  Once radio reaches this state,
   *  it will stay in receive mode regardless RF activity.
   */
  MDMCTRL1L = MDMCTRL1L_RESET_VALUE | RX_MODE_INFINITE_RECEPTION;

  /* turn on the receiver */
  macRxOn();

  /*
   *  Wait for radio to reach infinite reception state.  Once it does,
   *  The least significant bit of ADTSTH should be pretty random.
   */
  while (FSMSTATE != FSM_FFCTRL_STATE_RX_INF)

  /* put 16 random bits into the seed value */
  {
    uint16 rndSeed;
    uint8  i;

    rndSeed = 0;

    for(i=0; i<16; i++)
    {
      /* use most random bit of analog to digital receive conversion to populate the random seed */
      rndSeed = (rndSeed << 1) | (ADCTSTH & 0x01);
    }

    /*
     *  The seed value must not be zero.  If it is, the psuedo random sequence will be always be zero.
     *  There is an extremely small chance this seed could randomly be zero (more likely some type of
     *  hardware problem would cause this).  The following check makes sure this does not happen.
     */
    if (rndSeed == 0x0000)
    {
      rndSeed = 0xBEEF; /* completely arbitrary "random" value */
    }

    /*
     *  Two writes to RNDL will set the random seed.  A write to RNDL copies current contents
     *  of RNDL to RNDH before writing new the value to RNDL.
     */
    RNDL = rndSeed & 0xFF;
    RNDL = rndSeed >> 8;
  }

  /* turn off the receiver */
  macRxOff();

  /* take receiver out of infinite reception mode; set back to normal operation */
  MDMCTRL1L = MDMCTRL1L_RESET_VALUE | RX_MODE_NORMAL_OPERATION;

  /* turn radio back off */
  RFPWR |= RREG_RADIO_PD;
}


/**************************************************************************************************
 * @fn          macMcuRandomByte
 *
 * @brief       Returns a random byte using a special hardware feature that generates new
 *              random values based on the truly random seed set earlier.
 *
 * @param       none
 *
 * @return      a random byte
 **************************************************************************************************
 */
uint8 macMcuRandomByte(void)
{
  /* clock the random generator to get a new random value */
  ADCCON1 = (ADCCON1 & ~RCTRL_BITS) | RCTRL_CLOCK_LFSR;

  /* return new randomized value from hardware */
  return(RNDH);
}


/**************************************************************************************************
 * @fn          macMcuTimerCount
 *
 * @brief       Returns the upper eight bits of hardware timer count.  The full 16-bit timer
 *              count is not returned because the timer compare feature only compares one byte.
 *
 * @param       none
 *
 * @return      upper eight bits of hardware timer count
 **************************************************************************************************
 */
uint8 macMcuTimerCount(void)
{
  uint8 volatile temp;

  /* reading T2TLD latches T2THD */
  temp = T2TLD;
  return(T2THD);
}


/**************************************************************************************************
 * @fn          macMcuTimerCapture
 *
 * @brief       Returns the last timer capture.  This capture should have occurred at the
 *              receive time of the last frame (the last time SFD transitioned to active).
 *
 * @param       none
 *
 * @return      last capture of hardware timer (full 16-bit value)
 **************************************************************************************************
 */
uint16 macMcuTimerCapture(void)
{
  uint16 timerCapture;

  timerCapture = T2CAPLPL;
  timerCapture |= T2CAPHPH << 8;

  return (timerCapture);
}


/**************************************************************************************************
 * @fn          macMcuOverflowCount
 *
 * @brief       Returns the value of the overflow counter which is a special hardware feature.
 *              The overflow count actually is only 20 bits of information.
 *
 * @param       none
 *
 * @return      value of overflow counter
 **************************************************************************************************
 */
uint32 macMcuOverflowCount(void)
{
  uint32 overflowCount;

  /* for efficiency, the 32-bit value is encoded using endian abstracted indexing */

  /* must read T2OF0 first, this latches T2OF1 and T2OF2 */
  ((uint8 *)&overflowCount)[UINT32_NDX0] = T2OF0;
  ((uint8 *)&overflowCount)[UINT32_NDX1] = T2OF1;
  ((uint8 *)&overflowCount)[UINT32_NDX2] = T2OF2;
  ((uint8 *)&overflowCount)[UINT32_NDX3] = 0;

  return (overflowCount);
}


/**************************************************************************************************
 * @fn          macMcuOverflowCapture
 *
 * @brief       Returns the last capture of the overflow counter.  A special hardware feature
 *              captures the overflow counter when the regular hardware timer is captured.
 *
 * @param       none
 *
 * @return      last capture of overflow count
 **************************************************************************************************
 */
uint32 macMcuOverflowCapture(void)
{
  uint32 overflowCapture;

  /* for efficiency, the 32-bit value is encoded using endian abstracted indexing */

  ((uint8 *)&overflowCapture)[UINT32_NDX0] = T2PEROF0;
  ((uint8 *)&overflowCapture)[UINT32_NDX1] = T2PEROF1;
  ((uint8 *)&overflowCapture)[UINT32_NDX2] = T2PEROF2 & PEROF2_BITS;
  ((uint8 *)&overflowCapture)[UINT32_NDX3] = 0;

  return (overflowCapture);
}


/**************************************************************************************************
 * @fn          macMcuOverflowSetCount
 *
 * @brief       Sets the value of the hardware overflow counter.
 *
 * @param       count - new overflow count value
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuOverflowSetCount(uint32 count)
{
  MAC_ASSERT(!((count >> 16) & ~PEROF2_BITS));   /* illegal count value */

  /* for efficiency, the 32-bit value is decoded using endian abstracted indexing */

  /* T2OF2 must be written last */
  T2OF0 = ((uint8 *)&count)[UINT32_NDX0];
  T2OF1 = ((uint8 *)&count)[UINT32_NDX1];
  T2OF2 = ((uint8 *)&count)[UINT32_NDX2];
}


/**************************************************************************************************
 * @fn          macMcuOverflowSetCompare
 *
 * @brief       Set overflow count compare value.  An interrupt is triggered when the overflow
 *              count equals this compare value.
 *
 * @param       count - overflow count compare value
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuOverflowSetCompare(uint32 count)
{
  halIntState_t  s;
  uint8 imBits;

  MAC_ASSERT(!((count >> 16) & ~PEROF2_BITS));   /* illegal count value */

  HAL_ENTER_CRITICAL_SECTION(s);

  /* remember value of interrupt mask bits, allows OFCMPIM bit to be restored later */
  imBits = T2PEROF2 & ~PEROF2_BITS;

  /*
   *  Disable overflow compare interrupts.  (It is OK to write zero to compare part of
   *  this register.  Interrupts are disabled and the new value will be written and the
   *  interrupt flag cleared before interrupts are re-enabled.)
   */
  T2PEROF2 = imBits & ~OFCMPIM;
  T2PEROF2 = imBits & ~OFCMPIM;  /* writing twice is chip bug workaround */

  /* for efficiency, the 32-bit value is decoded using endian abstracted indexing */
  T2PEROF0 = ((uint8 *)&count)[UINT32_NDX0];
  T2PEROF1 = ((uint8 *)&count)[UINT32_NDX1];

  /* see declaration of this shadow variable for more information */
  shadowPerof2 = ((uint8 *)&count)[UINT32_NDX2];

  /* write the compare value part of T2PEROF2 without re-enabling overflow compare interrupts */
  T2PEROF2 = (imBits & ~OFCMPIM) | shadowPerof2;
  T2PEROF2 = (imBits & ~OFCMPIM) | shadowPerof2;  /* writing twice is chip bug workaround */

  /*
   *  Now that new compare value is stored, clear the interrupt flag.  This is important just
   *  in case a false match was generated as the multi-byte compare value was written.
   */
  T2CNF = T2CNF_BASE_VALUE | (~OFCMPIF & T2CNF_IF_BITS);

  /* re-enable overflow compare interrupts if they were previously enabled */
  T2PEROF2 = imBits | shadowPerof2;
  T2PEROF2 = imBits | shadowPerof2;  /* writing twice is chip bug workaround */

  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macMcuTimer2Isr
 *
 * @brief       Interrupt service routine for timer2, the MAC timer.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
HAL_ISR_FUNCTION( macMcuTimer2Isr, T2_VECTOR )
{
  uint8 t2perof2;

  /* temporary variable used to suppress volatile access order warning */
  t2perof2 = T2PEROF2;

  /*------------------------------------------------------------------------------------------------
   *  Overflow compare interrupt - triggers when then overflow counter is
   *  equal to the overflow compare register.
   */
  if ((T2CNF & OFCMPIF) & t2perof2)
  {
    /* call function for dealing with the timer compare interrupt */
    macBackoffTimerCompareIsr();

    /*
     *  NOTE : The interrupt flag for overflow compare is not cleared here.  This is done
     *         in the code that sets a new overflow compare value.
     */
  }

  /*------------------------------------------------------------------------------------------------
   *  Overflow interrupt - triggers when the hardware timer rolls over.
   */
  else if ((T2CNF & PERIF) & t2perof2)
  {
    /* call energy detect interrupt function, this interrupt not used for any other functionality */
    mcuRecordMaxRssiIsr();

    /* clear the interrupt flag by writing a zero to only that flag, write one to other flags */
    T2CNF = T2CNF_BASE_VALUE | (~PERIF & T2CNF_IF_BITS);
  }
}


/**************************************************************************************************
 * @fn          macMcuOrT2PEROF2
 *
 * @brief       This function is used to OR the interrupt mask bits in register T2PERFOF2.
 *              Because of the hardware design a shadow register is required to preserve the
 *              other bits in this register.  See the declaration of variable shadowPerof2 above
 *              for a full description.
 *
 * @param       orValue - value to OR register with
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuOrT2PEROF2(uint8 orValue)
{
  halIntState_t  s;

  MAC_ASSERT(!(orValue & PEROF2_BITS)); /* only interrupt mask bits should be affected */

  /*
   *  Perform OR operation only on interrupt mask bits.  The shadow register preserves
   *  the value previously written to the other bits in the register.
   */
  HAL_ENTER_CRITICAL_SECTION(s);
  T2PEROF2 = ((T2PEROF2 & ~PEROF2_BITS) | orValue) | shadowPerof2;
  T2PEROF2 = ((T2PEROF2 & ~PEROF2_BITS) | orValue) | shadowPerof2;  /* writing twice is chip bug workaround */
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macMcuAndT2PEROF2
 *
 * @brief       This function is used to AND the interrupt mask bits in register T2PERFOF2.
 *              Because of the hardware design a shadow register is required to preserve the
 *              other bits in this register.  See the declaration of variable shadowPerof2 above
 *              for a full description.
 *
 * @param       andValue - value to AND register with
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuAndT2PEROF2(uint8 andValue)
{
  halIntState_t  s;

  MAC_ASSERT((andValue & PEROF2_BITS) == PEROF2_BITS); /* only interrupt mask bits should be affected */

  /*
   *  Perform AND operation only on interrupt mask bits.  The shadow register preserves
   *  the value previously written to the other bits in the register.
   */
  HAL_ENTER_CRITICAL_SECTION(s);
  T2PEROF2 = ((T2PEROF2 & ~PEROF2_BITS) & andValue) | shadowPerof2;
  T2PEROF2 = ((T2PEROF2 & ~PEROF2_BITS) & andValue) | shadowPerof2;  /* writing twice is chip bug workaround */
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macMcuRfIsr
 *
 * @brief       Interrupt service routine that handles all RF interrupts.  There are a number
 *              of conditions "ganged" onto this one ISR so each condition must be tested for.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
HAL_ISR_FUNCTION( macMcuRfIsr, RF_VECTOR )
{
  uint8 rfim;

  rfim = RFIM;

  if ((RFIF & IRQ_CSP_INT) & rfim)
  {
    /*
     *  Important!  Because of how the CSP programs are written, CSP_INT interrupts should
     *  be processed before CSP_STOP interrupts.  This becomes an issue when there are
     *  long critical sections.
     */
    /* clear flag */
    RFIF = ~IRQ_CSP_INT;
    macCspTxIntIsr();
  }
  else if ((RFIF & IRQ_CSP_STOP) & rfim)
  {
    /* clear flag */
    RFIF = ~IRQ_CSP_STOP;
    macCspTxStopIsr();
  }
  else if ((RFIF & IRQ_TXDONE) & rfim)
  {
    /* disable interrupt - set up is for "one shot" operation */
    HAL_DISABLE_INTERRUPTS();
    RFIM &= ~IM_TXDONE;
    HAL_ENABLE_INTERRUPTS();
//////////////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : On Rev B parts the TXDONE signal does not fire for transmitted ACKs.
//  As a workaround, the SFD signal is used instead (the TXDONE #defines used here have
//  been redefined elsewhere to be SFD values).  There is a problem with the SFD signal
//  though... it fires at the *start* of an ACK.  To get around this, the code pends until
//  the TX_ACTIVE signal goes inactive.  Ugly.  Delete posthaste once Rev B is obsolete.
#ifndef _REMOVE_REV_B_WORKAROUNDS
    while (RFSTATUS & TX_ACTIVE);
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif
//////////////////////////////////////////////////////////////////////////////////////////////
    macRxAckTxDoneCallback();
  }
  else if ((RFIF & IRQ_FIFOP) & rfim)
  {
    /* continue to execute interrupt handler as long as FIFOP is active */
    do
    {
      macRxThresholdIsr();
      RFIF = ~IRQ_FIFOP;
    } while (RFSTATUS & FIFOP);
  }

/////////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : workaround for chip bug #297, replace with following code when fixed
/////////////////////////////////////////////////////////////////////////////////////////
#ifndef _REMOVE_REV_B_WORKAROUNDS
  S1CON = 0x00;
  HAL_DISABLE_INTERRUPTS();
  rfim = RFIM;
  if (RFIF & rfim)
  {
    S1CON = 0x03;
  }
  HAL_ENABLE_INTERRUPTS();
#else
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//  keep this code, delete the rest
  S1CON = 0x00;
  RFIF = 0xFF;
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif
/////////////////////////////////////////////////////////////////////////////////////////
}



/////////////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : part of workaround for chip bug #297, delete it when possible
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _REMOVE_REV_B_WORKAROUNDS
/**************************************************************************************************
 * @fn          macMcuOrRFIM
 *
 * @brief       Workaround code that should disappear when Rev B is obsolete.
 *
 * @param       value - value to "or" into RFIM
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuOrRFIM(uint8 value)
{
  halIntState_t  s;

  HAL_ENTER_CRITICAL_SECTION(s);
  S1CON = 0x00;
  RFIM |= value;
  {
    uint8 rfim;
    rfim = RFIM;
    if (RFIF & rfim)
    {
      S1CON = 0x03;
    }
  }
  HAL_EXIT_CRITICAL_SECTION(s);
}

/**************************************************************************************************
 * @fn          macMcuAndRFIM
 *
 * @brief       Workaround code that should disappear when Rev B is obsolete.
 *
 * @param       value - value to "and" into RFIM
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuAndRFIM(uint8 value)
{
  halIntState_t  s;

  HAL_ENTER_CRITICAL_SECTION(s);
  S1CON = 0x00;
  RFIM &= value;
  {
    uint8 rfim;
    rfim = RFIM;
    if (RFIF & rfim)
    {
      S1CON = 0x03;
    }
  }
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macMcuWriteRFIF
 *
 * @brief       Workaround code that should disappear when Rev B is obsolete.
 *
 * @param       value - value write to RFIF
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuWriteRFIF(uint8 value)
{
  halIntState_t  s;
  uint8 rfim;

  HAL_ENTER_CRITICAL_SECTION(s);
  RFIF = value;
  S1CON = 0x00;
  rfim = RFIM;
  if (RFIF & rfim)
  {
    S1CON = 0x03;
  }
  HAL_EXIT_CRITICAL_SECTION(s);
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////


/**************************************************************************************************
 * @fn          macMcuRecordMaxRssiStart
 *
 * @brief       Starts recording of the maximum received RSSI value.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macMcuRecordMaxRssiStart(void)
{
  /* start maximum recorded value at the lowest possible value */
  maxRssi = -128;

  /* enable timer overflow interrupt */
  macMcuOrT2PEROF2(PERIM);
}


/**************************************************************************************************
 * @fn          macMcuRecordMaxRssiStop
 *
 * @brief       Stops recording of the maximum received RSSI.  It returns the maximum value
 *              received since starting the recording.
 *
 * @param       none
 *
 * @return      maximum received RSSI value
 **************************************************************************************************
 */
int8 macMcuRecordMaxRssiStop(void)
{
  /* disable timer overflow interrupt */
  macMcuAndT2PEROF2(~PERIM);

  return(maxRssi);
}


/*=================================================================================================
 * @fn          macMcuRecordMaxRssiIsr
 *
 * @brief       Interrupt service routine called during recording of max RSSI value.
 *
 * @param       none
 *
 * @return      none
 *=================================================================================================
 */
static void mcuRecordMaxRssiIsr(void)
{
  int8 rssi;

  /* read latest RSSI value */
  rssi = RSSIL;

  /* if new RSSI value is greater than the maximum already received, it is the new maximum */
  if (rssi > maxRssi)
  {
    maxRssi = rssi;
  }
}



/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */
#if ((IRQ_SFD != IM_SFD) || (IRQ_FIFOP != IM_FIFOP) || (IRQ_TXDONE != IM_TXDONE))
#error "ERROR: Compile time error with RFIF vs RFIM register defines."
#endif

#if ((OFCMPIF != OFCMPIM) || (PERIF != PERIM) || (CMPIF != CMPIM))
#error "ERROR: Compile time error with T2CNF vs T2PEROF2 register defines."
#endif


/**************************************************************************************************
*/
