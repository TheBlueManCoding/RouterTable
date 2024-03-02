/* ===================================================================== *
*                                                                       *
* DISPLAY SYSTEM                                                        *
*                                                                       *
* ===================================================================== **/
#include "Textfield.h"
#include "RouterTableCam.h"
#include "GrblMaster.h"
#include "Config.h"
#include "GlobalInstances.h"

bool messageBox(PROGMEM const char* message, int line) {
  lines[line].setText_P(message);

  while(1) {
	LCDML_CONTROL_loop();
	if (LCDML_BUTTON_checkEnter()) {
		LCDML_BUTTON_resetEnter();
		return true;
	}
	  
	if (LCDML_BUTTON_checkLeft()) {
		LCDML_BUTTON_resetLeft();
		return false;
	}
  }
}

void printPassAndPosition(int activeDado, int dadoCount, int activePass, int passCount, double positionY, double positionZ, bool invertedPosition) {
	
	char buffer[20];
	if (dadoCount>1) {
		snprintf_P(buffer, sizeof(buffer), PSTR("D:%i/%i - P:%i/%i"), activeDado+1, dadoCount, activePass+1, passCount);
	} else {
		snprintf_P(buffer, sizeof(buffer), PSTR("Pass %i/%i"), activePass+1, passCount);	
	}
	lines[0].setText(buffer);
	
	char numberY[10];
	char numberZ[10];
	#ifdef ENABLE_Z_AXIS
		dtostrf(positionY, 1, 1, numberY);
		dtostrf(positionZ, 1, 1, numberZ);
		snprintf_P(buffer, sizeof(buffer), PSTR("Y:%s Z:%s %s"), numberY, numberZ, ((invertedPosition)?"inv!":""));
		lines[1].setText(buffer);
	#else
		dtostrf(positionY, 1, 1, numberY);
		snprintf_P(buffer, sizeof(buffer), PSTR("Pos: %s  %s"), numberY, ((invertedPosition)?"inv!":""));
		lines[1].setText(buffer);
	#endif
}

/************************************************************************/
// scan number in parts, one char per call.
// return: true if number is complete
/************************************************************************/
bool scanNumber(double& number, double min, double max) {	
	lines[1].setNumber(number);
	lines[1].clearOnAppend();
	
	LCDML_BUTTON_resetEnter();
	LCDML_BUTTON_resetLeft();
	
	while(1) {
		LCDML_CONTROL_loop();
		
		if (LCDML_BUTTON_checkLeft()) {
			LCDML_BUTTON_resetLeft();
			lines[1].clear();
			return false;
		}
		
		if (LCDML_BUTTON_checkEnter()) {
			LCDML_BUTTON_resetEnter();
			number = atof(lines[1].getText());
			if(number > max || number < min) {
				lines[1].clear();
				DEBUGP("scan:wrong");
				return false;
			}
			lines[1].clear();
			DEBUGN("scan:got",number);
			return true;
		}

		if (lastKey != NO_KEY) {
			lines[1].append(lastKey);
			lastKey = NO_KEY;
		}	
	}
}

void waitEnter() {
	LCDML_BUTTON_resetEnter();
	
	while(1) {
		LCDML_CONTROL_loop();
		
		
		if (LCDML_BUTTON_checkEnter()) {
			LCDML_BUTTON_resetEnter();
		}
	}
	
	LCDML_DISP_funcend();
}

void doManualPositioning(Axis axis, bool canChangeAxis, bool exitOnEnter) {
	double position = GrblMaster::getPosition(axis);
	unsigned long lasttick = millis();
	unsigned long lastSave = millis();
	int lastDirection = 1;
	bool firstmove = true;
	
	struct Helper {
		void printPosition(Axis axis) {
			lines[1].setText((char*)GrblMaster::getAxisString(axis));
			lines[1].appendText_P(PSTR(": "));
			lines[1].appendNumber(GrblMaster::getPosition(axis));
		}
	}helper;
	
	helper.printPosition(axis);
	
	while(1) {
		LCDML_CONTROL_loop();
		double increment = 0;
		
		if(canChangeAxis) {
#ifdef BUTTON_F1
		if (lastKey == BUTTON_F1 && axis == Axis::Z) {
				axis = Axis::Y;
				lastKey = NO_KEY;
				GrblMaster::stopJog();
				position = GrblMaster::getPosition(axis);
				helper.printPosition(axis);
			}
#endif
#ifdef BUTTON_F1
			if (lastKey == BUTTON_F1 && axis == Axis::Y) {
				axis = Axis::Z;
				lastKey = NO_KEY;
				GrblMaster::stopJog();
				position = GrblMaster::getPosition(axis);
				helper.printPosition(axis);
			}
#endif	
		}
		
#ifdef BUTTON_F2
		if (lastKey == BUTTON_F2) {
			lastKey = NO_KEY;
			if(messageBox(PSTR("zero axis?"), 1)) {
				GrblMaster::setPosition(axis, 0.0);
				position = GrblMaster::getPosition(axis);
			}
			helper.printPosition(axis);
		}
#endif

		if(lastKey >= '-') {
			// dont erase key, pass it to the scanner!
			lastKey = NO_KEY;
			lines[1].clear();
			if(scanNumber(position, GrblMaster::getPositionMin(axis), GrblMaster::getPositionMax(axis))) {
				GrblMaster::gotoPositionJog(axis, position);
				helper.printPosition(axis);
				continue;
			}
		}

		bool moved = false;
		if (LCDML_BUTTON_checkUp() || LCDML_BUTTON_checkDown()) {
					
			int direction = (LCDML_BUTTON_checkDown()) ? -1:1;
			
			LCDML_BUTTON_resetUp();
			LCDML_BUTTON_resetDown();
					
			double diff = millis() - lasttick;
			increment = direction;
			//DEBUGN("time", diff);
			if(diff < 60) {
				increment *= 4.0;
			} else if(diff < 70) {
				increment *= 2.0;
			} else if(diff < 90) {
				increment *= 1.0;
			} else {
				increment *= 0.1;
			}
					
			if (direction != lastDirection) {
				lastDirection = direction;
				if(!firstmove) {
					GrblMaster::stopJog();
					position = GrblMaster::getPosition(axis);
				}
				continue;
			}
					
			position += increment;
			lasttick = millis();
			GrblMaster::gotoPositionJog(axis, position);
			helper.printPosition(axis);
			moved = true;
			firstmove = false;
		}
		
		// save the position because during the move its not done because its to slow
		if(!moved) {
			if(millis() > lastSave+1000) {
				GrblMaster::savePositionJog();	
			}
		}
		
		if(exitOnEnter) {
			if (LCDML_BUTTON_checkEnter()) {
				LCDML_BUTTON_resetEnter();
				break;
			}
		} else {
			if (LCDML_BUTTON_checkLeft()) {
				LCDML_BUTTON_resetLeft();
				break;
			}		
		}
	}
	
	GrblMaster::stopJog();
	lines[1].clear();
	
	LCDML_DISP_funcend();
}

/*void doProbe(Axis axis) {
	bool error = false;
	lines[0].setText_P(PSTR("probe to plate"));
	if(GrblMaster::probe(axis, 10, 120, 6000)) {
		lines[0].setText_P(PSTR("probe from plate"));
		if(GrblMaster::probe(axis, -2, 20, 6000)) {
			lines[0].setText_P(PSTR("set new zero"));
			delay(500);
			GrblMaster::setPosition(axis, 0.0);
		} else {
			lines[0].setText_P(PSTR("probe failed"));
			error = true;
		}
		} else {
		lines[0].setText_P(PSTR("probe failed"));
		error = true;
	}
			
	if (error) {
		GrblMaster::reset();
	}
}*/
// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_change_cutter)
// *********************************************************************
{
}

void LCDML_DISP_loop(LCDML_FUNC_change_cutter)
{	
	// disable/lock spindle
	GrblMaster::spindleOff();
	// set new router bit width
	lines[0].setText_P(PSTR("Cutter width?"));
	double oldCutterWidth = Settings::values().common.cutterWidth;
	if(scanNumber(Settings::values().common.cutterWidth, 0, 80)) {
		Settings::save();
		LCDML_DISP_funcend();
	}

	// recalibrate the offset by calculating the difference between new and old cutter
	// --> the results depends on the accuracy of the width setting
	double offset = (Settings::values().common.cutterWidth - oldCutterWidth) / 2;
	GrblMaster::setPosition(Axis::Y, GrblMaster::getPosition(Axis::Y) + offset);
	
	#ifdef ENABLE_Z_AXIS	
	// drive fully up and fence back for router change
	GrblMaster::gotoPositionRaw(Axis::Z, GrblMaster::getPositionMin(Axis::Z));
	delay(500); // required because of some strange bug
	GrblMaster::gotoPositionRaw(Axis::Y, 150.0);
	GrblMaster::gotoPositionRaw(Axis::Z,  GrblMaster::getPositionMax(Axis::Z));	
	
	// get the new zero position
	if(messageBox(PSTR("zero pos Z?"), 0)) {
		lines[0].setText_P(PSTR("select pos Z"));
		GrblMaster::gotoPositionRaw(Axis::Z, 0);
		
		doManualPositioning(Axis::Z, false, true);
		
//		if(messageBox(PSTR("Probe plate Z?"), 0)) {
//			doProbe(Axis::Z);
//		} else {
			lines[0].setText_P(PSTR("set new zero"));
			delay(500);
			GrblMaster::setPosition(Axis::Z, 0.0);
			lines[1].setText_P(PSTR("Z="));
			lines[1].appendNumber(GrblMaster::getPosition(Axis::Z));
			delay(1000);
//		}
	}
	#endif
	
	// search new position by joystick
	if(messageBox(PSTR("zero pos Y?"), 0)) {
		// drive bit down to measure Y with the plate
		GrblMaster::gotoPosition(0, 10.0);
			
		lines[0].setText_P(PSTR("select pos Y"));
		GrblMaster::resetUserOffset(Axis::Y);
		doManualPositioning(Axis::Y, false, true);
		
		// search plate 
//		if(messageBox(PSTR("Probe plate Y?"), 0)) {
//			doProbe(Axis::Y);
//		} else {
			lines[0].setText_P(PSTR("set new zero"));
			delay(500);
			GrblMaster::setPosition(Axis::Y, 0.0);
			lines[1].setText_P(PSTR("Y="));
			lines[1].appendNumber(GrblMaster::getPosition(Axis::Y));
			delay(1000);
//		}
	}
	
	// go to zero position
	GrblMaster::gotoPositionRaw(Axis::Z, 0);
	GrblMaster::gotoPositionRaw(Axis::Y, 0);		
	
	// unlock spindle
	GrblMaster::spindleOn();	

	LCDML_DISP_funcend();
}

void LCDML_DISP_loop_end(LCDML_FUNC_change_cutter) {}

// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_setup_values)
// *********************************************************************
{
}

void LCDML_DISP_loop(LCDML_FUNC_setup_values)
{
	if(LCDML_BUTTON_checkLeft()) {
		LCDML_DISP_funcend();
	}
	
	lines[0].setText_P(PSTR("backslash Y?"));
	if(!scanNumber(Settings::values().common.backslash[(int)Axis::Y], 0, 5)) {
		return;
	}
	
#ifdef ENABLE_Z_AXIS
	lines[0].setText_P(PSTR("backslash Z?"));
	if(!scanNumber(Settings::values().common.backslash[(int)Axis::Z], 0, 5)) {
		return;
	}
#endif

	lines[0].setText_P(PSTR("max travel Y?"));
	if(!scanNumber(Settings::values().common.maxTravel[(int)Axis::Y], 0, 2000)) {
		return;
	}
	
#ifdef ENABLE_Z_AXIS
	lines[0].setText_P(PSTR("max travel Z?"));
	if(!scanNumber(Settings::values().common.maxTravel[(int)Axis::Z], 0, 100)) {
		return;
	}
#endif
	

#if 0	
#ifdef ENABLE_Z_AXIS
	lines[0].setText_P(PSTR("offset Z"));
	if(!scanNumber(Settings::values().position.offset[(int)Axis::Z], -1000, 1000)) {
		return;
	}
	
	lines[0].setText_P(PSTR("mPos Z"));
	if(!scanNumber(Settings::values().position.position[(int)Axis::Z], 0, 1000)) {
		return;
	}
#endif

	lines[0].setText_P(PSTR("offset Y?"));
	if(!scanNumber(Settings::values().position.offset[(int)Axis::Y], -1000, 1000)) {
		return;
	}
	
	lines[0].setText_P(PSTR("mPos Y?"));
	if(!scanNumber(Settings::values().position.position[(int)Axis::Y], 0, 1000)) {
		return;
	}
#endif

	Settings::save();
	Settings::load();
	LCDML_DISP_funcend();
}

void LCDML_DISP_loop_end(LCDML_FUNC_setup_values) {}
	
// cut all the dados or only one dado
void doDados(RouterFenceCam::Dado dados[], int dadoCount, int dadoMaxCount, double sheetWidth) {
	
	DEBUGP("doDados");
	
	// calculate reverse order cuts(needed if sheet wider than travel of fence)
	bool ret = RouterFenceCam::calcDadosReversedOrder(sheetWidth, Settings::values().common.maxTravel[(int)Axis::Y], 
		Settings::values().common.cutterWidth, dados, dadoCount, dadoMaxCount);
		
	if(!ret) {
		lines[0].setText_P(PSTR("err calc dados"));
		delay(1000);
		LCDML_DISP_funcend();
		return;
	}
	
	// do dados
	int activeDado=0;

	// do drives
	int activePass=0;
	RouterFenceCam::Pass passes[30]; int passCount = 0;
	bool firstpass=true;
	while(1) {
		LCDML_CONTROL_loop();

		bool updatePosition = firstpass;
		bool calcNextDado = firstpass;
		bool updateView = firstpass;
				
		firstpass=false;

		if (LCDML_BUTTON_checkDown()) {
			if (activePass < passCount-1) {
				activePass++;
			} else if (activeDado < dadoCount-1) {
				activePass = 0;
				activeDado++;
				calcNextDado=true;
			}
			updateView = true;
			LCDML_BUTTON_resetDown();
		}
				
		if (LCDML_BUTTON_checkUp()) {
			if (activePass > 0) {
				activePass--;
			} else if (activeDado > 0) {
				activeDado--;
				calcNextDado=true;
			}
			updateView = true;
			LCDML_BUTTON_resetUp();
		}
				
		if (LCDML_BUTTON_checkEnter()) {
			updatePosition = true;
			updateView = true;
			LCDML_BUTTON_resetEnter();
		}
				
		if (LCDML_BUTTON_checkLeft()) {
			LCDML_BUTTON_resetLeft();
			LCDML_DISP_funcend();
			return;
		}
				
		if (calcNextDado) {		// calc passes for dado			
			passCount = 0;
			calcNextDado=true; 
			if(!RouterFenceCam::calculateDadoPasses(Settings::values().common.cutterWidth, Settings::values().common.cutterMaxCutDepth, dados[activeDado],
				passes, sizeof(passes)/sizeof(double), passCount)) {
				lines[1].clear();
				messageBox(PSTR("failed calc dado"), 0);
				LCDML_DISP_funcend();
				return;
			}
		}

		if (updateView) {
			printPassAndPosition(activeDado, dadoCount, activePass, passCount, AS_DOUBLE(passes[activePass].posY) + Settings::values().common.cutterWidth, AS_DOUBLE(passes[activePass].posZ),  dados[activeDado].reversedOrder);
		}
				
		if (updatePosition) {
			if (GrblMaster::gotoPosition(AS_DOUBLE(passes[activePass].posY) + Settings::values().common.cutterWidth, AS_DOUBLE(passes[activePass].posZ))) {
				if (activePass < passCount-1) {
					activePass++;
					printPassAndPosition(activeDado, dadoCount, activePass, passCount, AS_DOUBLE(passes[activePass].posY) + Settings::values().common.cutterWidth, AS_DOUBLE(passes[activePass].posZ), dados[activeDado].reversedOrder);
				}
			}
		}
	}
	
	LCDML_DISP_funcend();
}
	
// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_shutdown)
// *********************************************************************
{
}

void LCDML_DISP_loop(LCDML_FUNC_shutdown)
{
	GrblMaster::gotoPositionRaw(-10.0, GrblMaster::getPosition(Axis::Z), true);
	GrblMaster::gotoPositionRaw(GrblMaster::getPosition(Axis::Y), -10.0, true);

	LCDML_DISP_funcend();
}

void LCDML_DISP_loop_end(LCDML_FUNC_shutdown) {}
	
// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_manual)
// *********************************************************************
{
}

void LCDML_DISP_loop(LCDML_FUNC_manual)
{
	#ifdef ENABLE_Z_AXIS
	lines[0].setText_P(PSTR("select position"));
	doManualPositioning(Axis::Z, true, false);
	#else
	lines[0].setText_P(PSTR("select pos Y"));
	doManualPositioning(Axis::Y, false, false);
	#endif
		
	LCDML_DISP_funcend();
}

void LCDML_DISP_loop_end(LCDML_FUNC_manual) {}

// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_homing_and_reset_position)
// *********************************************************************
{
}

void LCDML_DISP_loop(LCDML_FUNC_homing_and_reset_position)
{
	if(messageBox(PSTR("homing?"), 1)) {
		GrblMaster::homing();
		
		GrblMaster::setMachinePosition(Settings::values().common.maxTravel[(int)Axis::Y], 0);
		GrblMaster::setPosition(Axis::Y, Settings::values().common.maxTravel[(int)Axis::Y]);
		GrblMaster::setPosition(Axis::Z, 0);
	}
	
	if(messageBox(PSTR("goto zero?"), 1)) {
		GrblMaster::gotoPosition(0, 0);
	}
	
	LCDML_DISP_funcend();
}

void LCDML_DISP_loop_end(LCDML_FUNC_homing_and_reset_position) {}

// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_dado)
// *********************************************************************
{
}

void LCDML_DISP_loop(LCDML_FUNC_dado)
{	
	lines[0].setText_P(PSTR("dado width?"));
	if(!scanNumber(Settings::values().dado.width, 0, 100)) {
		return;
	}
	
	lines[0].setText_P(PSTR("dado depth?"));
	if(!scanNumber(Settings::values().dado.cutDepth, 0, Settings::values().common.maxTravel[(int)Axis::Z])) {
		return;
	}
	
	lines[0].setText_P(PSTR("cut depth/pass?"));
	if(!scanNumber(Settings::values().common.cutterMaxCutDepth, 0, Settings::values().common.maxTravel[(int)Axis::Z])) {
		return;
	}
	
	lines[0].setText_P(PSTR("dado position?"));
	if(!scanNumber(Settings::values().dado.position, 0, Settings::values().common.maxTravel[(int)Axis::Y * 2])) { // because we can cut from both sides
		return;
	}
	
	lines[0].setText_P(PSTR("sheet width?"));
	if(!scanNumber(Settings::values().dado.sheetWidth, 0, Settings::values().common.maxTravel[(int)Axis::Y * 2])) {
		return;
	}
	Settings::save();
	
	RouterFenceCam::Dado dados[2] = {0};
	dados[0].position = Settings::values().dado.position; 
	dados[0].width = Settings::values().dado.width; 
	dados[0].depth = Settings::values().dado.cutDepth; 
	dados[0].reversedOrder = false;
	doDados(dados, 1, 2, Settings::values().dado.sheetWidth);
  
	LCDML_DISP_funcend();
}

void LCDML_DISP_loop_end(LCDML_FUNC_dado) {}
	
// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_finger_joint_setup)
// *********************************************************************
{
}

void LCDML_DISP_loop(LCDML_FUNC_finger_joint_setup)
{
	lines[0].setText_P(PSTR("finger count?"));
	if(!scanNumber(Settings::values().fingerJoint.fingerCount, 0, 60)) {
		return;
	}
	
	lines[0].setText_P(PSTR("clearance width?"));
	if(!scanNumber(Settings::values().fingerJoint.clearanceWidth, 0, 10)) {
		return;
	}
	
	lines[0].setText_P(PSTR("sheet width?"));
	if(!scanNumber(Settings::values().fingerJoint.sheetWidth, 0, Settings::values().common.maxTravel[(int)Axis::Y] * 2)) { // because we can cut from both sides
		return;
	}
#ifdef ENABLE_Z_AXIS
	lines[0].setText_P(PSTR("finger length?"));
	if(!scanNumber(Settings::values().fingerJoint.cutDepth, 0, Settings::values().common.maxTravel[(int)Axis::Z])) {
		return;
	}
	
	lines[0].setText_P(PSTR("cut depth/pass?"));
	if(!scanNumber(Settings::values().common.cutterMaxCutDepth, 0, Settings::values().common.maxTravel[(int)Axis::Z])) {
		return;
	}
#else
	// only add dummy values to satisfy the calculation tool
	Settings::values().fingerJoint.cutDepth = Settings::values().common.cutterMaxCutDepth = 1.0;
#endif
	Settings::save();
	
	LCDML_DISP_funcend();
}

void LCDML_DISP_loop_end(LCDML_FUNC_finger_joint_setup) {}
	
// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_finger_joint_male)
// *********************************************************************
{
}
#define MAX_FINGERS 15
void LCDML_DISP_loop(LCDML_FUNC_finger_joint_male)
{
	RouterFenceCam::Dado dados[MAX_FINGERS];
	int dadoCount = 0;
	if (RouterFenceCam::calculateFingerGrooves(true, Settings::values().common.cutterWidth, Settings::values().fingerJoint.sheetWidth,
		Settings::values().fingerJoint.clearanceWidth, Settings::values().fingerJoint.cutDepth, Settings::values().fingerJoint.fingerCount, 
		dados, MAX_FINGERS, dadoCount)) {
		doDados(dados, dadoCount, MAX_FINGERS, Settings::values().fingerJoint.sheetWidth);
	} else {
		lines[0].setText_P(PSTR("err calc fingers"));
		delay(1000);
		LCDML_DISP_funcend();
		return;
	}
}

void LCDML_DISP_loop_end(LCDML_FUNC_finger_joint_male) {}
	
// *********************************************************************
void LCDML_DISP_setup(LCDML_FUNC_finger_joint_female)
// *********************************************************************
{
}

void LCDML_DISP_loop(LCDML_FUNC_finger_joint_female)
{
	RouterFenceCam::Dado dados[MAX_FINGERS];
	int dadoCount = 0;
	if (RouterFenceCam::calculateFingerGrooves(false, Settings::values().common.cutterWidth, Settings::values().fingerJoint.sheetWidth,
		Settings::values().fingerJoint.clearanceWidth, Settings::values().fingerJoint.cutDepth, Settings::values().fingerJoint.fingerCount, dados, MAX_FINGERS, dadoCount)) {
		doDados(dados, dadoCount, MAX_FINGERS, Settings::values().fingerJoint.sheetWidth);
	} else {
		lines[0].setText_P(PSTR("err calc fingers"));
		delay(1000);
		LCDML_DISP_funcend();
		return;
	}
}

void LCDML_DISP_loop_end(LCDML_FUNC_finger_joint_female) {}



