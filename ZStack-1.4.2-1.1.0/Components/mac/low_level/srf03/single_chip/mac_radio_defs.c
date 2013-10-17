/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-03-23 17:01:39 -0700 (Fri, 23 Mar 2007) $
    Revision:       $Revision: 13847 $

    Description:

    Describe the purpose and contents of the file.

       Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/



/* ------------------------------------------------------------------------------------------------
 *                                             Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "mac_radio_defs.h"
#include "hal_types.h"
#include "hal_assert.h"


/* ------------------------------------------------------------------------------------------------
 *                                        Global Constants
 * ------------------------------------------------------------------------------------------------
 */
const uint8 CODE macRadioDefsTxPowerTable[] =
{
  /*   0 dBm */   0x5F,   /* characterized as -0.4 dBm in datasheet */
  /*  -1 dBm */   0x3F,   /* characterized as -0.9 dBm in datasheet */
  /*  -2 dBm */   0x3F,
  /*  -3 dBm */   0x1B,   /* characterized as -2.7 dBm in datasheet */
  /*  -4 dBm */   0x17,   /* characterized as -4.0 dBm in datasheet */
  /*  -5 dBm */   0x13,   
  /*  -6 dBm */   0x13,   /* characterized as -5.7 dBm in datasheet */
  /*  -7 dBm */   0x13,
  /*  -8 dBm */   0x0F,   /* characterized as -7.9 dBm in datasheet */
  /*  -9 dBm */   0x0F,
  /* -10 dBm */   0x0F,
  /* -11 dBm */   0x0B,   /* characterized as -10.8 dBm in datasheet */
  /* -12 dBm */   0x0B,
  /* -13 dBm */   0x0B,
  /* -14 dBm */   0x0B,
  /* -15 dBm */   0x07,   /* characterized as -15.4 dBm in datasheet */
  /* -16 dBm */   0x07,
  /* -17 dBm */   0x07,
  /* -18 dBm */   0x07,
  /* -19 dBm */   0x06,   /* characterized as -18.6 dBm in datasheet */
  /* -20 dBm */   0x06,
  /* -21 dBm */   0x06,
  /* -22 dBm */   0x06,
  /* -23 dBm */   0x06,
  /* -24 dBm */   0x06,
  /* -25 dBm */   0x03    /* characterized as -25.2 dBm in datasheet */
};


/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */
HAL_ASSERT_SIZE(macRadioDefsTxPowerTable, MAC_RADIO_TX_POWER_MAX_MINUS_DBM+1);  /* array size mismatch */

#if (HAL_CPU_CLOCK_MHZ != 32)
#error "ERROR: The only tested/supported clock speed is 32 MHz."
#endif

#if (MAC_RADIO_RECEIVER_SENSITIVITY_DBM > MAC_SPEC_MIN_RECEIVER_SENSITIVITY)
#error "ERROR: Radio sensitivity does not meet specification."
#endif


/**************************************************************************************************
 */
