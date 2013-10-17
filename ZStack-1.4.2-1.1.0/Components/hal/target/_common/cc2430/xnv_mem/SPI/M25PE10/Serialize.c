/****** implementation File for support of STFL-I based Serial Flash Memory Driver *****

   Filename:    Serialize.c
   Description:  Support to c2076.c. This files is aimed at giving a basic
    example of the SPI serial interface used to communicate with STMicroelectronics
    serial Flash devices. The functions below are used in an environment where the
    master has an embedded SPI port (STMicroelectronics µPSD).

   Version:     1.0
   Date:        08-11-2004
   Authors:    Tan Zhi, STMicroelectronics, Shanghai (China)
   Copyright (c) 2004 STMicroelectronics.

   THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH
   CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS A
   RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
   CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT OF SUCH
   SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN
   IN CONNECTION WITH THEIR PRODUCTS.
********************************************************************************

   Version History.
   Ver.   Date      Comments

   1.0   08/11/2004   Initial release

*******************************************************************************/
#include "ioCC2430.h"
#include "hal_types.h"
#include "hal_defs.h"
#include "Serialize.h"
#include "FlashUtils.h"

#define STATIC

/***************UART and SPI copnfiguration macros from ./external/.../hal.h*****************/

/***********************************  Configure UARTs  *************************************/
#define IO_PER_LOC_UART1_AT_PORT0_PIN2345() do { PERCFG = (PERCFG&~0x02)|0x00; } while (0)
#define IO_PER_LOC_UART1_AT_PORT1_PIN4567() do { PERCFG = (PERCFG&~0x02)|0x02; } while (0)

#define IO_PER_LOC_UART0_AT_PORT0_PIN2345() do { PERCFG = (PERCFG&~0x01)|0x00; } while (0)
#define IO_PER_LOC_UART0_AT_PORT1_PIN2345() do { PERCFG = (PERCFG&~0x01)|0x01; } while (0)

// Macro for getting the clock division factor
#define CLKSPD  (CLKCON & 0x07)

// baud rate macro definitions
#define BR_2400          1
#define BR_4800          2
#define BR_9600          3
#define BR_14400         4
#define BR_19200         5
#define BR_28800         6
#define BR_38400         7
#define BR_57600         8
#define BR_76800         9
#define BR_115200       10
#define BR_153600       11
#define BR_230400       12
#define BR_307200       13

//*****************************************************************************
// Macro for setting up an SPI connection. The macro configures the appropriate
// pins for peripheral operation, sets the baudrate if the chip is configured
// to be SPI master, and sets the desired clock polarity and phase. Whether to
// transfer MSB or LSB first is also determined. _spi_ indicates whether
// to use spi 0 or 1. _baudRate_ must be one of 2400, 4800, 9600, 14400, 19200,
// 28800, 38400, 57600, 76800, 115200, 153600, 230400 or 307200.
// Possible options are defined below.

// The macros in this section simplify UART operation.
#define BAUD_E(baud, clkDivPow) (        \
    (baud==BR_2400)   ?  6  +clkDivPow : \
    (baud==BR_4800)   ?  7  +clkDivPow : \
    (baud==BR_9600)   ?  8  +clkDivPow : \
    (baud==BR_14400)  ?  8  +clkDivPow : \
    (baud==BR_19200)  ?  9  +clkDivPow : \
    (baud==BR_28800)  ?  9  +clkDivPow : \
    (baud==BR_38400)  ?  10 +clkDivPow : \
    (baud==BR_57600)  ?  10 +clkDivPow : \
    (baud==BR_76800)  ?  11 +clkDivPow : \
    (baud==BR_115200) ?  11 +clkDivPow : \
    (baud==BR_153600) ?  12 +clkDivPow : \
    (baud==BR_230400) ?  12 +clkDivPow : \
    (baud==BR_307200) ?  13 +clkDivPow : \
    0  )


#define BAUD_M(baud) (         \
    (baud==BR_2400)   ?  59  : \
    (baud==BR_4800)   ?  59  : \
    (baud==BR_9600)   ?  59  : \
    (baud==BR_14400)  ?  216 : \
    (baud==BR_19200)  ?  59  : \
    (baud==BR_28800)  ?  216 : \
    (baud==BR_38400)  ?  59  : \
    (baud==BR_57600)  ?  216 : \
    (baud==BR_76800)  ?  59  : \
    (baud==BR_115200) ?  216 : \
    (baud==BR_153600) ?  59  : \
    (baud==BR_230400) ?  216 : \
    (baud==BR_307200) ?  59  : \
    0)

#define SPI_SETUP(spi, baudRate, options)           \
   do {                                             \
      U##spi##UCR = 0x80;                           \
      U##spi##CSR = 0x00;                           \
                                                    \
      if(spi == 0){                                 \
         if(PERCFG & 0x01){                         \
            P1SEL |= 0x3C;                          \
         } else {                                   \
            P0SEL |= 0x3C;                          \
         }                                          \
      }                                             \
      else {                                        \
         if(PERCFG & 0x02){                         \
            P1SEL |= 0xF0;                          \
         } else {                                   \
            P0SEL |= 0x3C;                          \
         }                                          \
      }                                             \
                                                    \
      if(options & SPI_SLAVE){                      \
         U##spi##CSR = 0x20;                        \
      }                                             \
      else {                                        \
         U##spi##GCR = BAUD_E(baudRate, CLKSPD);    \
         U##spi##BAUD = BAUD_M(baudRate);           \
      }                                             \
      U##spi##GCR |= (options & 0xE0);              \
   } while(0)


// Options for the SPI_SETUP macro.
#define SPI_SLAVE              0x01
#define SPI_MASTER             0x00
#define SPI_CLOCK_POL_LO       0x00
#define SPI_CLOCK_POL_HI       0x80
#define SPI_CLOCK_PHA_0        0x00
#define SPI_CLOCK_PHA_1        0x40
#define SPI_TRANSFER_MSB_FIRST 0x20
#define SPI_TRANSFER_MSB_LAST  0x00

/***************************************************************************/
#ifdef PROTOTYPE_DATAFLASH_SUPPORT
  #define SLAVE_CHIP_SELECT()     (P2 &= 0xFE)
  #define SLAVE_CHIP_DESELECT()   (P2 |= 1)
  #define IOCHARBUF               U0DBUF
  #define UARTCSR                 U0CSR
#else
  #define SLAVE_CHIP_SELECT()     (P1 &= 0xEF)
  #define SLAVE_CHIP_DESELECT()   (P1 |= 0x10)
  #define IOCHARBUF               U1DBUF
  #define UARTCSR                 U1CSR

#endif

#ifdef CC2430_BOOT_CODE
STATIC __near_func void ConfigureSpiMaster(SpiMasterConfigOptions opt);
#else
STATIC void ConfigureSpiMaster(SpiMasterConfigOptions opt);
#endif

STATIC int8 SPIReadWrite(uint8 which, uint32 address, uint8 *buf, uint16 len);

STATIC uint8 s_xmemIsInit;

/*******************************************************************************
Function:     DF_spiInit(void)
Arguments:
Return Values:There is no return value for this function.
Description:  This function is a one-time configuration for the CPU to set some
              ports to work in SPI mode (when they have multiple functions. For
              example, in some CPUs, the ports can be GPIO pins or SPI pins if
              properly configured).
              please refer to the specific CPU datasheet for proper
              configurations.
*******************************************************************************/
#ifdef CC2430_BOOT_CODE
__near_func
#endif
void DF_spiInit( int8 (**readWrite)(uint8, uint32, uint8 *, uint16))
{
   // configure SPI

  if (!s_xmemIsInit)  {
    s_xmemIsInit = 1;

    // The TI reference design uses UART1 Alt. 2 in SPI mode
    IO_PER_LOC_UART1_AT_PORT1_PIN4567();
    SPI_SETUP(1, BR_115200, SPI_MASTER       | \
                            SPI_CLOCK_POL_LO | \
                            SPI_CLOCK_PHA_0  | \
                            SPI_TRANSFER_MSB_FIRST);

    // There is some awkwardness here. The SPI_SETUP macro above set P1.4 as a
    // peripheral pin in support of SPI. But the reference design uses P1.4 as
    // the /CS for the external dataflash part. We need to reconfigure the pin
    // as a GPIO pin, set the direction register to output, and set it high to
    // deselect the part.
    P1SEL &= ~BV(4);    // set P1.4 as GPIO pin
    P1DIR |=  BV(4);  // set P1.4 as output
    P1    |=  BV(4);  // set output high
  }

  *readWrite  = SPIReadWrite;
}

int8 SPIReadWrite(uint8 which, uint32 address, uint8 *buf, uint16 len)
{
  ParameterType param;

  param.Read.udAddr               = address;
  param.Read.udNrOfElementsToRead = len;
  param.Read.pArray               = buf;

  if (which == XMEM_READ)  {
      Flash(Read, &param);
  }
  else  {
      Flash(PageWrite, &param);
  }

  return 0;
}

/*******************************************************************************
Function:     ConfigureSpiMaster(SpiMasterConfigOptions opt)
Arguments:    opt configuration options, all acceptable values are enumerated in
              SpiMasterConfigOptions, which is a typedefed enum.
Return Values:There is no return value for this function.
Description:  This function can be used to properly configure the SPI master
              before and after the transfer/receive operation
Pseudo Code:
   Step 1  : perform or skip select/deselect slave
   Step 2  : perform or skip enable/disable transfer
   Step 3  : perform or skip enable/disable receive
*******************************************************************************/
STATIC
#ifdef CC2430_BOOT_CODE
__near_func
#endif
void ConfigureSpiMaster(SpiMasterConfigOptions opt)
{
/*
    if(enumNull == opt)
      return;
*/
    if(opt & MaskBit_SelectSlave_Relevant)   (opt & MaskBit_SlaveSelect) ? SLAVE_CHIP_SELECT() : SLAVE_CHIP_DESELECT();
/**
    if(opt & MaskBit_Trans_Relevant)(opt & MaskBit_Trans) ? EnableTrans():DisableTrans();
    if(opt & MaskBit_Recv_Relevant) (opt & MaskBit_Recv)  ? EnableRcv():DisableRcv();
**/
}

/*******************************************************************************
Function:     Serialize(const CharStream* char_stream_send,
              CharStream* char_stream_recv,
              SpiMasterConfigOptions optBefore,
              SpiMasterConfigOptions optAfter
              )
Arguments:    char_stream_send, the char stream to be sent from the SPI master to
              the Flash memory, usually contains instruction, address, and data to be
              programmed.
              char_stream_recv, the char stream to be received from the Flash memory
              to the SPI master, usually contains data to be read from the memory.
              optBefore, configurations of the SPI master before any transfer/receive
              optAfter, configurations of the SPI after any transfer/receive
Return Values:TRUE
Description:  This function can be used to encapsulate a complete transfer/receive
              operation
Pseudo Code:
   Step 1  : perform pre-transfer configuration
   Step 2  : perform transfer/ receive
    Step 2-1: transfer ...
        (a typical process, it may vary with the specific CPU)
        Step 2-1-1:  check until the SPI master is available
        Step 2-1-2:  send the byte stream cycle after cycle. it usually involves:
                     a) checking until the transfer-data-register is ready
                     b) filling the register with a new byte
    Step 2-2: receive ...
        (a typical process, it may vary with the specific CPU)
        Step 2-2-1:  Execute ONE pre-read cycle to clear the receive-data-register.
        Step 2-2-2:  receive the byte stream cycle after cycle. it usually involves:
                     a) triggering a dummy cycle
                     b) checking until the transfer-data-register is ready(full)
                     c) reading the transfer-data-register
   Step 3  : perform post-transfer configuration
*******************************************************************************/
#ifdef CC2430_BOOT_CODE
__near_func
#endif
Bool Serialize(const CharStream* char_stream_send,
               CharStream* char_stream_recv,
               SpiMasterConfigOptions optBefore,
               SpiMasterConfigOptions optAfter
               )
{
  uint16 inLen, outLen;
  uint8 *pIn, *pOut;

  if (char_stream_send)  {
    pOut   = char_stream_send->pChar;
    outLen = char_stream_send->length;
  }
  else  {
    outLen = 0;
  }

  if (char_stream_recv)  {
    pIn   = char_stream_recv->pChar;
    inLen = char_stream_recv->length;
  }
  else  {
    inLen = 0;
  }

  ConfigureSpiMaster(optBefore);

  while (outLen--)  {
    while (UARTCSR & 1)  ;
    IOCHARBUF = *pOut++;
  }



  // wait for last Tx to finish.
  while (UARTCSR & 1) ;

  while (inLen--)  {
    // dummy write
    IOCHARBUF = 0;
    while (UARTCSR & 1) ;
    *pIn++ = IOCHARBUF;
  }

  ConfigureSpiMaster(optAfter);

/**
    ST_uint32 i;
    ST_uint32 length;
    unsigned char* pChar;

    // Step 1  : perform pre-transfer configuration
    ConfigureSpiMaster(optBefore);

    // Step 2  : perform transfer / receive
    // Step 2-1: transfer ...
    length = char_stream_send->length;
    pChar  = char_stream_send->pChar;
    // 2-1-1 Wait until SPI is available
	while (SPISTAT & SPI_FLAG_BUSY);
    // 2-1-2 send the byte stream cycle after cycle
    for(i = 0; i < length; ++i)
    {
	    while (!(SPISTAT & SPI_FLAG_TI));			// check until the transfer-data-register is ready(not full)
	    SPITDR = *(pChar++);			        // fill the register with a new byte
    }
	while (!(SPISTAT & SPI_FLAG_TRANSMIT_END));

     // Step 2-2: receive ...
     // Step 2-2-1:  execute ONE pre-read cycle to clear the receive-data-register.
	 i = SPIRDR;									
     // Step 2-2-2:  send the byte stream cycle after cycle.
     if(ptrNull != (int)char_stream_recv)		    	// skip if no reception needed
     {
         length = char_stream_recv->length;
         pChar  = char_stream_recv->pChar;
         for(i = 0; i < length; ++i)
         {
             SPITDR = SPI_FLASH_INS_DUMMY;                      // triggering a dummy cycle
             while (!(SPISTAT & SPI_FLAG_TI));
             while (!(SPISTAT & SPI_FLAG_TRANSMIT_END));	// checking until the transfer-data-register is ready
             while (!(SPISTAT & SPI_FLAG_RI));

             *(pChar++) = SPIRDR;				// read the transfer-data-register
         }
         while (!(SPISTAT & SPI_FLAG_TRANSMIT_END));
     }

    // Step 3  : perform post-transfer configuration
    ConfigureSpiMaster(optAfter);

**/
    return TRUE;
}

