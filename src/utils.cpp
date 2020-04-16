#include <utils.h>

void Utils::printTopic(Adafruit_ILI9341 &tft, uint8_t col, uint8_t row, String text, uint8_t xOffset = 0, String label = "")
{
	char textChar[30];
	Serial.print(col);
	Serial.print(",");
	Serial.print(row);
	Serial.print(" - ");
	Serial.println(text);

	tft.fillRect(
			((TEXT_X) + col * (TEXT_W)) + xOffset,
			TEXT_Y + row * (TEXT_H),
			TEXT_W,
			TEXT_Y + 2,
			BACKCOLOR);
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(
			((TEXT_X) + col * (TEXT_W)) + xOffset,
			TEXT_Y + row * (TEXT_H));

	tft.setTextSize(TEXT_TSIZE);
	text.toCharArray(textChar, 30);
	tft.print(textChar);

	tft.setCursor(
			((TEXT_X) + col * (TEXT_W)),
			0 + row * (TEXT_H));
	tft.setTextSize(1);
	label.toCharArray(textChar, 30);
	tft.print(textChar);
}

void Utils::initButtons(char buttonlabels[15][10] , Adafruit_GFX_Button buttons[15], Adafruit_ILI9341 &tft, uint8_t col, uint8_t row)
{
	buttons[col + row * 3].initButton(
			&tft,
			BUTTON_X + col * (BUTTON_W + BUTTON_SPACING_X),
			BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y), // x, y, w, h, outline, fill, text
			BUTTON_W,
			BUTTON_H,
			ILI9341_WHITE,
			BACKCOLOR,
			ILI9341_WHITE,
			buttonlabels[col + row * 3],
			2);

	buttons[col + row * 3].drawButton();
}
