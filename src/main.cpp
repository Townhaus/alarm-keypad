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
#include <constants.h>
#include <utils.h>

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
NTPClient timeClient(ntpUDP, "pool.ntp.org", -25200);

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

	Utils::printTopic(tft, 0, 1, textfield, 0, "");
}

void printAlarmStatus(uint8_t col, uint8_t row, String status)
{
	Serial.println("printAlarmStatus: " + status);
	uint16_t color = COLOR_GREEN;
	if (status == "armed away" || status == "armed home")
	{
		color = COLOR_RED;
	}
	if (status == "pending")
	{
		color = COLOR_YELLOW;
	}
	Utils::printTopic(tft, col, row, status, 20, "Alarm Status");
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
		if (uiState == HOME_HA_UI_STATE)
		{
			Utils::printTopic(tft, 0, 1, exteriorTemp, 0, "Exterior Temperature");
		}
	}

	if (topic == "/weather/foyer/temperature")
	{
		payload = payload + (char)247;
		payload.toCharArray(foyerTemp, 25);
		if (uiState == HOME_HA_UI_STATE)
		{
			Utils::printTopic(tft, 0, 2, foyerTemp, 0, "Foyer Temperature");
		}
	}

	if (topic == "/weather/exterior/summary")
	{
		payload.toCharArray(weatherSummary, 15);
		if (uiState == HOME_HA_UI_STATE)
		{
			Utils::printTopic(tft, 0, 3, weatherSummary, 0, "Weather");
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

		tft.fillRect(
				0,
				TEXT_Y + 5 * (TEXT_H),
				240,
				200,
				BACKCOLOR);

		if (uiState == 2)
		{
			for (int i = 0; i <= sizeof(doc); i++)
			{
				const char *sensor = doc[i];
				if (sensor == NULL)
				{
					return;
				}
				Utils::printTopic(tft, 0, 5 + i, doc[i], 0, "Door");
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

void setup()
{
	Serial.begin(9600);

	tft.begin();
	Serial.print(tft.width());
	Serial.println(tft.height());

	tft.setRotation(0);
	tft.fillScreen(BACKCOLOR);
	touch.begin(tft.width(), tft.height());
	// Replace these for your screen module
	touch.setCalibration(1858, 265, 288, 1819);

	for (uint8_t row = 0; row < 5; row++)
	{
		for (uint8_t col = 0; col < 3; col++)
		{
			Utils::initButtons(buttonlabels, buttons, tft, col, row);
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
			Utils::printTopic(tft, 0, 0, timeClient.getFormattedTime().substring(0, 5), 0, "Time");
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
		mqttClient.publish("/alarm/get-status");
		Serial.println("HOME_HA_UI_STATE_LOADING");
		tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ILI9341_BLACK);
		char btnKey[5] = {'A', 'L', 'R', 'M'};
		shouldShowKeypad.initButton(
				&tft,
				0,
				0, // x, y, w, h, outline, fill, text
				240,
				320,
				BACKCOLOR,
				BACKCOLOR,
				BACKCOLOR,
				btnKey,
				2);
		shouldShowKeypad.drawButton();

		Utils::printTopic(tft, 0, 0, timeClient.getFormattedTime().substring(0, 5), 0, "Time");
		Utils::printTopic(tft, 0, 1, exteriorTemp, 0, "Exterior Temperature");
		Utils::printTopic(tft, 0, 2, foyerTemp, 0, "Foyer Temperature");
		Utils::printTopic(tft, 0, 3, weatherSummary, 0, "Weather");
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
					Utils::printTopic(tft, 0, 1, textfield, 0, "");

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
					Utils::printTopic(tft, 0, 1, textfield, 0, "");

					buttons[b].press(false);
					delay(300); // UI debouncing
					buttons[b].drawButton();
				}

				if (b == ALARM_UI_ARM_HOME_BTN)
				{
					mqttClient.publish("/alarm/action/arm-home", alarmCode);
					resetTextField();
				}

				if (b == HOME_HA_BACK_BTN)
				{
					delay(300); // UI debouncing

					uiState = HOME_HA_UI_STATE_LOADING;
				}

				if (b == ALARM_UI_DISARM_BTN)
				{
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
