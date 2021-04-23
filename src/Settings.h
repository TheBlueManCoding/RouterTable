/**
 * static class with all settings of the project.
 * 
 * the full settings base could be saved and load from eeprom.
 */

#ifndef SettingsH
#define SettingsH
#include "Logger.h"
#include "Axis.h"
#include <avr/eeprom.h>
#define EEPROM_START_ADDRESS 0x00
#define VERSION 0x03

class Settings {
  struct Values {
	uint8_t savedToEEprom;
    struct {
		double cutterWidth;
		double maxTravel[(int)Axis::MAX_AXIS];
		double backslash[(int)Axis::MAX_AXIS];
    }common;

    struct {
		double fingerCount;
		double clearanceWidth;
		double cutDepth;
		double sheetWidth;
    }fingerJoint;

    struct {
		double width;
		double position;
		double cutDepth;
		double sheetWidth;
    }dado;
	
	struct {
		double position[(int)Axis::MAX_AXIS];
		double offset[(int)Axis::MAX_AXIS];
	}position;
  };
  
  private:
  
  public:
  static void load() {
	  Values v;
	  
	  eeprom_read_block(&v, EEPROM_START_ADDRESS, sizeof(v));
	  
	  if(v.savedToEEprom != VERSION) {
		DEBUGP("restore");
		v.common.cutterWidth =  10.0;
		v.common.maxTravel[(int)Axis::Y] = 310.0;
		v.common.maxTravel[(int)Axis::Z] = 66.0;
		v.common.backslash[(int)Axis::Y]  = 1;
		v.common.backslash[(int)Axis::Z] = 1;
		  	  
		v.dado.position       = 1.0;
		v.dado.width          = 18.0;
		v.dado.sheetWidth     = 250.0;
		v.dado.cutDepth       = 0.0;
		  	  
		v.fingerJoint.clearanceWidth = 0.1;
		v.fingerJoint.fingerCount    = 10.0;
		v.fingerJoint.sheetWidth     =  250.0;
		v.fingerJoint.cutDepth       = 0.0;
		
		v.position.position[(int)Axis::Y] = 0;
		v.position.position[(int)Axis::Z] = 0;
		v.position.offset[(int)Axis::Y] = 0;
		v.position.offset[(int)Axis::Z] = 0;
		
		save();
	  } else {
		  // do nothing, we get data from eeprom
	  }
	  
	  values() = v;
  }
  
  static Values& values() {
    static Values valuesInstance;
    return valuesInstance;
  }

  static void save() {
	  DEBUG("save");
	  values().savedToEEprom = VERSION;
	  uint8_t* data = (uint8_t*)&values();
	  for(size_t i=0; i<sizeof(Values); i++) {
		  if(eeprom_read_byte((const uint8_t*)EEPROM_START_ADDRESS+i) != *(data+i)) {
			   eeprom_write_byte((uint8_t*)EEPROM_START_ADDRESS+i, *(data+i));
		  }
	  }
  }
};

#endif
