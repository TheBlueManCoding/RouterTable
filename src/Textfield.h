/**
 * Textfield implementation for lcd display using the global "lcd"
 */
#ifndef TextfieldH
#define TextfieldH

#include <LiquidCrystal_I2C.h>
extern LiquidCrystal_I2C lcd;

template <int maxLength>
class Textfield {
	int posX;
	int posY;
	char text[maxLength+1];
	int length;
	bool clearNextAppend;
	
	void update() {
		lcd.setCursor(posX, posY);
		lcd.print(text);
		
		for(int i=length; i<maxLength; i++) {
			lcd.print(" ");
		}
	}
	
	public:
	Textfield(int posX, int posY) : posX(posX), posY(posY), length(0), clearNextAppend(false) {
		//clear();
	}
	
	void append(char data) {
		if (clearNextAppend) {
			clearNextAppend=false;
			clear();
		}
		if (length < maxLength - 1) {
			text[length++] = data;
			text[length] = '\0';
		}
		update();
	}
	
	void clearOnAppend() {
		clearNextAppend = true;
	}
	
	void clear() {
		length = 0;
		memset(text, 0, maxLength);
		update();
	}
	
	void setText(char* text) {
		strncpy(this->text, text, maxLength);
		length = strlen(this->text);
		this->text[length] = '\0';
		update();
	}
	
	void appendText(char* text) {
		strncpy(this->text + strlen(this->text), text, maxLength - strlen(this->text));
		length = strlen(this->text);
		this->text[length] = '\0';
		update();
	}
	
	void setText_P(PROGMEM const char * text) {
		strncpy_P(this->text, text, maxLength);
		length = strlen(this->text);
		this->text[length] = '\0';
		update();
	}
	
	void setTemporaryText_P(PROGMEM const char * text) {
		lcd.setCursor(posX, posY);
		
		int len = strlen_P(text);
		for(int i=0; i<len; i++) {
			lcd.print((char)pgm_read_byte(&text[i]));
		}

		for(int i=length; i<maxLength; i++) {
			lcd.print(" ");
		}
	}
	
	void restore() {
		update();
	}
	
	void appendText_P(PROGMEM const char * text) {
		strncpy_P(this->text + strlen(this->text), text, maxLength - strlen(this->text));
		length = strlen(this->text);
		this->text[length] = '\0';
		update();
	}
	
	char* getText() {
		return text;
	}
	
	void setNumber(double number) {
		char buffer[maxLength];
		dtostrf(number, 1, 2, buffer);
		setText(buffer);
	}
	
	void appendNumber(double number) {
		char buffer[maxLength];
		dtostrf(number, 1, 2, buffer);
		appendText(buffer);
	}
};

#endif