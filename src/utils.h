/*
  Morse.h - Library for flashing Morse code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/
#ifndef utils
#define utils

#include <Adafruit_ILI9341esp/Adafruit_ILI9341esp.h>
#include <constants.h>
#include <utils.h>

class Utils
{
public:
	static void printTopic(Adafruit_ILI9341 &tft, uint8_t col, uint8_t row, String text, uint8_t xOffset, String label);
	static void initButtons(char buttonlabels[15][10], Adafruit_GFX_Button buttons[15], Adafruit_ILI9341 &tft, uint8_t col, uint8_t row);
};

#endif
