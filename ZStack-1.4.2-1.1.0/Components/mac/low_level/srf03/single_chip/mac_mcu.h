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

#ifndef MAC_MCU_H
#define MAC_MCU_H


/* ------------------------------------------------------------------------------------------------
 *                                     Compiler Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_mcu.h"
#include "hal_types.h"
#include "hal_defs.h"
#include "hal_board.h"


/* ------------------------------------------------------------------------------------------------
 *                                    Target Specific Defines
 * ------------------------------------------------------------------------------------------------
 */
/* IP0, IP1 */
#define IPX_0                 BV(0)
#define IPX_1                 BV(1)
#define IPX_2                 BV(2)
#define IP_RFERR_RF_DMA_BV    IPX_0
#define IP_RXTX0_T2_BV        IPX_2

/* T2CNF */
#define CMPIF           BV(7)
#define PERIF           BV(6)
#define OFCMPIF         BV(5)
#define SYNC            BV(1)
#define RUN             BV(0)
#define T2CNF_IF_BITS   (CMPIF | PERIF | OFCMPIF)

/* T2PEROF2 */
#define CMPIM           BV(7)
#define PERIM           BV(6)
#define OFCMPIM         BV(5)
#define PEROF2_BITS     (BV(3) | BV(2) | BV(1) | BV(0))

/* RFIF */
#define IRQ_TXDONE      BV(6)
#define IRQ_FIFOP       BV(5)
#define IRQ_SFD         BV(4)
#define IRQ_CSP_STOP    BV(1)
#define IRQ_CSP_INT     BV(0)

/* RFIM */
#define IM_TXDONE       BV(6)
#define IM_FIFOP        BV(5)
#define IM_SFD          BV(4)
#define IM_CSP_STOP     BV(1)
#define IM_CSP_INT      BV(0)

/* SLEEP */
#define XOSC_STB        BV(6)
#define OSC_PD          BV(2)

/* CLKCON */
#define OSC32K          BV(7)
#define OSC             BV(6)

/* IRQSRC */
#define TXACK           BV(0)


/* ------------------------------------------------------------------------------------------------
 *                                       Interrupt Macros
 * ------------------------------------------------------------------------------------------------
 */
////////////////////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : workaround for chip bug #297, remove when Rev B obsolete
#ifndef _REMOVE_REV_B_WORKAROUNDS
void macMcuWriteRFIF(uint8 value);
void macMcuOrRFIM(uint8 value);
void macMcuAndRFIM(uint8 value);

#define MAC_MCU_WRITE_RFIF(x)         macMcuWriteRFIF(x)
#define MAC_MCU_OR_RFIM(x)            macMcuOrRFIM(x)
#define MAC_MCU_AND_RFIM(x)           macMcuAndRFIM(x)

// redefine TXDONE signal as SFD signal, see note in macMcuRfIsr() for more info
#undef IM_TXDONE
#define IM_TXDONE     IM_SFD
#undef IRQ_TXDONE
#define IRQ_TXDONE     IRQ_SFD

#else
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//  keep this code, delete the rest
#define MAC_MCU_WRITE_RFIF(x)         HAL_CRITICAL_STATEMENT( RFIF = x; S1CON = 0x00; RFIF = 0xFF; )
#define MAC_MCU_OR_RFIM(x)            st( RFIM |= x; )  /* compiler must use atomic ORL instruction */
#define MAC_MCU_AND_RFIM(x)           st( RFIM &= x; )  /* compiler must use atomic ANL instruction */
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAC_MCU_FIFOP_ENABLE_INTERRUPT()              MAC_MCU_OR_RFIM(IM_FIFOP)
#define MAC_MCU_FIFOP_DISABLE_INTERRUPT()             MAC_MCU_AND_RFIM(~IM_FIFOP)
#define MAC_MCU_FIFOP_CLEAR_INTERRUPT()               MAC_MCU_WRITE_RFIF(~IRQ_FIFOP)

#define MAC_MCU_TXDONE_ENABLE_INTERRUPT()             MAC_MCU_OR_RFIM(IM_TXDONE)
#define MAC_MCU_TXDONE_DISABLE_INTERRUPT()            MAC_MCU_AND_RFIM(~IM_TXDONE)
#define MAC_MCU_TXDONE_CLEAR_INTERRUPT()              MAC_MCU_WRITE_RFIF(~IRQ_TXDONE)

#define MAC_MCU_CSP_STOP_ENABLE_INTERRUPT()           MAC_MCU_OR_RFIM(IM_CSP_STOP)
#define MAC_MCU_CSP_STOP_DISABLE_INTERRUPT()          MAC_MCU_AND_RFIM(~IM_CSP_STOP)
#define MAC_MCU_CSP_STOP_CLEAR_INTERRUPT()            MAC_MCU_WRITE_RFIF(~IRQ_CSP_STOP)
#define MAC_MCU_CSP_STOP_INTERRUPT_IS_ENABLED()       (RFIM & IM_CSP_STOP)

#define MAC_MCU_CSP_INT_ENABLE_INTERRUPT()            MAC_MCU_OR_RFIM(IM_CSP_INT)
#define MAC_MCU_CSP_INT_DISABLE_INTERRUPT()           MAC_MCU_AND_RFIM(~IM_CSP_INT)
#define MAC_MCU_CSP_INT_CLEAR_INTERRUPT()             MAC_MCU_WRITE_RFIF(~IRQ_CSP_INT)
#define MAC_MCU_CSP_INT_INTERRUPT_IS_ENABLED()        (RFIM & IM_CSP_INT)


/* ------------------------------------------------------------------------------------------------
 *                                       Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void macMcuInit(void);
uint8 macMcuRandomByte(void);
uint8 macMcuTimerCount(void);
uint16 macMcuTimerCapture(void);
uint32 macMcuOverflowCount(void);
uint32 macMcuOverflowCapture(void);
void macMcuOverflowSetCount(uint32 count);
void macMcuOverflowSetCompare(uint32 count);
void macMcuOrT2PEROF2(uint8 value);
void macMcuAndT2PEROF2(uint8 value);
void macMcuRecordMaxRssiStart(void);
int8 macMcuRecordMaxRssiStop(void);
void macMcuRecordMaxRssiIsr(void);


/**************************************************************************************************
 */
#endif
