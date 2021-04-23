// ============================================================ 
//                                                              
// Router Table Controller for GRBL                      
//                                                            
// ============================================================ 
// todo
// ============================================================ 

  // include libs
  #include <Wire.h> 
  #include <LiquidCrystal_I2C.h>
  #include <LCDMenuLib.h>
  #include "Settings.h"
  #include "Textfield.h"
  #include "GrblMaster.h"
  #include "Config.h"
  #include "Logger.h"

  // lib config
  #define _LCDML_DISP_cfg_button_press_time          100    // button press time in ms
  #define _LCDML_DISP_cfg_scrollbar                  1      // enable a scrollbar
  #define _LCDML_DISP_cfg_cursor                     0x7E   // cursor Symbol

// ********************************************************************* 
// LCDML TYPE SELECT
// *********************************************************************
  //the text field to control the lcd
  Textfield<_LCDML_DISP_cols> lines[_LCDML_DISP_rows] = {
	  Textfield<_LCDML_DISP_cols>(0,0), 
	  Textfield<_LCDML_DISP_cols>(0,1)
  };

  // lcd object
  LiquidCrystal_I2C lcd(0x3F,16,2);  // Set the LCD I2C address 
   
  const uint8_t scroll_bar[5][8] = {
    {B10001, B10001, B10001, B10001, B10001, B10001, B10001, B10001}, // scrollbar top
    {B11111, B11111, B10001, B10001, B10001, B10001, B10001, B10001}, // scroll state 1 
    {B10001, B10001, B11111, B11111, B10001, B10001, B10001, B10001}, // scroll state 2
    {B10001, B10001, B10001, B10001, B11111, B11111, B10001, B10001}, // scroll state 3
    {B10001, B10001, B10001, B10001, B10001, B10001, B11111, B11111}  // scrollbar bottom
  }; 

// *********************************************************************
// LCDML MENU/DISP
// *********************************************************************
  // create menu
  // menu element count - last element id
  // this value must be the same as the last menu element
  #define _LCDML_DISP_cnt    10
  
  // LCDML_root        => layer 0 
  // LCDML_root_X      => layer 1 
  // LCDML_root_X_X    => layer 2 
  // LCDML_root_X_X_X  => layer 3 
  // LCDML_root_... 	 => layer ... 
  
  // LCDMenuLib_add(id, group, prev_layer_element, new_element_num, lang_char_array, callback_function)
  LCDML_DISP_init(_LCDML_DISP_cnt);
  LCDML_DISP_add      (0  , _LCDML_G1  , LCDML_root        , 1  , "Manual mode"        , LCDML_FUNC_manual);
  LCDML_DISP_add      (1  , _LCDML_G1  , LCDML_root        , 2  , "Dado"               , LCDML_FUNC_dado);
  LCDML_DISP_add      (2  , _LCDML_G1  , LCDML_root        , 3  , "Finger Joint"       , LCDML_FUNC);
  LCDML_DISP_add      (3  , _LCDML_G1  , LCDML_root_3      , 4  , "Setup"              , LCDML_FUNC_finger_joint_setup);
  LCDML_DISP_add      (4  , _LCDML_G1  , LCDML_root_3      , 5  , "Male Joint"         , LCDML_FUNC_finger_joint_male);
  LCDML_DISP_add      (5  , _LCDML_G1  , LCDML_root_3      , 6  , "Female Joint"       , LCDML_FUNC_finger_joint_female);
  LCDML_DISP_add      (6  , _LCDML_G1  , LCDML_root        , 7  , "Shutdown"           , LCDML_FUNC_shutdown);
  LCDML_DISP_add      (7  , _LCDML_G1  , LCDML_root        , 8  , "Setup"              , LCDML_FUNC);
  LCDML_DISP_add      (8  , _LCDML_G1  , LCDML_root_8      , 9  , "Change Cutter"      , LCDML_FUNC_change_cutter);
  LCDML_DISP_add      (9  , _LCDML_G1  , LCDML_root_8      , 10 , "Home&resetPos"      , LCDML_FUNC_homing_and_reset_position);
  LCDML_DISP_add      (10 , _LCDML_G1  , LCDML_root_8      , 11 , "Values"             , LCDML_FUNC_setup_values);

  LCDML_DISP_createMenu(_LCDML_DISP_cnt);


// ********************************************************************* 
// LCDML BACKEND (core of the menu, do not change here anything yet)
// ********************************************************************* 
  // define backend function  
  #define _LCDML_BACK_cnt    1  // last backend function id
  
  LCDML_BACK_init(_LCDML_BACK_cnt);
  LCDML_BACK_new_timebased_dynamic (0  , ( 20UL )        , _LCDML_start  , LCDML_BACKEND_control);
  LCDML_BACK_new_timebased_dynamic (1  , ( 100UL )       , _LCDML_stop   , LCDML_BACKEND_menu);
  LCDML_BACK_create();


// *********************************************************************
// SETUP
// *********************************************************************
  void setup()
  {  
#ifdef DEBUG_BUILD
	loggerInit();
#endif
	  
    while(!Serial);                    // wait until serial ready
    Serial.begin(115200);              // start serial
    
	DEBUGP("startup");
    // LCD Begin
    lcd.init();
    lcd.backlight();
    lcd.begin(_LCDML_DISP_cols,_LCDML_DISP_rows);  
	
    lcd.home();                   // go home    

    // set special chars for scrollbar
    lcd.createChar(0, (uint8_t*)scroll_bar[0]);
    lcd.createChar(1, (uint8_t*)scroll_bar[1]);
    lcd.createChar(2, (uint8_t*)scroll_bar[2]);
    lcd.createChar(3, (uint8_t*)scroll_bar[3]);
    lcd.createChar(4, (uint8_t*)scroll_bar[4]);
  
    // Enable all items with _LCDML_G1
    LCDML_DISP_groupEnable(_LCDML_G1); // enable group 1
  
    // LCDMenu Setup
    LCDML_setup(_LCDML_BACK_cnt);
	
	// Grbl System Setup
	Settings::load();
	GrblMaster::reset();
	delay(100);
	GrblMaster::init(Settings::values().common.backslash[(int)Axis::Y],
	Settings::values().common.backslash[(int)Axis::Z],
	Settings::values().common.maxTravel[(int)Axis::Y],
	Settings::values().common.maxTravel[(int)Axis::Z]);
	
	GrblMaster::spindleOn();
  }

// *********************************************************************
// LOOP
// *********************************************************************
  void loop()
  { 
    // this function must called here, do not delete it
    LCDML_run(_LCDML_priority); 
  }




// *********************************************************************
// check some errors - do not change here anything
// *********************************************************************
# if(_LCDML_DISP_rows > _LCDML_DISP_cfg_max_rows)
# error change value of _LCDML_DISP_cfg_max_rows in LCDMenuLib.h
# endif
# if(_LCDML_DISP_cols > _LCDML_DISP_cfg_max_string_length)
# error change value of _LCDML_DISP_cfg_max_string_length in LCDMenuLib.h
# endif
