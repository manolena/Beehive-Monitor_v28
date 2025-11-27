#ifndef LCD_SERVER_SIMPLE_H
#define LCD_SERVER_SIMPLE_H

#include <Arduino.h>

// Lightweight web-mirror buffer API used by UI code.
// Implementation stores 4 lines of 20 chars each in RAM and exposes simple getters/setters.

void lcd_server_startTask(); // start background task (optional)
void lcd_server_stopTask();

void lcd_set_line_simple(uint8_t idx, const String &text);
String lcd_get_line_simple(uint8_t idx);

#endif // LCD_SERVER_SIMPLE_H