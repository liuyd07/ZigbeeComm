/******************************************************************************
Filename:     RF04EB.h
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0

Description:
Commonly used macros and function prototypes for use with RF04EB.
Functions decleared in this header is implemented in RF04Dev.c.

See RF04Dev.c for function description
******************************************************************************/

#ifndef RF04EB_H
#define RF04EB_H

#include "ioCC2430.h"
#include "hal.h"

/******************************************************************************
* Button S1
*
******************************************************************************/
#define BUTTON_PUSH         P0_4
#define BUTTON_PRESSED()    (!BUTTON_PUSH)
#define INIT_BUTTON()       (P0DIR &= ~0x10)

BOOL buttonPushed( void );

/******************************************************************************
* Joystick
*
******************************************************************************/
#define JOYSTICK_PUSH         P0_5
#define JOYSTICK_PRESSED()    JOYSTICK_PUSH
#define INIT_JOYSTICK_PUSH() \
    do {                     \
        P0DIR &= ~0x20;      \
        P0INP |= 0x20;       \
    } while (0)

BOOL joystickPushed( void );

typedef enum {CENTRED, LEFT, RIGHT, UP, DOWN} JOYSTICK_DIRECTION;

#define JOYSTICK              P0_6
#define INIT_JOYSTICK()       IO_DIR_PORT_PIN(0, 6, IO_IN)
#define ADC_INPUT_JOYSTICK    0x06

JOYSTICK_DIRECTION getJoystickDirection( void );

/******************************************************************************
* LED
*
* LED1 = GLED (green)
* LED2 = RLED (red)
* LED3 = YLED (yellow)
* LED4 = BLED (blue)
*
* LED2 and LED4 are not connected to CC2430
******************************************************************************/
#define LED_OFF 1
#define LED_ON  0


#define LED1          P1_0
#define LED2          P1_1
#define LED3          P1_2

#define RLED          LED1
#define GLED          LED2
#define YLED          LED3


#define INIT_LED1()      do { LED1 = LED_OFF; IO_DIR_PORT_PIN(1, 0, IO_OUT); P1SEL &= ~0x00;} while (0)
#define INIT_LED2()      do { LED1 = LED_OFF; IO_DIR_PORT_PIN(1, 1, IO_OUT); P1SEL &= ~0x01;} while (0)
#define INIT_LED3()      do { LED3 = LED_OFF; IO_DIR_PORT_PIN(1, 2, IO_OUT); P1SEL &= ~0x02;} while (0)

#define INIT_GLED()      INIT_LED2()
#define INIT_YLED()      INIT_LED3()


#define SET_LED1()  (LED1 = LED_ON)
#define SET_LED3()  (LED3 = LED_ON)

#define SET_GLED()  (GLED = LED_ON)
#define SET_YLED()  (YLED = LED_ON)


#define CLR_LED1()  (LED1 = LED_OFF)
#define CLR_LED3()  (LED3 = LED_OFF)

#define CLR_GLED()  (GLED = LED_OFF)
#define CLR_YLED()  (YLED = LED_OFF)


#define SET_LED_MASK( n )                            \
    do {                                             \
        if ((n) & 0x01) SET_LED1(); else CLR_LED1(); \
        if ((n) & 0x04) SET_LED3(); else CLR_LED3(); \
    } while (0)


/******************************************************************************
* LCD
*
* See lcd.h for lcd fuctions
******************************************************************************/
#define LCD_SDA  P1_2
#define LCD_SCL  P2_0


/******************************************************************************
* UART
*
******************************************************************************/
#define UART_RD          P0_2
#define UART_TD          P0_3


/******************************************************************************
* PWM sound
*
******************************************************************************/
#define SOUND_OUT        P1_2
#define SOUND_IN         P0_0


/******************************************************************************
* Potentiometer
*
******************************************************************************/
#define POT             P0_7
#define INIT_POT()      IO_DIR_PORT_PIN(0, 7, IO_IN);
#define ADC_INPUT_POT   0x07

UINT8 getPotValue( void );


/******************************************************************************
*LCD I/O
*
*******************************************************************************/
#define	LCD_RST     	P1_6
#define	LCD_RS      	P1_7
#define	LCD_CLK     	P1_5
#define	LCD_SDO     	P2_0
#define	LCD_CS      	P1_4
#define LCD_PWR         P1_2

/******************************************************************************
*KEY I/O
*
*******************************************************************************/
#define	KEY_CANCEL     	P0_4
#define	KEY_OK      	P0_5
#endif
