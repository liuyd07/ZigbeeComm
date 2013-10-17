/*********************************************************************
    Filename:       ZMain.c
    Revised:        $Date: 2007-04-17 16:38:52 -0700 (Tue, 17 Apr 2007) $
    Revision:       $Revision: 14036 $

    Description:    Startup and shutdown code for ZStack
    Notes:          This version targets the Chipcon CC2430DB/CC2430EB

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
#include "OSAL_Memory.h"
#include "OSAL_Nv.h"
#include "OnBoard.h"
#include "ZMAC.h"
#include "MTEL.h"

#include "nwk_globals.h"
#include "ZDApp.h"
#include "ssp.h"
#include "ZGlobals.h"

#ifndef NONWK
  #include "AF.h"
#endif

/* Hal */
#include "hal_lcd.h"
#include "hal_key.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_assert.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// LED Flash counter, waiting for default 64-bit address
#define FLASH_COUNT 20000

// Maximun number of Vdd samples checked before go on
#define MAX_VDD_SAMPLES  3
#define ZMAIN_VDD_LIMIT  HAL_ADC_VDD_LIMIT_4

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

extern __near_func uint8 GetCodeByte(uint32);

extern bool HalAdcCheckVdd (uint8 limit);

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * ZMAIN API JUMP FUNCTIONS
 *
 * If the MINIMIZE_ROOT compile flag is defined, ZMAIN API functions
 * are implemented as "jump functions" located in the ROOT segment,
 * as expected by the NWK object libraries. This allows the actual
 * ZMAIN function bodies to locate outside ROOT memory, increasing
 * space for user defined constants, strings, etc in ROOT memory.
 *
 * If the MINIMIZE_ROOT compile flag in not defined, the ZMAIN API
 * functions are aliased to the similarly-named function bodies and
 * located in the ROOT segment with no "jump function" overhead.
 * This is the default behavior which produces smaller overall code
 * size and maximizes available code space in BANK1...BANK3.
 *
 */

#ifdef MINIMIZE_ROOT
  // ZMAIN functions are not forced into ROOT segment
  #define ZSEG
#else
  // ZMAIN functions are forced into ROOT segment
  #define ZSEG ROOT
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static ZSEG void zmain_dev_info( void );
static ZSEG void zmain_ext_addr( void );
static ZSEG void zmain_ram_init( void );
static ZSEG void zmain_vdd_check( void );

#ifdef LCD_SUPPORTED
static ZSEG void zmain_lcd_init( void );
#endif

/*********************************************************************
 * @fn      main
 * @brief   First function called after startup.
 * @return  don't care
 *********************************************************************/
extern void initUARTtest(void);
extern void Uart_Baud_rate(int Baud_rate);
ZSEG int main( void )
{

  // Turn off interrupts
  osal_int_disable( INTS_ALL );

  // Make sure supply voltage is high enough to run
  zmain_vdd_check();

  // Initialize stack memory
  zmain_ram_init();

  // Initialize board I/O
  InitBoard( OB_COLD );

  // Initialze HAL drivers
  HalDriverInit();

  // Initialize NV System
  osal_nv_init( NULL );

  // Determine the extended address
  zmain_ext_addr();

  // Initialize basic NV items
  zgInit();

  // Initialize the MAC
  ZMacInit();

#ifndef NONWK
  // Since the AF isn't a task, call it's initialization routine
  afInit();
#endif

  // Initialize the operating system
  osal_init_system();

  // Allow interrupts
  osal_int_enable( INTS_ALL );

  // Final board initialization
  InitBoard( OB_READY );

  // Display information about this device
  zmain_dev_info();

  /* Display the device info on the LCD */
#ifdef LCD_SUPPORTED
  zmain_lcd_init();
#endif
initUARTtest();
Uart_Baud_rate(384);

  osal_start_system(); // No Return from here
} // main()

/*********************************************************************
 * @fn      zmain_vdd_check
 * @brief   Check if the Vdd is OK to run the processor.
 * @return  Return if Vdd is ok; otherwise, flash LED, then reset
 *********************************************************************/
static ZSEG void zmain_vdd_check( void )
{
  uint8 vdd_passed_count = 0;
  bool toggle = 0;

  // Initialization for board related stuff such as LEDs
  HAL_BOARD_INIT();

  // Repeat getting the sample until number of failures or successes hits MAX
  // then based on the count value, determine if the device is ready or not
  while ( vdd_passed_count < MAX_VDD_SAMPLES )
  {
    if ( HalAdcCheckVdd (ZMAIN_VDD_LIMIT) )
    {
      vdd_passed_count++;    // Keep track # times Vdd passes in a row
      MicroWait (10000);     // Wait 10ms to try again
    }
    else
    {
      vdd_passed_count = 0;  // Reset passed counter
      MicroWait (50000);     // Wait 50ms
      MicroWait (50000);     // Wait another 50ms to try again
    }

    /* toggle LED1 and LED2 */
    if (vdd_passed_count == 0)
    {
      if ((toggle = !(toggle)))
        HAL_TOGGLE_LED1();
      else
        HAL_TOGGLE_LED2();
    }
  }

  /* turn off LED1 */
  HAL_TURN_OFF_LED1();
  HAL_TURN_OFF_LED2();
}

/*********************************************************************
 * @fn      zmain_ext_addr
 * @brief   Makes extended address if none exists.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_ext_addr( void )
{
  uint8 i;
  uint8 led;
  uint8 tmp;
  uint8 *xad;
  uint16 AtoD;

  // Initialize extended address in NV
  osal_nv_item_init( ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL );
  osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &aExtendedAddress );

  // Check for uninitialized value (erased EEPROM = 0xFF)
  xad = (uint8*)&aExtendedAddress;
  for ( i = 0; i < Z_EXTADDR_LEN; i++ )
    if ( *xad++ != 0xFF ) return;

#ifdef ZDO_COORDINATOR
  tmp = 0x10;
#else
  tmp = 0x20;
#endif
  // Initialize with a simple pattern
  xad = (uint8*)&aExtendedAddress;
  for ( i = 0; i < Z_EXTADDR_LEN; i++ )
    *xad++ = tmp++;

  // Flash LED1 until user hits SW5
  led = HAL_LED_MODE_OFF;
  while ( HAL_KEY_SW_5 != HalKeyRead() )
  {
    MicroWait( 62500 );
    HalLedSet( HAL_LED_1, led^=HAL_LED_MODE_ON );  // Toggle the LED
    MicroWait( 62500 );
  }
  HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );

  // Plug AtoD data into lower bytes
  AtoD = HalAdcRead (HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_10);
  xad = (uint8*)&aExtendedAddress;
  *xad++ = LO_UINT16( AtoD );
  *xad = HI_UINT16( AtoD );

#if !defined( ZTOOL_PORT ) || defined( ZPORT ) || defined( NV_RESTORE )
  // If no support for Z-Tool serial I/O,
  // Write temporary 64-bit address to NV
  osal_nv_write( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &aExtendedAddress );
#endif
}

/*********************************************************************
 * @fn      zmain_dev_info
 * @brief   Gets or makes extended address.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_dev_info ( void )
{
#ifdef LCD_SUPPORTED
  uint8 i;
  uint8 ch;
  uint8 *xad;
  unsigned char lcd_buf[18];

  // Display the extended address
  xad = (uint8*)&aExtendedAddress + Z_EXTADDR_LEN - 1;
  for ( i = 0; i < Z_EXTADDR_LEN*2; xad-- ) {
    ch = (*xad >> 4) & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
    ch = *xad & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
  }
  lcd_buf[Z_EXTADDR_LEN*2] = '\0';
  HalLcdWriteString( "IEEE Address:", HAL_LCD_LINE_1 );
  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_2 );
#endif // LCD
}

/*********************************************************************
 * @fn      zmain_ram_init
 * @brief   Initialize ram for stack "high-water-mark" observations.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_ram_init( void )
{
  uint8 *end;
  uint8 *ptr;

  // Initialize the call (parameter) stack
  end = (uint8*)CSTK_BEG;  // Lower end
  ptr = (uint8*)(*( __idata uint16*)(CSTK_PTR));  // Upper end
  while ( --ptr > end )
    *ptr = STACK_INIT_VALUE;

  // Initialize the return (address) stack
  ptr = (uint8*)RSTK_END - 1;  // Upper end
  while ( --ptr > (uint8*)SP )
    *(__idata uint8*)ptr = STACK_INIT_VALUE;
}

#ifdef LCD_SUPPORTED
/*********************************************************************
 * @fn      zmain_lcd_init
 * @brief   Initialize LCD at start up.
 * @return  none
 *********************************************************************/
static ZSEG void zmain_lcd_init ( void )
{
#ifdef LCD_SD
 // if ( LcdLine1 == NULL )
  {
    HalLcdWriteString( "Figure8 Wireless", HAL_LCD_LINE_1 );

#if defined( MT_MAC_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "MAC-MT Coord", HAL_LCD_LINE_2 );
#else
      HalLcdWriteString( "MAC-MT Device", HAL_LCD_LINE_2 );
#endif // ZDO
#elif defined( MT_NWK_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "NWK Coordinator", HAL_LCD_LINE_2 );
#else
      HalLcdWriteString( "NWK Device", HAL_LCD_LINE_2 );
#endif // ZDO
#endif // MT_FUNC
  }
#endif // LCD_SD
}
#endif

/*********************************************************************
*********************************************************************/
