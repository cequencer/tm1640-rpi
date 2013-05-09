// src/libtm1640/tm1640.c - Main interface code for TM1640
// Copyright 2013 FuryFire
// Copyright 2013 Michael Farrell <http://micolous.id.au/>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "tm1640.h"
#include "font.h"


char tm1640_invertVertical(char input) {
	return (input & 0xC0) |
		// swap top and bottom
		((input & 0x01) << 3) |
		((input & 0x08) >> 3) |
		// swap lefts
		((input & 0x02) << 1) |
		((input & 0x04) >> 1) |
		// swap rights
		((input & 0x10) << 1) |
		((input & 0x20) >> 1);
}


int tm1640_displayWrite(tm1640_display* display, int offset, const char * string, char length, int invertMode) {
	//If we can actually print this
	if(offset + length <= 16) {
		char buffer[30];
		memset(buffer, 0, sizeof(buffer));
		
		int c;
		
		// translate input to segments
		// TODO: provide function to allow raw writing of segments
		for (c=0; c<length; c++) {
			buffer[c] = tm1640_ascii_to_7segment(string[c]);
			
			switch (invertMode) {
				case INVERT_MODE_NONE:
					// do nothing
					break;
					
				case INVERT_MODE_VERTICAL:
					buffer[c] = tm1640_invertVertical(buffer[c]);
					break;
				
				default:
					return -EINVAL;
			}
		}
		tm1640_sendCmd(display, 0x44 );
		tm1640_send(display, 0xC0 + offset, buffer, c);
		
		return 0;
	} else {
		return -EINVAL;
	}
}


char tm1640_ascii_to_7segment(char ascii) {
	if (ascii < FONT_FIRST_CHAR || ascii > FONT_LAST_CHAR) {
		// character than is not in font, skip.
		return 0;
	}
	
	return DEFAULT_FONT[ascii - FONT_FIRST_CHAR];
}


void tm1640_displayClear(tm1640_display* display)
{
	char buffer[16];
	memset( buffer, 0x00, 16 );
	tm1640_send(display, 0xC0, buffer, 16 );
}

void tm1640_displayOn(tm1640_display* display, char brightness) {
	if (brightness < 1) brightness = 1;
	if (brightness > 7) brightness = 7;
	
	tm1640_sendCmd(display, 0x88 + brightness);
}


void tm1640_displayOff(tm1640_display* display) {
	tm1640_sendCmd(display, 0x80);
}


tm1640_display* tm1640_init(int clockPin, int dataPin)
{
	if(wiringPiSetup( ) == -1) 
	{
		return NULL;
	}
	digitalWrite(clockPin, HIGH );
	digitalWrite(dataPin, HIGH );

	pinMode(clockPin, OUTPUT );
	pinMode(dataPin, OUTPUT );
	
	tm1640_display* display = malloc(sizeof(tm1640_display));
	
	// clear for good measure
	memset(display, 0, sizeof(tm1640_display));
	
	display->clockPin = clockPin;
	display->dataPin = dataPin;

	return display;

}


void tm1640_destroy(tm1640_display* display) {
	free(display);
}


void tm1640_sendRaw(tm1640_display* display, char out )
{
	int i;
	for(i = 0; i < 8; i++)
	{
		digitalWrite( display->dataPin, out & (1 << i) );
		digitalWrite( display->clockPin, HIGH );
		delayMicroseconds( 1 );
		digitalWrite( display->clockPin, LOW );
	}
}

void tm1640_send(tm1640_display* display, char cmd, char * data, int len )
{
	//Issue start command
	digitalWrite( display->dataPin, LOW );
	delayMicroseconds( 1 );
	digitalWrite( display->clockPin, LOW );

	tm1640_sendRaw(display, cmd);
	if(data != NULL)
	{
		int i;
		for(i = 0; i < len; i++)
		{
			tm1640_sendRaw(display, data[i] );
		}
	}
	//Issue stop command
	digitalWrite( display->clockPin, HIGH );
	delayMicroseconds( 1 );
	digitalWrite( display->dataPin, HIGH );
}


void tm1640_sendCmd(tm1640_display* display, char cmd )
{
	tm1640_send(display, 0x40, 0 ,0 );
	tm1640_send(display, cmd, 0, 0 );

	digitalWrite(display->dataPin, LOW );
	digitalWrite( display->clockPin, LOW );
	digitalWrite( display->clockPin, HIGH );
	digitalWrite( display->dataPin, HIGH );
}
