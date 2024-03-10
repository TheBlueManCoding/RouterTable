/*
 * Config.h
 *
 * Created: 12.04.2020 17:16:16
 *  Author: Martin
 */

#ifndef ConfigH
#define ConfigH

//#define DEBUG_BUILD
//#define GRBL_DEBUG
#define BIG_KEYBOARD
#define ENABLE_Z_AXIS
#define ENABLE_BUTTON_F1
#define ENABLE_BUTTON_F2

// settings for lcd
#define _LCDML_DISP_cols 16
#define _LCDML_DISP_rows 2

// button definitions, need to match with buttonmaps in LCDML_CONTROL.cpp
#define BUTTON_F1 '<'
#define BUTTON_F2 '>'
#define BUTTON_UP 'u'
#define BUTTON_DOWN 'd'
#define BUTTON_LEFT 'l'
#define BUTTON_RIGHT 'r'
#define BUTTON_ESC 'e'
#define BUTTON_ENTER '\n'

#endif /* ConfigH */