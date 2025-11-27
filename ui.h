#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <WebServer.h>

// Language constants
#define LANG_EN 0
#define LANG_GR 1

// Button type used across the UI/menu code
typedef enum {
  BTN_NONE = 0,
  BTN_UP_PRESSED,
  BTN_DOWN_PRESSED,
  BTN_SELECT_PRESSED,
  BTN_BACK_PRESSED
} Button;

// Beep request structure used by button/beeper tasks
typedef struct {
  uint16_t hz;
  uint16_t ms;
  uint8_t  repeat;
  uint16_t gap_ms;
} BeepReq;

// Shared LCD mutex (defined in ui.cpp)
extern SemaphoreHandle_t lcdMutex;

// Global queue used for beeper requests (defined in ui.cpp)
extern QueueHandle_t beepQueue;

// Current language (defined in ui.cpp)
extern int currentLanguage;

// UI / LCD API
void uiInit();
void showSplashScreen();

void uiPrint_P(const __FlashStringHelper *str, uint8_t col, uint8_t row);
void uiPrint_P(uint8_t col, uint8_t row, const __FlashStringHelper *str);
void uiPrint(uint8_t col, uint8_t row, const char *msg);

void ui_safePrintAt(uint8_t col, uint8_t row, const String &seg);
void uiClear();
void ui_setMarkerCharAtRow(uint8_t row, char ch);

void uiUpdateNetworkIndicator();

// Greek / UTF8 helpers
void lcdPrintGreek(const char *utf8str, uint8_t col, uint8_t row);
void lcdPrintGreek_P(const __FlashStringHelper *str, uint8_t col, uint8_t row);

// Button input (implemented in ui_buttons.cpp)
Button getButton();

// Buzzer helper (implemented in ui.cpp)
void beepBuzzer(unsigned long hz, unsigned long ms);

// Mirror helpers (declared here for modules that use them)
void lcd_set_line_simple(uint8_t idx, const String &text);
String lcd_get_line_simple(uint8_t idx);
uint32_t lcd_get_mirror_version();

// Provisioning registration (forward declare)
void provisioning_register(WebServer &server);

#endif // UI_H