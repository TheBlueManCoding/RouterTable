// =====================================================================
//
// CONTROL
//
// =====================================================================
// *********************************************************************
#include "GlobalInstances.h"

extern int8_t g_LCDML_BACK_start_stop[1];

void LCDML_CONTROL_setup();

// *********************************************************************
// CONTROL TASK, DO NOT CHANGE
// *********************************************************************
void LCDML_BACK_setup(LCDML_BACKEND_control)
// *********************************************************************
{
  // call setup   
  LCDML_CONTROL_setup();      
}
// backend loop
boolean LCDML_BACK_loop(LCDML_BACKEND_control)
{    
  // call loop
  LCDML_CONTROL_loop();

  // go to next backend function and do not block it
  return true;  
}
// backend stop stable
void LCDML_BACK_stable(LCDML_BACKEND_control)
{
}

// *********************************************************************
// *************** (3) CONTROL WITH ENCODER ****************************
// *********************************************************************

  #include <Keypad.h>
  #include <Button.h>
  #include <Encoder.h>
  #include "Config.h"
  
  #ifdef BIG_KEYBOARD
    const byte ROWS = 5; 
    const byte COLS = 4;
	#define BUTTON_F1     '<'
	#define BUTTON_F2     '>'
	#define BUTTON_UP     'u'
	#define BUTTON_DOWN   'd'
	#define BUTTON_LEFT   'l'
	#define BUTTON_RIGHT  'r'
	#define BUTTON_ESC    'e'
	#define BUTTON_ENTER  '\n'
	
    char keys[ROWS][COLS] = {
	    {'<','>','-','.'},
	    {'1','2','3','u'},
	    {'4','5','6','d'},
	    {'7','8','9','e'},
	    {'l','0','r','\n'},
    };
    
    byte colPins[COLS] = {7,6,5,4}; //connect to the row pinouts of the keypad
    byte rowPins[ROWS] = {8,9,10,11,12}; //connect to the column pinouts of the keypad
  #else
	const byte ROWS = 4; //four rows
	const byte COLS = 3; //three columns
	#define BUTTON_UP     'u'
	#define BUTTON_DOWN   'd'
	#define BUTTON_LEFT   'l'
	#define BUTTON_RIGHT  'r'
	#define BUTTON_ESC    'e'
	#define BUTTON_ENTER  '\n'
	
	char keys[ROWS][COLS] = {
		{'1','2','3'},
		{'4','5','6'},
		{'7','8','9'},
		{'.','0','\n'},
	};
	
	byte rowPins[ROWS] = {10,9,8,7}; //connect to the row pinouts of the keypad
	byte colPins[COLS] = {6,5,4}; //connect to the column pinouts of the keypad
  #endif
  
  Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
  char lastKey = NO_KEY;

// settings
  #define _LCDML_CONTROL_encoder_pin_a           2 // pin encoder b
  #define _LCDML_CONTROL_encoder_pin_b           3 // pin encoder a
  #define _LCDML_CONTROL_encoder_pin_button      17 // A3 // pin taster
  
  #define EMERGENCY_STOP_PIN                     16 // A2
  #define EMERGENCY_UNLOCK_PIN                   15 // A1 -> unused at the moment                    

Button buttonEmergencyStop(EMERGENCY_STOP_PIN);
bool emergencyStop = false;

// *********************************************************************
// setup
void LCDML_CONTROL_setup()
{
  // set encoder update intervall time 
  LCDML_BACK_dynamic_setLoopTime(LCDML_BACKEND_control, 1UL);  // 5ms 

  // init pins  
  pinMode(_LCDML_CONTROL_encoder_pin_a      , INPUT_PULLUP);
  pinMode(_LCDML_CONTROL_encoder_pin_b      , INPUT_PULLUP);
  pinMode(_LCDML_CONTROL_encoder_pin_button , INPUT_PULLUP); 

  buttonEmergencyStop.begin();
}
// *********************************************************************
// loop
void LCDML_CONTROL_loop()
{    
  // read encoder status
  static Encoder encoder(_LCDML_CONTROL_encoder_pin_a, _LCDML_CONTROL_encoder_pin_b);
  static long lastValue = 0, lastButtonValue = digitalRead(_LCDML_CONTROL_encoder_pin_button);
  unsigned char buttonValue = digitalRead(_LCDML_CONTROL_encoder_pin_button);
  
  // check button
  if (buttonValue != lastButtonValue) {
	  lastButtonValue = buttonValue;
	  
	  if(!buttonValue) {
		  LCDML_BUTTON_enter();
	  }
  }
  
	char key = keypad.getKey();
	lastKey = NO_KEY;
	if (key != NO_KEY) {
		switch (key) {
			case BUTTON_DOWN:   LCDML_BUTTON_down();  break;
			case BUTTON_UP:     LCDML_BUTTON_up();    break;
			case BUTTON_RIGHT:  LCDML_BUTTON_right(); break;
			case BUTTON_ENTER:  LCDML_BUTTON_enter(); break;
			case BUTTON_ESC:    // fall through!
			case BUTTON_LEFT:   LCDML_BUTTON_left();
								LCDML_BUTTON_quit();  break;
			default: lastKey = key;
		}
	}
	
  if (buttonEmergencyStop.toggled()) {
	  if (buttonEmergencyStop.read() == Button::PRESSED) {
		  emergencyStop = true;
		  for(int i=0; i<_LCDML_DISP_rows; i++) {
			  lines[i].setTemporaryText_P(PSTR(""));
		  }
		  lines[0].setTemporaryText_P(PSTR("EMERGENCY STOP!"));
		  
		  while(1) {
			if (buttonEmergencyStop.released()) {
				for(int i=0; i<_LCDML_DISP_rows; i++) {
					lines[i].restore();
				}
				break;				
			}
		  }
	  }
  }

  // check encoder status and set control menu
  long value = encoder.read() / 2;
  if (lastValue != value) {
	  if (value - lastValue < 0) {
		  LCDML_BUTTON_down();
	  } else {
		  LCDML_BUTTON_up();
	  }
	  
	  lastValue = value;
  }
}
// *********************************************************************
// ******************************* END *********************************
// *********************************************************************
