// lcd_server_simple.cpp
// Simple mirror layer for LCD lines (mirror debug prints removed)
//
// Provides:
//   String lcd_get_line_simple(uint8_t row);
//   void lcd_set_line_simple(uint8_t row, const String &line);
//
// No other structural changes were made.

#include "lcd_server_simple.h"
#include <Arduino.h>

static String mirror_lines[4];

String lcd_get_line_simple(uint8_t row) {
  if (row >= 4) return String();
  // Ensure length 20
  String s = mirror_lines[row];
  while (s.length() < 20) s += ' ';
  if (s.length() > 20) s = s.substring(0, 20);
  return s;
}

void lcd_set_line_simple(uint8_t row, const String &line) {
  if (row >= 4) return;
  // Allow up to 60 bytes for UTF-8 (20 chars * 3 bytes max)
  String s = line;
  if (s.length() > 60) s = s.substring(0, 60);
  // Do not pad with spaces here, trust the caller (menu_manager) to pad visually
  mirror_lines[row] = s;
  // Intentionally no Serial prints here - mirror debug output removed
}