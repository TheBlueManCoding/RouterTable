#ifndef GlobalInstancesH
#define GlobalInstancesH

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LCDMenuLib.h>
#include "Settings.h"
#include "Textfield.h"
#include "GrblMaster.h"
#include "Config.h"
#include "Logger.h"
#include "Keypad.h"

// lib config
#define _LCDML_DISP_cfg_button_press_time 100 // button press time in ms
#define _LCDML_DISP_cfg_scrollbar 1           // enable a scrollbar
#define _LCDML_DISP_cfg_cursor 0x7E           // cursor Symbol

extern LiquidCrystal_I2C lcd;
extern LCDMenuLib LCDML;
void LCDML_lcd_menu_clear();
void LCDML_CONTROL_loop();
void LCDML_lcd_menu_display();

extern Textfield<_LCDML_DISP_cols> lines[_LCDML_DISP_rows];
extern char lastKey;
extern unsigned long g_lcdml_initscreen;
extern unsigned long g_LCDML_BACK_dynTime_LCDML_BACKEND_control;
extern unsigned long g_LCDML_BACK_timer[];
extern const uint8_t g_LCDML_BACK_id__LCDML_BACKEND_menu;

#endif