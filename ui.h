#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// Ensure lcd server line buffer prototypes are visible
#include "lcd_server_simple.h"

// languages
#define LANG_EN 0
#define LANG_GR 1

// Button type and values used across the project
typedef enum {
  BTN_NONE = 0,
  BTN_UP_PRESSED,
  BTN_DOWN_PRESSED,
  BTN_SELECT_PRESSED,
  BTN_BACK_PRESSED
} Button;

// exported globals (defined in ui.cpp)
extern SemaphoreHandle_t lcdMutex;
extern int currentLanguage;
extern QueueHandle_t beepQueue;

// UI API
void uiInit();
void uiPrint(uint8_t col, uint8_t row, const char *msg);
void uiPrint_P(const __FlashStringHelper *str, uint8_t col, uint8_t row);
void uiPrint_P(uint8_t col, uint8_t row, const __FlashStringHelper *str);
void uiClear();
void ui_setMarkerCharAtRow(uint8_t row, char ch);
void ui_safePrintAt(uint8_t col, uint8_t row, const String &seg);
void lcdPrintGreek(const char *utf8str, uint8_t col, uint8_t row);
void lcdPrintGreek_P(const __FlashStringHelper *str, uint8_t col, uint8_t row);
void uiUpdateNetworkIndicator();
void uiRefreshMirror();

// Other helpers used across units
Button getButton();
void showSplashScreen();
void beepBuzzer(unsigned long hz, unsigned long ms);

#endif // UI_H