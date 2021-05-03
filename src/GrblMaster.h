/**
 * static class grbl related commands.
 */

#ifndef GrblMasterH
#define GrblMasterH

#ifndef __GNUC__
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h" 
#else
#include "WProgram.h"
#endif
#else
#include <math.h>
#endif

#include "Config.h"
#include "Logger.h"
#include "Settings.h"

class GrblMaster {
	
private:
	
	struct Parameters {
		double backslash[(int)Axis::MAX_AXIS];
		double max[(int)Axis::MAX_AXIS];
	};
	 
	static Parameters parameters;
	
	// the real position of the machine, buffered and meant to be the same as in the machine.
	// this buffer is used to do soft limits.
	//typedef struct Machine {
	//	double mPos[MAX_AXIS]; //Y: the fence, zero is the position towards the user
	//	                       //Z: the router bit, zero is the router is all the way down
	//	double offset[MAX_AXIS];
	//  // these values are now directly saved in settings.values().position
	
	//static Machine machine;
	
	static double getMPos(Axis axis) {
		return Settings::values().position.position[(int)axis];
	}
	
	static void setMPos(Axis axis, double position, bool save) {
		DEBUGN("set:mPos", position);
		Settings::values().position.position[(int)axis] = position; 
		
		if (save) {
			Settings::save();
		}
	}
	
	static void setUserOffset(Axis axis, double offset, bool save) {
		DEBUGN("offset", offset);
		Settings::values().position.offset[(int)axis] = offset;
		
		if (save) {
			Settings::save();
		}
	}
	
	static double getUserOffset(Axis axis) {
		return Settings::values().position.offset[(int)axis];
	}
	
	static bool checkUserPosRange(Axis axis, double position) {
		double mPos = toMPos(axis, position);
		
		if (mPos < 0 || mPos > getMaxPos(axis)) {
			DEBUGN("range err", mPos);
			DEBUGN("max", getMaxPos(axis));
			return false;
		}	
		
		return true;	
	}
	
	static double getMaxPos(Axis axis) {
		return parameters.max[(int)axis];
	}
	
	// calculate machine pos from user pos
	static double toMPos(Axis axis, double position) {
		return position - getUserOffset(axis);		
	}
	// calculate user position from user position
	static double toUserPos(Axis axis, double position) {
		DEBUGN("mPos", position);
		DEBUGN("offset", getUserOffset(axis)); 
		return position + getUserOffset(axis);
	}
public:
	typedef struct {
		bool idle;
		struct {
			double x;
			double y;
			double z;
		}pos;
	}GrblInfo;
	
	static void init(double backslashY, double backslashZ, double maxY, double maxZ) {
		parameters.backslash[(int)Axis::Y] = backslashY;
		parameters.backslash[(int)Axis::Z] = backslashZ;
		parameters.max[(int)Axis::Y] = maxY;
		parameters.max[(int)Axis::Z] = maxZ;
		
		// restore position on grbl because its not saved there
		setMachinePositionRaw(getMPos(Axis::Y), getMPos(Axis::Z));
	}
	
	static void resetUserOffset(Axis axis) {
		setUserOffset(axis, 0, true);
	}
	
	static boolean homing() {
		return sendCommand("$H", 25000, true, true);	
	}
	
	static double getPositionMin(Axis axis) {
		return toUserPos(axis, 0.0);
	}
	
	static double getPositionMax(Axis axis) {
		return toUserPos(axis, getMaxPos(axis));
	}
		
	static bool gotoPositionJog(Axis axis, double position) {
		
		double mPos = toMPos(axis, position);
		bool cutoff = false;
		if (mPos < 0) {
			mPos = 0;
			cutoff = true;
		} else if (mPos > getMaxPos(axis)) {
			mPos = getMaxPos(axis);
			cutoff = true;
		}
		
		char command[30], text[10];
		dtostrf(mPos, 1, 2, text);
		snprintf_P(command, sizeof(command), PSTR("$J=G90 %s%s F1000"), getAxisString(axis), text);
		
		if(sendCommand(command, 1000, false, false)) {
			setMPos(axis, mPos, false); // don't save here, its to slow, save
			return !cutoff;
		}
		return false;
	}
	
	static bool stopJog() {
		char resetCommand[] = {0x85, 0x00};
		if(sendCommand(resetCommand, 1000, true, true)) {
			GrblInfo info;
			if (!getInfo(info)) {
				return false;
			}
			
			// restore real position for the case we stopped the moving
			setMPos(Axis::Y, info.pos.y, false);
			setMPos(Axis::Z, info.pos.z, true);
		}
		
		return true;
	}
	
	static void savePositionJog() {
		Settings::save();
	}

	static bool gotoPositionRaw(double positionY, double positionZ, bool waitAnswer) {
		
		if(!checkUserPosRange(Axis::Y, positionY)) {
			return false;
		}
		
		if(!checkUserPosRange(Axis::Z, positionZ)) {
			return false;
		}
		
		char command[30], textY[10], textZ[10];
		dtostrf(toMPos(Axis::Y, positionY), 1, 2, textY);
		dtostrf(toMPos(Axis::Z, positionZ), 1, 2, textZ);
		snprintf_P(command, sizeof(command), PSTR("G90 G0 Y%s Z%s"), textY, textZ);
		if(!sendCommand(command, 10000, false, waitAnswer)) {
			return false;
		}
		
		setMPos(Axis::Y, toMPos(Axis::Y, positionY), false);
		setMPos(Axis::Z, toMPos(Axis::Z, positionZ), true);;
		
		if(!waitAnswer) {
			return true;
		}
		
		// wait until position is reached
		int timeout = 40000;
		GrblInfo info;
		while (timeout) {
			timeout -= 500;
			if(!getInfo(info)) {
				continue; //return false;
			}

			if(info.idle) {
				return true;
			}
			
			delay(100);
		}
		
		return false; // timeout
	}
	
	static bool gotoPositionRaw(Axis axis, double position) {
		if(axis == Axis::Y) {
			return gotoPositionRaw(position, getPosition(Axis::Z), true);
		} else {
			return gotoPositionRaw(getPosition(Axis::Y), position, true);	
		}
	}
	
	static bool gotoPosition(double positionY, double positionZ) {
		static double lastPositionY = 0, lastPositionZ = 0;
		
		if(positionY > lastPositionY) {
			double posZ =  positionZ;
			if(lastPositionZ != positionZ) {
				posZ -= parameters.backslash[(int)Axis::Z];
			}
			
			if(!gotoPositionRaw(positionY + parameters.backslash[(int)Axis::Y], posZ, false)) {
				return false;
			}
		}
		
		if(!gotoPositionRaw(positionY, positionZ, true)) {
			return false;
		}
		
		lastPositionY = positionY;
		lastPositionZ = positionZ;
		return true;
	}

	static bool loadPosition_(Axis axis, double& position) {
		GrblInfo info;
		if (!getInfo(info)) {
			return false;
		}
		switch (axis) {
			case Axis::Y: position = info.pos.y; return true;
			case Axis::Z: position = info.pos.z; return true;
			default: return false;
		}
	}
	
	static double getPosition(Axis axis) {
		return toUserPos(axis, getMPos(axis));
	}
		
	static void setPosition(Axis axis, double position) {
		DEBUGN("new pos", position);
		// calc offset
		double offset = position - getMPos(axis);
		
		// store offset for user position
		setUserOffset(axis, offset, true);
	}
	
	static bool probe(Axis axis, int distance, int feedrate, int timeout) {
		char command[30];
		memset(command, 0, sizeof(command));
		snprintf_P(command, sizeof(command), PSTR("G38.2 Y%i F%i"), distance, feedrate);
		return sendCommand(command, timeout, true, true);
	}
	
	static bool getInfo(GrblInfo& info) {
		
		DEBUGP("getInfo");

		// empty rx buffer
		while(Serial.read() != -1) {
		}
			
		Serial.setTimeout(1000);
		Serial.print("?");
		Serial.print("\r\n");
			
#ifdef GRBL_DEBUG			
		return true;
#endif
			
		char buffer[100];
		int ret = 1;
		memset(buffer, 0, sizeof(buffer));
		ret = Serial.readBytesUntil('\n', buffer, sizeof(buffer));
		DEBUG(buffer);

		if(ret > 0)
		{
			// cut the \r\n
			buffer[strlen(buffer)-1] = '\0';
				
			if(parseGrblInfoMessage(buffer, &info)) {
				DEBUGP("Ok");
				return true;
			} else {
				DEBUGP("Wrong answer");
				return false;
			}
		} else {
			DEBUGP("No answer");
			return false;
		}	
	}
	
	static bool reset() {
		
		sendCommand("$X", 1000, false, true);
		
		char resetCommand[] = {0x18, 0x00};
		sendCommand(resetCommand, 3000, true, true);
		delay(1000);
		
		return sendCommand("$X", 1000, false, true);
	}
	
	static bool spindleOn() {
		return sendCommand("M03", 3000, false, true);
	}
	
	static bool spindleOff() {
		return sendCommand("M05", 3000, false, true);
	}
	
	static bool setMachinePosition(double positionY, double positionZ) {
		if(setMachinePositionRaw(positionY, positionZ)) {
			setMPos(Axis::Y, positionY, false);
			setMPos(Axis::Z, positionZ, true);
			return true;
		}
		return false;
	}
		
	private:
	static bool setMachinePositionRaw(double positionY, double positionZ) {
		struct Helper {
			 bool setMachinePosition(double positionY, double positionZ, int maxCount) {
				char command[30], textY[10], textZ[10];
				memset(command, 0, sizeof(command));
				dtostrf(positionY, 1, 2, textY);
				dtostrf(positionZ, 1, 2, textZ);
				snprintf_P(command, sizeof(command), PSTR("G10 P0 L20 Y%s Z%s"), textY, textZ);
				if(sendCommand(command, 1000, false, true)) {
					GrblInfo info;
					if(!getInfo(info)) {
						return false; // communication error
					} 
					return true;
					
					/*if(info.pos.y != positionY || info.pos.z != positionZ) {
						DEBUGP("set pos error");
						if(maxCount>0) {
							return setMachinePosition(positionY, positionZ, maxCount--); //try again, recursive call	
						} else {
							return false;
						}
					} else {
						return true; // position is the same	
					}*/
				} else {
					return false; // send position failed
				}
			 }			 
		} helper;
		
		int maxCount = 3;
		return helper.setMachinePosition(positionY, positionZ, maxCount--);
	}
	
	static bool sendCommand(const char* command, int timeout, bool waitForSecondAnswer, bool waitAnswer) {
		
		DEBUG(command);
		
		// empty rx buffer
		while(Serial.read() != -1) {
		}
		
		Serial.setTimeout(timeout);
		Serial.print(command);
		Serial.print("\r\n");
		
		if (!waitAnswer) {
			return true;
		}
		
#ifdef GRBL_DEBUG
		DEBUGP("Done simu call");
		return true;
#endif
		
		char buffer[64];
		int ret = 1;
		if(waitForSecondAnswer) {
			memset(buffer, 0, sizeof(buffer));
			ret = Serial.readBytesUntil('\n', buffer, sizeof(buffer));
		}
		if (ret > 0) {
			memset(buffer, 0, sizeof(buffer));
			ret = Serial.readBytesUntil('\n', buffer, sizeof(buffer));
		}
		if(ret > 0)
		{
			// cut the \r\n
			buffer[strlen(buffer)-1] = '\0';
			
			if(strcmp(buffer, "ok") == 0) {
				DEBUG("Done");
				return true;
			} else {
				DEBUGP("Wrong answer");
				DEBUG(buffer);
			}
		} else {
			DEBUGP("No answer");
		}

		return false;
	}

	/**
	 * parse the grbl info message result
	 * @param infoMessageAnswer format:  <Idle|WPos:5.000,5.000,10.000...ignore the rest
	 * @param info the resulting info data
	 * @return true if parsing was possible
	 */
	static bool parseGrblInfoMessage(char* infoMessageAnswer, GrblInfo* info) {
		DEBUG(infoMessageAnswer);
		
		char* token = strtok(infoMessageAnswer, "<,:|");

		int index = 0;
		while(token != NULL) {
			double* tokenNumberBuffer = NULL;

			switch (index) {
			case 0:
				info->idle = false;
				if (strcmp(token, "Idle") == 0) {
					info->idle = true;	
				}
				break;
				
			case 1:
				if (strcmp(token, "WPos") != 0) {
					return false;
				}
				break;

			case 2:
				tokenNumberBuffer = &info->pos.x;
				break;

			case 3:
				tokenNumberBuffer = &info->pos.y;
				break;

			case 4:
				tokenNumberBuffer = &info->pos.z;
				break;
				
			default: break;
			}
			
			//DEBUGP("token");
			//DEBUG(token);
				
			if (tokenNumberBuffer != NULL) {
				*tokenNumberBuffer = atof(token);
			}

			token = strtok(NULL, "<,:>|");
			index++;
		}

		return true;
	}
	
public:
	static const char* getAxisString(Axis axis) {
		const char* axisString[] = {"Y", "Z"};
		return axisString[(int)axis];
	}

};

#endif