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
#define ALARM_UI_DISARM_BTN 0
#define ALARM_UI_ARM_HOME_BTN 2
