#ifndef TEMP_H
#define TEMP_H
#include "ioCC2430.h"
#include "hal.h"

/*
typedef unsigned char       BOOL;

// Data
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;

// Unsigned numbers
typedef unsigned char       UINT8;
typedef unsigned char       INT8U;
typedef unsigned short      UINT16;
typedef unsigned short       INT16U;
typedef unsigned long       UINT32;
typedef unsigned long       INT32U;

// Signed numbers
typedef signed char         INT8;
typedef signed short        INT16;
typedef signed long         INT32;

*/
#define ADC_REF_1_25_V      0x00
#define ADC_14_BIT          0x30
#define ADC_TEMP_SENS       0x0E

#define DISABLE_ALL_INTERRUPTS() (IEN0 = IEN1 = IEN2 = 0x00)

/*
#define ADC_SINGLE_CONVERSION(settings) \
   do{ ADCCON3 = (settings); }while(0)
*/

#define ADC_SAMPLE_SINGLE() \
  do { ADC_STOP(); ADCCON1 |= 0x40;  } while (0)

#define ADC_SAMPLE_READY()  (ADCCON1 & 0x80)

#define ADC_STOP() \
  do { ADCCON1 |= 0x30; } while (0)

#define ADC14_TO_CELSIUS(ADC_VALUE)    ( ((ADC_VALUE) >> 4) - 315)

/******************************************************************************
*******************      Power and clock management        ********************
*******************************************************************************

These macros are used to set power-mode, clock source and clock speed.

******************************************************************************/

// Macro for getting the clock division factor
#define CLKSPD  (CLKCON & 0x07)

// Macro for getting the timer tick division factor.
#define TICKSPD ((CLKCON & 0x38) >> 3)

// Macro for checking status of the crystal oscillator
#define XOSC_STABLE (SLEEP & 0x40)

// Macro for checking status of the high frequency RC oscillator.
#define HIGH_FREQUENCY_RC_OSC_STABLE    (SLEEP & 0x20)


// Macro for setting power mode
#define SET_POWER_MODE(mode)                   \
   do {                                        \
      if(mode == 0)        { SLEEP &= ~0x03; } \
      else if (mode == 3)  { SLEEP |= 0x03;  } \
      else { SLEEP &= ~0x03; SLEEP |= mode;  } \
      PCON |= 0x01;                            \
      asm("NOP");                              \
   }while (0)


// Where _mode_ is one of
#define POWER_MODE_0  0x00  // Clock oscillators on, voltage regulator on
#define POWER_MODE_1  0x01  // 32.768 KHz oscillator on, voltage regulator on
#define POWER_MODE_2  0x02  // 32.768 KHz oscillator on, voltage regulator off
#define POWER_MODE_3  0x03  // All clock oscillators off, voltage regulator off

// Macro for setting the 32 KHz clock source
#define SET_32KHZ_CLOCK_SOURCE(source) \
   do {                                \
      if( source ) {                   \
         CLKCON |= 0x80;               \
      } else {                         \
         CLKCON &= ~0x80;              \
      }                                \
   } while (0)

// Where _source_ is one of
#define CRYSTAL 0x00
#define RC      0x01

/*
// Macro for setting the main clock oscillator source,
//turns off the clock source not used
//changing to XOSC will take approx 150 us
#define SET_MAIN_CLOCK_SOURCE(source) \
   do {                               \
      if(source) {                    \
        CLKCON |= 0x40;               \
        while(!HIGH_FREQUENCY_RC_OSC_STABLE); \
        SLEEP |= 0x04;                \
      }                               \
      else {                          \
        SLEEP &= ~0x04;               \
        while(!XOSC_STABLE);          \
        asm("NOP");                   \
        CLKCON &= ~0x47;              \
        SLEEP |= 0x04;                \
      }                               \
   }while (0)

*/
void InitTempSensor(void);
INT8 GetTemperature(void);
void Temperature_main(void);

#endif
