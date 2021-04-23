// 
// 
// 

#include "Logger.h"
#ifdef DEBUG_BUILD
#include <SoftwareSerial.h>

SoftwareSerial serial(A1, A0);
void loggerInit() {
	serial.begin(115200);	
}
void _debugP(PROGMEM const char * text) {
	while(pgm_read_byte(text) != 0x00) {
		serial.print((char)pgm_read_byte(text++));
	}
	serial.println("");
}

void _debugN(PROGMEM const char * text, double number) {
	while(pgm_read_byte(text) != 0x00) {
		serial.print((char)pgm_read_byte(text++));
	}
	serial.print("=");
	char numb[10];
	dtostrf(number, 1, 2, numb);
	serial.println(number);
}

void _debug(char * text) {
	serial.println(text);
}
#endif
