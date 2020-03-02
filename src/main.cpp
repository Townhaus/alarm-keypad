#include <NTPClient.h>
#include <MQTT.h>
#include <MQTTClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiClient.h>
#include <WiFiServerSecureAxTLS.h>
#include <ESP8266WiFiSTA.h>
#include <WiFiClientSecureAxTLS.h>
#include <ESP8266WiFiType.h>
#include <WiFiServerSecure.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <CertStoreBearSSL.h>
#include <WiFiUdp.h>
#include <BearSSLHelpers.h>
#include <WiFiServer.h>
#include <WiFiServerSecureBearSSL.h>
#include <ESP8266WiFiGeneric.h>
#include <ArduinoJson.h>

#include <Arduino.h>
#include <SPI.h>

#include <Adafruit_ILI9341esp/Adafruit_ILI9341esp.h>
#include <Adafruit_GFX.h>
#include <XPT2046.h>
#include <Wire.h>
#include <secrets.h>

// Modify the following two lines to match your hardware
// Also, update calibration parameters below, as necessary

// For the esp shield, these are the default.
#define TFT_DC 2
#define TFT_CS 15

#define BACKCOLOR 0x0000
#define PRINTCOL 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_YELLOW 0xFFE0
/******************* UI details */
#define BUTTON_X 40
#define BUTTON_Y 120
#define BUTTON_W 76
#define BUTTON_H 40
#define BUTTON_SPACING_X 5
#define BUTTON_SPACING_Y 5
#define BUTTON_TEXTSIZE 2
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// text box where numbers go
#define TEXT_X 10
#define TEXT_Y 15
#define TEXT_W 230
#define TEXT_H 35
#define TEXT_TSIZE 2
#define TEXT_TCOLOR 0xFFFF
#define TEXT_LEN 6

#define ALARM_UI_STATE 0
#define ALARM_UI_STATE_LOADING 1
#define HOME_HA_UI_STATE 2
#define HOME_HA_UI_STATE_LOADING 3
#define HOME_HA_BACK_BTN 14

uint16_t uiState = ALARM_UI_STATE;

char textfield[TEXT_LEN + 1] = "";
char alarmCode[TEXT_LEN + 1] = "";
uint8_t textfield_i = 0;
uint8_t alarmCode_i = 0;

int timeUpdateInterval = 60000;
int alarmUiTimeout = 10000;

unsigned long timeNow = 0;
unsigned long lastAlarmUiKeypress = 0;

char buttonlabels[15][10] = {
		"DIS",
		"AWY",
		"HOME",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"CLR",
		"0",
		char(31)};
uint16_t buttoncolors[15] = {
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR,
		BACKCOLOR};
uint16_t buttonTextSize[15] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
XPT2046 touch(/*cs=*/4, /*irq=*/5);
Adafruit_GFX_Button buttons[15];
Adafruit_GFX_Button shouldShowKeypad;

char exteriorTemp[8];
char foyerTemp[8];
char weatherSummary[15];
char alarmDetails[50];
String alarmStatus = "N/A";

WiFiClient net;
WiFiUDP ntpUDP;
MQTTClient mqttClient;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -28800);

void printTopic(uint8_t col, uint8_t row, String text, uint8_t xOffset = 0, String label = "")
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
			TEXT_Y,
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

void resetTextField()
{
	for (int x = 0; x < TEXT_LEN + 1; x++)
	{
		textfield[x] = ' ';
		alarmCode[x] = ' ';
	}
	textfield_i = 0;
	alarmCode_i = 0;

	Serial.println(textfield);

	printTopic(0, 1, textfield, 0);
}

void printAlarmStatus(uint8_t col, uint8_t row, String status)
{
	Serial.println("printAlarmStatus: " + status);
	uint16_t color = COLOR_GREEN;
	if (status == "armed_away" || status == "armed_home")
	{
		color = COLOR_RED;
	}
	if (status == "pending")
	{
		color = COLOR_YELLOW;
	}
	printTopic(col, row, status, 20, "Alarm Status");
	uint8_t x = (TEXT_X) + col * (TEXT_W);
	uint8_t y = TEXT_Y + row * (TEXT_H);
	tft.fillCircle(x + 5, y + 5, 7, color);
}

void messageReceived(String &topic, String &payload)
{
	Serial.println("incoming: " + topic + " - " + payload);
	Serial.println("uiState: " + String(uiState));
	if (topic == "/alarm/status")
	{
		alarmStatus = payload;
		if (uiState == ALARM_UI_STATE)
		{
			printAlarmStatus(0, 0, alarmStatus);
		}
		if (uiState == HOME_HA_UI_STATE)
		{
			printAlarmStatus(0, 4, alarmStatus);
		}
	}

	if (topic == "/weather/exterior/temperature")
	{
		payload = payload + (char)247;
		payload.toCharArray(exteriorTemp, 25);
		if (uiState == 2)
		{
			printTopic(0, 1, exteriorTemp, 0, "Exterior Temperature");
		}
	}

	if (topic == "/weather/foyer/temperature")
	{
		payload = payload + (char)247;
		payload.toCharArray(foyerTemp, 25);
		if (uiState == 2)
		{
			printTopic(0, 2, foyerTemp, 0, "Foyer Temperature");
		}
	}

	if (topic == "/weather/exterior/summary")
	{
		payload.toCharArray(weatherSummary, 15);
		if (uiState == 2)
		{
			printTopic(0, 3, weatherSummary, 0, "Weather");
		}
	}

	if (topic == "/alarm/details")
	{
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, payload);
		payload.toCharArray(alarmDetails, 15);
		if (error)
		{
			return;
		}
		if (uiState == 2)
		{
			tft.fillRect(
					((TEXT_X) + 0 * (TEXT_W)),
					TEXT_Y + 5 * (TEXT_H),
					TEXT_W,
					70,
					BACKCOLOR);
			for (int i = 0; i <= sizeof(doc); i++)
			{
				const char *sensor = doc[i];
				if (sensor == NULL)
				{
					return;
				}
				printTopic(0, 5 + i, doc[i], 0, "Door");
				Serial.println(sensor);
			}
		}
	}
}

void connect()
{
	Serial.print("connecting wifi...");
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.print(".");
		delay(1000);
	}

	Serial.print("connecting mqtt...");
	while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
	{
		Serial.print(".");
		delay(1000);
	}

	mqttClient.onMessage(messageReceived);
	mqttClient.subscribe("/alarm/status");
	mqttClient.subscribe("/alarm/details");
	mqttClient.subscribe("/weather/exterior/temperature");
	mqttClient.subscribe("/weather/foyer/temperature");
	mqttClient.subscribe("/weather/exterior/summary");
}

void createButton(uint8_t col, uint8_t row)
{
	buttons[col + row * 3].initButton(
			&tft,
			BUTTON_X + col * (BUTTON_W + BUTTON_SPACING_X),
			BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y), // x, y, w, h, outline, fill, text
			BUTTON_W,
			BUTTON_H,
			ILI9341_WHITE,
			buttoncolors[col + row * 3],
			ILI9341_WHITE,
			buttonlabels[col + row * 3],
			buttonTextSize[col + row * 3]);

	buttons[col + row * 3].drawButton();
}

void setup()
{
	Serial.begin(9600);
	SPI.setFrequency(ESP_SPI_FREQ);

	tft.begin();
	touch.begin(tft.width(), tft.height());

	Serial.print("tftx =");
	Serial.print(tft.width());
	Serial.print(" tfty =");
	Serial.println(tft.height());

	tft.setRotation(0);
	tft.fillScreen(BACKCOLOR);

	// Replace these for your screen module
	touch.setCalibration(1858, 265, 288, 1819);

	// create buttons
	for (uint8_t row = 0; row < 5; row++)
	{
		for (uint8_t col = 0; col < 3; col++)
		{
			createButton(col, row);
		}
	}

	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	// Note: Local domain names (e.g. "Computer.local" on OSX) are not supported by Arduino.
	// You need to set the IP address directly.
	mqttClient.begin(MQTT_HOST, net);
	timeClient.begin();
	connect();
}

void loop()
{
	mqttClient.loop();
	delay(10);

	if (!mqttClient.connected())
	{
		connect();
	}

	timeClient.update();

	uint16_t x, y;

	if (millis() > timeNow + timeUpdateInterval)
	{
		timeNow = millis();
		if (uiState == HOME_HA_UI_STATE_LOADING || uiState == HOME_HA_UI_STATE)
		{
			printTopic(0, 0, timeClient.getFormattedTime().substring(0, 5), 0, "Time");
		}
	}

	if (uiState == ALARM_UI_STATE && (millis() > lastAlarmUiKeypress + alarmUiTimeout))
	{
		Serial.println("ALARM UI TIMEOUT");
		lastAlarmUiKeypress = millis();
		uiState = HOME_HA_UI_STATE_LOADING;
	}

	if (uiState == HOME_HA_UI_STATE_LOADING)
	{
		Serial.println("HOME_HA_UI_STATE_LOADING");
		tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ILI9341_BLACK);
		char btnKey[5] = {'A', 'L', 'R', 'M'};
		shouldShowKeypad.initButton(
				&tft,
				BUTTON_X + 2 * (BUTTON_W + BUTTON_SPACING_X),
				BUTTON_Y + 4 * (BUTTON_H + BUTTON_SPACING_Y), // x, y, w, h, outline, fill, text
				BUTTON_W,
				BUTTON_H,
				ILI9341_WHITE,
				buttoncolors[1 + 1 * 3],
				ILI9341_WHITE,
				btnKey,
				2);
		shouldShowKeypad.drawButton();

		printTopic(0, 0, timeClient.getFormattedTime().substring(0, 5), 0, "Time");
		printTopic(0, 1, exteriorTemp, 0, "Exterior Temperature");
		printTopic(0, 2, foyerTemp, 0, "Foyer Temperature");
		printTopic(0, 3, weatherSummary, 0, "Weather");
		printAlarmStatus(0, 4, alarmStatus);
		uiState = HOME_HA_UI_STATE;
	}

	if (uiState == HOME_HA_UI_STATE && touch.isTouching())
	{
		Serial.println("HOME_HA_UI_STATE");
		touch.getPosition(x, y);

		if (shouldShowKeypad.contains(x, y))
		{
			uiState = ALARM_UI_STATE_LOADING;
		}
	}

	if (uiState == ALARM_UI_STATE_LOADING)
	{
		Serial.println("ALARM_UI_STATE_LOADING");

		tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ILI9341_BLACK);
		printAlarmStatus(0, 0, alarmStatus);

		for (uint8_t b = 0; b < 15; b++)
		{
			buttons[b].drawButton(); // draw normal
			buttons[b].press(false);
		}
		uiState = ALARM_UI_STATE;
		lastAlarmUiKeypress = millis();
	}

	if (uiState == ALARM_UI_STATE && touch.isTouching())
	{
		Serial.println("ALARM_UI_STATE");

		lastAlarmUiKeypress = millis();
		touch.getPosition(x, y);

		for (uint8_t b = 0; b < 15; b++)
		{
			if (buttons[b].contains(x, y))
			{
				buttons[b].press(true);
			}
			else
			{
				buttons[b].press(false);
			}
		}

		// now we can ask the buttons if their state has changed
		for (uint8_t b = 0; b < 15; b++)
		{
			if (buttons[b].justReleased())
			{
				// Serial.print("Released: "); Serial.println(b);
				buttons[b].drawButton(); // draw normal
			}

			if (buttons[b].justPressed())
			{
				buttons[b].drawButton(true); // draw invert!

				// if a numberpad button, append the relevant # to the textfield
				if ((b >= 3 || b > 12) && b != 12 && b != 14)
				{
					if (alarmCode_i < TEXT_LEN)
					{
						alarmCode[alarmCode_i] = buttonlabels[b][0];
						alarmCode_i++;
						alarmCode[alarmCode_i] = 0; // zero terminate

						textfield[textfield_i] = '*';
						textfield_i++;
						textfield[textfield_i] = 0; // zero terminate
					}

					Serial.println(textfield);
					printTopic(0, 1, textfield, 0);

					delay(300); // UI debouncing
					buttons[b].press(false);
					buttons[b].drawButton();
				}

				// clr button! delete char
				if (b == 12)
				{
					textfield[textfield_i] = 0;
					if (textfield > 0)
					{
						textfield_i--;
						textfield[textfield_i] = ' ';
					}

					alarmCode[alarmCode_i] = 0;
					if (alarmCode > 0)
					{
						alarmCode_i--;
						alarmCode[alarmCode_i] = ' ';
					}

					Serial.println(textfield);
					printTopic(0, 1, textfield, 0);

					buttons[b].press(false);
					delay(300); // UI debouncing
					buttons[b].drawButton();
				}

				if (b == 2)
				{
					Serial.print("Arming alarm, Code: ");
					Serial.print(alarmCode);
					mqttClient.publish("/alarm/action/arm-home", alarmCode);

					resetTextField();
				}

				if (b == HOME_HA_BACK_BTN)
				{
					delay(300); // UI debouncing

					uiState = HOME_HA_UI_STATE_LOADING;
				}

				if (b == 0)
				{
					Serial.print("Disarming alarm, Code:X");
					Serial.print(alarmCode);
					Serial.print("X");
					mqttClient.publish("/alarm/action/disarm", alarmCode);
					resetTextField();

					buttons[b].press(false);
					delay(300); // UI debouncing
					buttons[b].drawButton();
				}
			}
		}
	}
}
