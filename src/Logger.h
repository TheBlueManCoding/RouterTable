// Logger.h

#ifndef _LOGGER_h
#define _LOGGER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "config.h"

#ifdef DEBUG_BUILD
void loggerInit();

void _debugP(PROGMEM const char * text);
void _debugN(PROGMEM const char * text, double number);
void _debug(char * text);
#define DEBUGP(message) _debugP(PSTR(message));
#define DEBUGN(message,number) _debugN(PSTR(message), number);
#define DEBUG(message) _debug(message);

#else
#define DEBUGP(message)
#define DEBUGN(message,number)
#define DEBUG(message)
#endif

#endif

