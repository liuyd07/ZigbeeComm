/******************************************************************************
    Filename:       _hal_dma.c
    Revised:        $Date: 2007-03-13 14:24:09 -0700 (Tue, 13 Mar 2007) $
    Revision:       $Revision: 13747 $

    Description: This file contains the interface to the DMA.

    Copyright (c) 2007 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
******************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "hal_types.h"
#include "hal_defs.h"
#include "hal_dma.h"
#include "hal_mcu.h"
#include "hal_uart.h"

#if HAL_DMA

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

halDMADesc_t dmaCh0;
halDMADesc_t dmaCh1234[4];

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/******************************************************************************
 * @fn      HalDMAInit
 *
 * @brief   DMA Interrupt Service Routine
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
void HalDmaInit( void )
{
  HAL_DMA_SET_ADDR_DESC0( &dmaCh0 );
  HAL_DMA_SET_ADDR_DESC1234( dmaCh1234 );
}

#endif  // #if HAL_DMA
/******************************************************************************
******************************************************************************/
