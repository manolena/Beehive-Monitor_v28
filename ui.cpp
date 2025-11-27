#include "ui.h"
#include "config.h"
#include "greek_utils.h"
#include "lcd_server_simple.h"
#include "menu_manager.h"
#include "modem_manager.h"

#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include "safe_freertos.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// LCD instance (address may be 0x27)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// exported globals
SemaphoreHandle_t lcdMutex = NULL;
int currentLanguage = LANG_EN;
QueueHandle_t beepQueue = NULL;

// forward declarations (getButton is implemented below)
static String prepareTextForCol(const String &in, uint8_t col);
static void replaceWebLineSegment(uint8_t row, uint8_t col, const String &seg);
static void writeSegToLcdAndMirror(uint8_t col, uint8_t row, const String &seg);
void initGreekChars();
static void buzzerTask(void *pv);

// ----------------------- helpers -----------------------
static String prepareTextForCol(const String &in, uint8_t col) {
    String s = in;
    if (currentLanguage == LANG_GR) s = greekToUpper(s);
    uint8_t avail = (col < 20) ? (20 - col) : 0;
    if (s.length() > avail) s = s.substring(0, avail);
    while (s.length() < avail) s += ' ';
    return s;
}

static void replaceWebLineSegment(uint8_t row, uint8_t col, const String &seg) {
    String line = lcd_get_line_simple(row);
    while (line.length() < 20) line += ' ';
    uint8_t segLen = seg.length();
    if (col >= 20) return;
    String newLine = line.substring(0, col) + seg;
    if (col + segLen < 20) newLine += line.substring(col + segLen);
    lcd_set_line_simple(row, newLine);
}

// write segment to LCD char-by-char handling degree and mirror
static void writeSegToLcdAndMirror(uint8_t col, uint8_t row, const String &seg) {
  if (col >= 20 || row >= 4) return;
  uint8_t avail = (col < 20) ? (20 - col) : 0;
  if (avail == 0) return;
  String webSeg; webSeg.reserve(avail);

  const char *p = seg.c_str();
  uint8_t printed = 0;

  if (safeSemaphoreTake(lcdMutex, pdMS_TO_TICKS(200), "writeSeg")) {
    lcd.setCursor(col, row);
    while (*p && printed < avail) {
      uint8_t b = (uint8_t)*p;
      // UTF-8 degree (0xC2 0xB0)
      if (b == 0xC2 && (uint8_t)p[1] == 0xB0) {
        // send the pre-baked degree glyph in CGRAM: use lcd.write(223) as required
        lcd.write((uint8_t)223);
        webSeg += (char)0xC2; webSeg += (char)0xB0;
        p += 2; printed++; continue;
      }
      // single-byte degree 0xB0
      if (b == 0xB0) {
        lcd.write((uint8_t)223);
        webSeg += (char)0xC2; webSeg += (char)0xB0;
        p++; printed++; continue;
      }
      // Greek two-byte sequences fallback: handled by lcdPrintGreek when needed
      if (b == 0xCE || b == 0xCF) {
        lcd.write((uint8_t)'?'); webSeg += '?';
        if (p[1]) p += 2; else p++;
        printed++; continue;
      }
      // normal byte
      lcd.write((uint8_t)b);
      webSeg += *p;
      p++; printed++;
    }
    safeSemaphoreGive(lcdMutex, "writeSeg");
  } else {
    // fallback unprotected
    lcd.setCursor(col, row);
    while (*p && printed < avail) {
      uint8_t b = (uint8_t)*p;
      if (b == 0xC2 && (uint8_t)p[1] == 0xB0) {
        lcd.write((uint8_t)223);
        webSeg += (char)0xC2; webSeg += (char)0xB0;
        p += 2; printed++; continue;
      }
      if (b == 0xB0) {
        lcd.write((uint8_t)223);
        webSeg += (char)0xC2; webSeg += (char)0xB0;
        p++; printed++; continue;
      }
      if (b == 0xCE || b == 0xCF) {
        lcd.write((uint8_t)'?'); webSeg += '?';
        if (p[1]) p += 2; else p++;
        printed++; continue;
      }
      lcd.write((uint8_t)b);
      webSeg += *p;
      p++; printed++;
    }
  }

  while (webSeg.length() < avail) webSeg += ' ';
  replaceWebLineSegment(row, col, webSeg);
}

// ----------------------- UI public APIs -----------------------
void ui_safePrintAt(uint8_t col, uint8_t row, const String &seg) {
    if (col >= 20) return;
    String s = prepareTextForCol(seg, col);
    writeSegToLcdAndMirror(col, row, s);
}

static void uiPrint_P_core(uint8_t col, uint8_t row, const __FlashStringHelper *str) {
    String s = String(str);
    if (currentLanguage == LANG_GR) s = greekToUpper(s);
    String seg = prepareTextForCol(s, col);
    ui_safePrintAt(col, row, seg);
}

void uiPrint_P(const __FlashStringHelper *str, uint8_t col, uint8_t row) { uiPrint_P_core(col, row, str); }
void uiPrint_P(uint8_t col, uint8_t row, const __FlashStringHelper *str) { uiPrint_P_core(col, row, str); }

void uiPrint(uint8_t col, uint8_t row, const char *msg) {
    if (col >= 20) return;
    String seg = prepareTextForCol(String(msg), col);
    if (currentLanguage == LANG_GR) {
        lcdPrintGreek(seg.c_str(), col, row);
    } else {
        ui_safePrintAt(col, row, seg);
    }
}

void uiClear() {
    if (safeSemaphoreTake(lcdMutex, portMAX_DELAY, "uiClear")) {
      lcd.clear();
      safeSemaphoreGive(lcdMutex, "uiClear");
    } else {
      lcd.clear();
    }

    for (uint8_t i = 0; i < 4; ++i) {
        lcd_set_line_simple(i, String("                    "));
    }
}

void ui_setMarkerCharAtRow(uint8_t row, char ch) {
    if (safeSemaphoreTake(lcdMutex, portMAX_DELAY, "ui_setMarkerCharAtRow")) {
      lcd.setCursor(0, row);
      lcd.write((uint8_t)ch);
      safeSemaphoreGive(lcdMutex, "ui_setMarkerCharAtRow");
    } else {
      lcd.setCursor(0, row);
      lcd.write((uint8_t)ch);
    }

    String line = lcd_get_line_simple(row);
    while (line.length() < 20) line += ' ';
    line.setCharAt(0, ch);
    lcd_set_line_simple(row, line);
}

// ----------------------- Greek / degree printing -----------------------
// --------------------------------------------------
// GREEK CHAR SYSTEM
// --------------------------------------------------
void initGreekChars() {
  byte Gamma[8]  = { B11111, B10000, B10000, B10000, B10000, B10000, B10000, B00000 };
  byte Delta[8]  = { B00100, B01010, B10001, B10001, B10001, B10001, B11111, B00000 };
  byte Lambda[8] = { B00100, B01010, B10001, B10001, B10001, B10001, B10001, B00000 };
  byte Xi[8]     = { B11111, B00000, B00000, B01110, B00000, B00000, B11111, B00000 };
  byte Pi[8]     = { B11111, B10001, B10001, B10001, B10001, B10001, B10001, B00000 };
  byte Phi[8]    = { B01110, B10101, B10101, B10101, B01110, B00100, B00100, B00000 };
  byte Psi[8]    = { B10101, B10101, B10101, B01110, B00100, B00100, B00100, B00000 };
  byte Omega[8]  = { B01110, B10001, B10001, B10001, B01110, B00000, B11111, B00000 };

  lcd.createChar(0, Gamma);
  lcd.createChar(1, Delta);
  lcd.createChar(2, Lambda);
  lcd.createChar(3, Xi);
  lcd.createChar(4, Pi);
  lcd.createChar(5, Phi);
  lcd.createChar(6, Psi);
  lcd.createChar(7, Omega);
}

void lcdPrintGreek(const char *utf8str, uint8_t col, uint8_t row) {
    lcd.setCursor(col, row);
    const char *p = utf8str;

    while (*p) {
        if ((uint8_t)*p == 0xCE || (uint8_t)*p == 0xCF) {
            uint8_t first = (uint8_t)*p;
            p++;
            uint8_t second = (uint8_t)*p;

            if (first == 0xCE) {
                switch (second) {
                    case 0x91: lcd.write('A'); break;      // Α
                    case 0x92: lcd.write('B'); break;      // Β
                    case 0x93: lcd.write((uint8_t)0); break; // Γ
                    case 0x94: lcd.write((uint8_t)1); break; // Δ
                    case 0x95: lcd.write('E'); break;      // Ε
                    case 0x96: lcd.write('Z'); break;      // Ζ
                    case 0x97: lcd.write('H'); break;      // Η
                    case 0x98: lcd.write(242); break;      // Θ
                    case 0x99: lcd.write('I'); break;      // Ι
                    case 0x9A: lcd.write('K'); break;      // Κ
                    case 0x9B: lcd.write((uint8_t)2); break; // Λ
                    case 0x9C: lcd.write('M'); break;      // Μ
                    case 0x9D: lcd.write('N'); break;      // Ν
                    case 0x9E: lcd.write((uint8_t)3); break; // Ξ
                    case 0x9F: lcd.write('O'); break;      // Ο
                    case 0xA0: lcd.write((uint8_t)4); break; // Π
                    case 0xA1: lcd.write('P'); break;      // Ρ
                    case 0xA3: lcd.write(246); break;      // Σ
                    case 0xA4: lcd.write('T'); break;      // Τ
                    case 0xA5: lcd.write('Y'); break;      // Υ
                    case 0xA6: lcd.write((uint8_t)5); break; // Φ
                    case 0xA7: lcd.write('X'); break;      // Χ
                    case 0xA8: lcd.write((uint8_t)6); break; // Ψ
                    case 0xA9: lcd.write((uint8_t)7); break; // Ω
                    default: lcd.write('?'); break;
                }
            }
        } else {
            lcd.write(*p);
        }
        p++;
    }
}

void lcdPrintGreek_P(const __FlashStringHelper *str, uint8_t col, uint8_t row) {
    String s = String(str);
    lcdPrintGreek(s.c_str(), col, row);
}

// ----------------------- Network indicator -----------------------
void uiUpdateNetworkIndicator() {
    String seg = "    ";
    if (WiFi.status() == WL_CONNECTED) seg = "WIFI";
    else if (modem_isNetworkRegistered()) seg = "LTE ";
    ui_safePrintAt(16, 0, seg);
}

// ----------------------- Buzzer task + init -----------------------
static void buzzerTask(void *pv) {
  BeepReq req;
  for (;;) {
    if (safeQueueReceive(beepQueue, &req, portMAX_DELAY, "buzzerTask") == pdTRUE) {
      for (uint8_t i = 0; i < req.repeat; ++i) {
        beepBuzzer(req.hz, req.ms);
        if (i + 1 < req.repeat) vTaskDelay(pdMS_TO_TICKS(req.gap_ms));
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void beepBuzzer(unsigned long hz, unsigned long ms) {
#ifdef BUZZER_PIN
  if (hz == 0 || ms == 0) return;
  unsigned long us = (750000UL / hz);
  if (us < 50) us = 50;
  unsigned long rep = (ms * 500UL) / us;
  for (unsigned long i = 0; i < rep; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(us);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(us);
  }
#else
  (void)hz; (void)ms;
#endif
}

// ----------------------- Buttons (canonical implementation) -----------------------
Button getButton()
{
    static uint32_t lastTime = 0;
    static bool upLast   = true;
    static bool downLast = true;
    static bool selLast  = true;
    static bool backLast = true;

    uint32_t now = millis();
    if (now - lastTime < 120) return BTN_NONE;
    lastTime = now;

    bool upNow   = digitalRead(BTN_UP);
    bool downNow = digitalRead(BTN_DOWN);
    bool selNow  = digitalRead(BTN_SELECT);
    bool backNow = digitalRead(BTN_BACK);

    if (upLast && !upNow) {
      upLast = upNow;
      if (beepQueue) {
        BeepReq r = { 2000, 70, 1, 40 };
        safeQueueSend(beepQueue, &r, 0, "getButton:UP");
      }
      return BTN_UP_PRESSED;
    }
    if (downLast && !downNow) {
      downLast = downNow;
      if (beepQueue) {
        BeepReq r = { 2000, 70, 1, 40 };
        safeQueueSend(beepQueue, &r, 0, "getButton:DOWN");
      }
      return BTN_DOWN_PRESSED;
    }
    if (selLast && !selNow) {
      selLast = selNow;
      if (beepQueue) {
        BeepReq r = { 2000, 80, 2, 80 };
        safeQueueSend(beepQueue, &r, 0, "getButton:SELECT");
      }
      return BTN_SELECT_PRESSED;
    }
    if (backLast && !backNow) {
      backLast = backNow;
      if (beepQueue) {
        BeepReq r = { 2000, 70, 1, 40 };
        safeQueueSend(beepQueue, &r, 0, "getButton:BACK");
      }
      return BTN_BACK_PRESSED;
    }

    upLast   = upNow;
    downLast = downNow;
    selLast  = selNow;
    backLast = backNow;

    return BTN_NONE;
}

// ----------------------- init, splash -----------------------
void uiInit() {
    if (!lcdMutex) lcdMutex = xSemaphoreCreateMutex();

    if (!beepQueue) beepQueue = xQueueCreate(8, sizeof(BeepReq));
    if (!beepQueue) Serial.println("[UI] beepQueue creation FAILED");

    if (beepQueue) {
      // create buzzer task
      xTaskCreatePinnedToCore(buzzerTask, "buzzerTask", 3072, NULL, 2, NULL, 1);
    }

    pinMode(BTN_UP,     INPUT_PULLUP);
    pinMode(BTN_DOWN,   INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);
    pinMode(BTN_BACK,   INPUT_PULLUP);

#ifdef BUZZER_PIN
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
#endif

    if (safeSemaphoreTake(lcdMutex, portMAX_DELAY, "uiInit")) {
      lcd.init(); lcd.backlight(); lcd.clear();
      safeSemaphoreGive(lcdMutex, "uiInit");
    } else {
      lcd.init(); lcd.backlight(); lcd.clear();
    }

    initGreekChars();

    for (uint8_t i = 0; i < 4; ++i) lcd_set_line_simple(i, String("                    "));
}

void showSplashScreen() {
    uiClear();
    const unsigned long SPLASH_TIMEOUT = 3000UL;
    unsigned long splashStart = millis();

    if (currentLanguage == LANG_EN) {
        uiPrint_P(0, 0, F("===================="));
        uiPrint_P(0, 1, F("  BEEHIVE MONITOR   "));
        uiPrint_P(0, 2, F("        v28         "));
        uiPrint_P(0, 3, F("===================="));
    } else {
        uiPrint_P(0, 0, F("===================="));
        lcdPrintGreek_P(F(" ΠΑΡΑΚΟΛΟΥΘΗΣΗ      "), 0, 1);
        lcdPrintGreek_P(F("  ΚΥΨΕΛΗΣ v28       "), 0, 2);
        uiPrint_P(0, 3, F("===================="));
    }

    while (millis() - splashStart < SPLASH_TIMEOUT) {
        if (getButton() != BTN_NONE) break;
        delay(20);
    }
    uiClear();
}