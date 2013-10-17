/******************************************************************************
Filename:     app_ex_util.h
Target:       cc2430
Revised:      16/12-2005
Revision:     1.0

Description:
    Application Example Utilitiy Library definitions.

******************************************************************************/

#ifndef APP_EX_UTIL_H
#define APP_EX_UTIL_H

#include "hal.h"
#include <stdlib.h>
#include <stdio.h>

//Key define
#define ENTER      0x0D
#define BACK_SPACE 0x08
#define ESC        0x1B

// lcdMenu
#define ABORT_MENU 0xFF
UINT8 lcdMenu(char *text[], const UINT8 n);

// string_lib
INT16 getNumber(void);
char uartGetkey (void);

#endif
