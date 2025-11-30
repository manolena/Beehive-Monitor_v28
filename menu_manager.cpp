// menu_manager.cpp
// Full, clean menu manager v28
// - Full-line LCD overwrites to avoid leftover characters
// - Mirror (web) exact 20-char lines: marker col0, blank col1, labels cols2..15, network right-anchored on row 0
// - Menu list rendering identical for main menu and submenus (marker col0, labels at col1 on LCD)
// - Submenu/full-screen pages write from col 0 (as requested)

#include "menu_manager.h"
#include "ui.h"
#include "text_strings.h"
#include "config.h"
#include "time_manager.h"
#include "modem_manager.h"
#include "weather_manager.h"
#include "provisioning_ui.h"
#include "sms_handler.h"
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Preferences.h>
#include <math.h>

#include "calibration.h"
#include "network_manager.h"
#include "sensors.h"
#include "sd_logger.h"

extern LiquidCrystal_I2C lcd;
// sd_present declared in top-level sketch
extern bool sd_present;
// WebServer for handling client requests during blocking loops
#include <WebServer.h>
extern WebServer server;

// -------------------- Menu item storage --------------------------------
static MenuItem root;
static MenuItem* currentItem = nullptr;

static MenuItem m_status;
static MenuItem m_time;
static MenuItem m_measure;
static MenuItem m_weather;
static MenuItem m_connectivity;
static MenuItem m_data_sending; // New
static MenuItem m_provision;
static MenuItem m_calibration;
static MenuItem m_language;
static MenuItem m_sdinfo;
static MenuItem m_back;

static MenuItem cal_root;
static MenuItem m_cal_tare;
static MenuItem m_cal_cal;
static MenuItem m_cal_raw;
static MenuItem m_cal_save;
static MenuItem m_cal_back;

// incremental mirror state
static MenuItem* currentListArr[32];
static int currentMenuCount = 0;
static int displayedScroll = 0;
static int displayedSelectedIndex = 0;
static MenuItem* displayedListStart = nullptr;

// Forward declarations (menu screens)
void menuShowStatus();
void menuShowTime();
void menuShowMeasurements();
void menuShowCalibration();
void menuShowSDInfo();
void menuSetLanguage();
void menuCalTare();
void menuCalCalibrate();
void menuCalRaw();
void menuCalSave();
void menuShowConnectivity();
void menuShowWeather();
void menuShowProvision();
void menuShowDataSending();

// -------------------- Helpers: padding / writes ------------------

// Pad/truncate by bytes to exact length (useful for mirror which stores UTF-8)
// Helper: previously used for byte-padding, now pass-through.
// Truncation/padding is handled by uiPrint (visual chars).
static String padRightBytes(const String &s, size_t len) {
  (void)len;
  return s;
}

// Overwrite columns [col..19] on the physical LCD row with text (text padded/trunc to fit).
// Uses uiPrint which will call lcdPrintGreek when needed.
// Overwrite columns [col..19] on the physical LCD row with text (text padded/trunc to fit).
// Uses uiPrint which will call lcdPrintGreek when needed.
// Also explicitly updates the web mirror to ensure synchronization.
static void writeColsOverwrite(uint8_t col, uint8_t row, const String &text) {
  if (col >= 20 || row >= 4) return;
  uint8_t avail = 20 - col;
  String seg = padRightBytes(text, avail);
  uiPrint(col, row, seg.c_str());

  // Explicitly update mirror (workaround for potential ui.cpp linkage issues)
  String line = lcd_get_line_simple(row);
  while (line.length() < 20) line += ' ';
  String newLine = line.substring(0, col) + seg;
  if (col + seg.length() < 20) newLine += line.substring(col + seg.length());
  lcd_set_line_simple(row, newLine);
}

// Overwrite the full physical row (cols 0..19) with provided text (padded/trunc).
static void writeFullRow(uint8_t row, const String &line20) {
  if (row >= 4) return;
  String s = padRightBytes(line20, 20);
  uiPrint(0, row, s.c_str());
  lcd_set_line_simple(row, s); // Explicit mirror update
}

// Helper to clear screen and mirror
static void menuClear() {
  uiClear();
  for (int i=0; i<4; i++) lcd_set_line_simple(i, "                    ");
}

// -------------------- Mirror helpers ----------------------------------

// Build exact 20-char mirror line for a menu index at given mirror row.
// Mirror layout:
// [0] marker '>' or ' '
// [1] single blank
// [2..15] label bytes (max 14 bytes) left-aligned
// [16..19] network indicator on row 0 anchored to right, else spaces
static String buildMirrorLineForIndex(int idx, int row) {
  String line; line.reserve(20);
  for (int i = 0; i < 20; ++i) line += ' ';

  // marker at col0
  if (idx >= 0 && idx < currentMenuCount && currentListArr[idx]) {
    char m = (idx == displayedSelectedIndex) ? '>' : ' ';
    line.setCharAt(0, m);
  } else {
    line.setCharAt(0, ' ');
  }
  // col1 left as space

  // label into cols 2..15 (14 bytes)
  if (idx >= 0 && idx < currentMenuCount && currentListArr[idx]) {
    TextId id = currentListArr[idx]->text;
    const char* label_c = (currentLanguage == LANG_EN) ? getTextEN(id) : getTextGR(id);
    String label = String(label_c);
    // truncate/pad to 14 bytes (best-effort; Greek are multi-byte but mirror expects UTF-8 bytes)
    if (label.length() > 14) label = label.substring(0, 14);
    for (int i = 0; i < 14; ++i) {
      char ch = (i < (int)label.length()) ? label.charAt(i) : ' ';
      line.setCharAt(2 + i, ch);
    }
  } else {
    for (int i = 16; i < 20; ++i) line.setCharAt(i, ' ');
  }

  return line;
}

// Write all 4 mirror rows for current window
static void updateMirrorFromState() {
  for (int row = 0; row < 4; ++row) {
    int idx = displayedScroll + row;
    String m = buildMirrorLineForIndex(idx, row);
    while (m.length() < 20) m += ' ';
    if (m.length() > 20) m = m.substring(0,20);
    lcd_set_line_simple(row, m);
  }
}

// -------------------- Render menu (physical + mirror) ------------------

// Render the menu list: physical marker col0, labels at col1 (fully overwritten), mirror per-line exact.
static void renderFullMenu(MenuItem* list[], int MENU_COUNT, int selectedIndex, int scroll) {
  for (int r = 0; r < 4; ++r) {
    int idx = scroll + r;
    if (idx >= MENU_COUNT) {
      ui_setMarkerCharAtRow(r, ' ');
      writeColsOverwrite(1, r, String("")); // clears cols1..19
    } else {
      TextId id = list[idx]->text;
      const char* label_c = (currentLanguage == LANG_EN) ? getTextEN(id) : getTextGR(id);
      String label = String(label_c);
      // On row 0, limit label to cols 1-15 (15 chars) to leave space for network indicator (cols 16-19)
      // On other rows, use full width cols 1-19 (19 chars)
      int labelWidth = (r == 0) ? 15 : 19;
      String paddedLabel = padRightBytes(label, labelWidth);
      char marker = (idx == selectedIndex) ? '>' : ' ';
      ui_setMarkerCharAtRow(r, marker);
      writeColsOverwrite(1, r, paddedLabel);
    }
    // mirror line
    String mirrorLine = buildMirrorLineForIndex(idx, r);
    while (mirrorLine.length() < 20) mirrorLine += ' ';
    lcd_set_line_simple(r, mirrorLine);
  }
  // Update network indicator on row 0
  uiUpdateNetworkIndicator();
  // ensure mirror network indicator correct
  updateMirrorFromState();
}

void menuInit() {
  m_status       = { TXT_STATUS,       menuShowStatus,       &m_time,        nullptr,       &root,     nullptr };
  m_time         = { TXT_TIME,         menuShowTime,         &m_measure,     &m_status,     &root,     nullptr };
  m_measure      = { TXT_MEASUREMENTS, menuShowMeasurements, &m_weather,     &m_time,       &root,     nullptr };
  m_weather      = { TXT_WEATHER,      menuShowWeather,      &m_connectivity,&m_measure,    &root,     nullptr };
  m_connectivity = { TXT_CONNECTIVITY, menuShowConnectivity, &m_data_sending,&m_weather,    &root,     nullptr };
  m_data_sending = { TXT_DATA_SENDING, menuShowDataSending,  &m_provision,   &m_connectivity,&root,    nullptr };
  m_provision    = { TXT_PROVISION,    menuShowProvision,    &m_calibration, &m_data_sending,&root,    nullptr };

  m_calibration  = { TXT_CALIBRATION,  nullptr,              &m_language,    &m_provision,  &root,     &cal_root };

  m_language     = { TXT_LANGUAGE,     menuSetLanguage,      &m_sdinfo,      &m_calibration,&root,     nullptr };
  m_sdinfo       = { TXT_SD_INFO,      menuShowSDInfo,       &m_back,        &m_language,   &root,     nullptr };
  m_back         = { TXT_BACK,         nullptr,              nullptr,        &m_sdinfo,     &root,     nullptr };

  root.text  = TXT_NONE;
  root.child = &m_status;

  // calibration submenu (child list)
  m_cal_tare = { TXT_TARE,            menuCalTare,          &m_cal_cal,  nullptr,   &cal_root, nullptr };
  m_cal_cal  = { TXT_CALIBRATE_KNOWN, menuCalCalibrate,     &m_cal_raw,  &m_cal_tare,&cal_root, nullptr };
  m_cal_raw  = { TXT_RAW_VALUE,       menuCalRaw,           &m_cal_save, &m_cal_cal,&cal_root, nullptr };
  m_cal_save = { TXT_SAVE_FACTOR,     menuCalSave,          &m_cal_back, &m_cal_raw,&cal_root, nullptr };
  m_cal_back = { TXT_BACK,            nullptr,              nullptr,     &m_cal_save,&cal_root, nullptr };

  cal_root   = { TXT_CALIBRATION,     nullptr,              &m_cal_tare, nullptr,   &m_calibration,     nullptr };

  currentItem = &m_status;

  currentMenuCount = 0;
  displayedScroll = 0;
  displayedSelectedIndex = 0;
  displayedListStart = nullptr;

  updateMirrorFromState();
}

// ... (menuDraw updates)

void menuDraw() {
  uiClear();

  MenuItem* topList[] = {
    &m_status, &m_time, &m_measure, &m_weather, &m_connectivity,
    &m_data_sending, &m_provision, &m_calibration, &m_language, &m_sdinfo, &m_back
  };
// ... (rest of menuDraw)


  MenuItem* listStart = nullptr;
  MenuItem* highlighted = nullptr;

  if (currentItem && currentItem->parent && currentItem->parent != &root) {
    listStart = currentItem->parent->child;
    highlighted = currentItem;
  } else if (currentItem == &cal_root) {
    listStart = currentItem->child;
    highlighted = listStart;
  } else {
    listStart = topList[0];
    highlighted = currentItem;
  }

  if (!listStart) { listStart = topList[0]; highlighted = currentItem; }

  const int MAX_MENU_ITEMS = 32;
  MenuItem* list[MAX_MENU_ITEMS];
  int MENU_COUNT = 0;
  MenuItem* tmp = listStart;
  while (tmp && MENU_COUNT < MAX_MENU_ITEMS) {
    list[MENU_COUNT++] = tmp;
    tmp = tmp->next;
  }

  int selectedIndex = 0;
  for (int i = 0; i < MENU_COUNT; ++i) if (list[i] == highlighted) { selectedIndex = i; break; }

  if (selectedIndex < displayedScroll) displayedScroll = selectedIndex;
  if (selectedIndex > displayedScroll + 3) displayedScroll = selectedIndex - 3;

  for (int i = 0; i < MENU_COUNT && i < 32; ++i) currentListArr[i] = list[i];
  currentMenuCount = MENU_COUNT;
  displayedListStart = listStart;
  displayedSelectedIndex = selectedIndex;

  renderFullMenu(list, MENU_COUNT, selectedIndex, displayedScroll);
}

// menuUpdate - handle buttons and re-render the visible window (no in-place partial updates)
void menuUpdate() {
  Button b = getButton();
  if (b == BTN_NONE) return;

  MenuItem* parent = currentItem->parent;
  if (!parent) parent = &root;

  MenuItem* first = parent->child;
  if (!first) first = &m_status;
  MenuItem* last  = first;
  while (last && last->next) last = last->next;

  if (b == BTN_UP_PRESSED) {
    MenuItem* oldItem = currentItem;
    if (currentItem->prev) currentItem = currentItem->prev; else currentItem = last;
    int newIndex = -1;
    for (int i = 0; i < currentMenuCount; ++i) if (currentListArr[i] == currentItem) { newIndex = i; break; }
    if (newIndex >= 0) {
      displayedSelectedIndex = newIndex;
      if (displayedSelectedIndex < displayedScroll) displayedScroll = displayedSelectedIndex;
      if (displayedSelectedIndex > displayedScroll + 3) displayedScroll = displayedSelectedIndex - 3;
      renderFullMenu(currentListArr, currentMenuCount, displayedSelectedIndex, displayedScroll);
      return;
    }
    menuDraw();
    return;
  }

  if (b == BTN_DOWN_PRESSED) {
    MenuItem* oldItem = currentItem;
    if (currentItem->next) currentItem = currentItem->next; else currentItem = first;
    int newIndex = -1;
    for (int i = 0; i < currentMenuCount; ++i) if (currentListArr[i] == currentItem) { newIndex = i; break; }
    if (newIndex >= 0) {
      displayedSelectedIndex = newIndex;
      if (displayedSelectedIndex < displayedScroll) displayedScroll = displayedSelectedIndex;
      if (displayedSelectedIndex > displayedScroll + 3) displayedScroll = displayedSelectedIndex - 3;
      renderFullMenu(currentListArr, currentMenuCount, displayedSelectedIndex, displayedScroll);
      return;
    }
    menuDraw();
    return;
  }

  if (b == BTN_BACK_PRESSED) {
    if (currentItem->parent) { currentItem = currentItem->parent; menuDraw(); }
    return;
  }

  if (b == BTN_SELECT_PRESSED) {
    if (currentItem->action) { currentItem->action(); return; }
    if (currentItem->child) { currentItem = currentItem->child; menuDraw(); return; }
  }
}

// -------------------- Menu screens (full-screen pages) ------------------
// Full-screen pages overwrite cols 0..19 padded and clear markers. uiRefreshMirror() ensures mirror sync.

void menuShowProvision() {
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  writeColsOverwrite(0, 0, padRightBytes(String(getTextEN(TXT_PROVISION)), 20));
  writeColsOverwrite(0, 1, padRightBytes(String("1) Geocode City"), 20));
  writeColsOverwrite(0, 2, padRightBytes(String(""), 20));
  writeColsOverwrite(0, 3, padRightBytes(String(getTextEN(TXT_BACK_SMALL)), 20));
  uiRefreshMirror();

  while (true) {
    server.handleClient();
    Button b = getButton();
    if (b == BTN_SELECT_PRESSED) {
      provisioning_ui_enterCityCountry();
      menuDraw();
      return;
    } else if (b == BTN_BACK_PRESSED) {
      menuDraw();
      return;
    }
    delay(80);
  }
}

void menuShowStatus() {
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  String dt = timeManager_isTimeValid() ? (timeManager_getDate() + " " + timeManager_getTime()) : String("01-01-1970  00:00:00");
  writeColsOverwrite(0, 0, padRightBytes(dt, 20));

  Preferences p; p.begin("beehive", true);
  String latS = p.getString("owm_lat","");
  String lonS = p.getString("owm_lon","");
  p.end();

  String latLine = "LAT: -----";
  String lonLine = "LON: -----";
  if (latS.length()) {
    double lat = latS.toDouble();
    char b[64]; snprintf(b, sizeof(b), "LAT:%8.4f", lat);
    latLine = String(b);
  }
  if (lonS.length()) {
    double lon = lonS.toDouble();
    char b[64]; snprintf(b, sizeof(b), "LON:%9.4f", lon);
    lonLine = String(b);
  }

  writeColsOverwrite(0, 1, padRightBytes(latLine, 20));
  writeColsOverwrite(0, 2, padRightBytes(lonLine, 20));
  writeColsOverwrite(0, 3, padRightBytes(String(getTextEN(TXT_BACK_SMALL)), 20));
  uiRefreshMirror();

  unsigned long lastUpdate = 0;
  String oldDateTime = "";
  float oldWeight = NAN;
  float oldBattV = NAN;
  int oldBattP = -999;

  while (true) {
    server.handleClient();
    timeManager_update();
    unsigned long now = millis();
    if (now - lastUpdate >= 1000) {
      lastUpdate = now;
      String ndt = timeManager_isTimeValid() ? (timeManager_getDate() + " " + timeManager_getTime()) : String("01-01-1970  00:00:00");
      if (ndt != oldDateTime) {
        writeColsOverwrite(0, 0, padRightBytes(ndt, 20));
        oldDateTime = ndt;
      }
      float w = test_weight;
      if (!(isnan(w) && isnan(oldWeight)) && fabs((isnan(w)?0:w) - (isnan(oldWeight)?0:oldWeight)) > 0.01f) {
        char buf[64]; snprintf(buf, sizeof(buf), "WEIGHT: %5.1f kg", w);
        writeColsOverwrite(0, 1, padRightBytes(String(buf), 20));
        oldWeight = w;
      }
      float bv = test_batt_voltage; int bp = test_batt_percent;
      if (!(isnan(bv) && isnan(oldBattV)) && (fabs((isnan(bv)?0:bv) - (isnan(oldBattV)?0:oldBattV)) > 0.01f || bp != oldBattP)) {
        char buf[64]; snprintf(buf, sizeof(buf), "BATTERY: %.2fV %3d%%", bv, bp);
        writeColsOverwrite(0, 2, padRightBytes(String(buf), 20));
        oldBattV = bv; oldBattP = bp;
      }
      writeColsOverwrite(0, 3, padRightBytes(String(getTextEN(TXT_BACK_SMALL)), 20));
      uiRefreshMirror();
    }
    Button btn = getButton();
    if (btn == BTN_BACK_PRESSED || btn == BTN_SELECT_PRESSED) { menuDraw(); return; }
    delay(20);
  }
}

// -------------------- SD Info (uses global sd_present) ------------------
void menuShowSDInfo() {
  while (true) {
    server.handleClient();
    menuClear();
    
    // Row 0: Header
    if (currentLanguage == LANG_EN) {
      uiPrint(0, 0, getTextEN(TXT_SD_CARD_INFO));
    } else {
      lcdPrintGreek(getTextGR(TXT_SD_CARD_INFO), 0, 0);
    }

    if (!sd_present) {
      // No card
      if (currentLanguage == LANG_EN) uiPrint(0, 1, getTextEN(TXT_NO_CARD));
      else lcdPrintGreek(getTextGR(TXT_NO_CARD), 0, 1);
    } else {
      // Card OK - Show stats
      // Row 1: File
      String fname = sdlog_getCurrentFilename();
      // Remove leading slash for display if space is tight
      if (fname.startsWith("/")) fname = fname.substring(1);
      
      String line1 = "F:" + fname;
      writeColsOverwrite(0, 1, padRightBytes(line1, 20));

      // Row 2: Records & Last Time
      // Format: "R:123 T:12:34"
      String ts = sdlog_getLastTimestamp(); // YYYY-MM-DDTHH:MM:SS
      String timePart = "";
      if (ts.length() >= 19) {
        timePart = ts.substring(11, 16); // HH:MM
      }
      
      String line2 = "R:" + String(sdlog_getRecordCount()) + " T:" + timePart;
      writeColsOverwrite(0, 2, padRightBytes(line2, 20));
    }

    // Row 3: Back
    if (currentLanguage == LANG_EN) uiPrint(0, 3, getTextEN(TXT_BACK_SMALL));
    else lcdPrintGreek(getTextGR(TXT_BACK_SMALL), 0, 3);

    uiRefreshMirror();

    // Poll buttons
    unsigned long start = millis();
    while (millis() - start < 1000) { // Refresh every second
      Button b = getButton(); 
      if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) { 
        menuDraw(); 
        return; 
      } 
      delay(50); 
    }
  }
}

void menuSetLanguage() {
  currentLanguage = (currentLanguage == LANG_EN ? LANG_GR : LANG_EN);
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  if (currentLanguage == LANG_EN) writeColsOverwrite(0,0,padRightBytes(String(getTextEN(TXT_LANGUAGE_EN)),20));
  else writeColsOverwrite(0,0,padRightBytes(String(getTextGR(TXT_LANGUAGE_GR)),20));
  uiRefreshMirror();
  delay(500);
  menuDraw();
}

void menuShowTime() {
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  writeColsOverwrite(0, 0, padRightBytes(String("DATE: ") + timeManager_getDate(), 20));
  writeColsOverwrite(0, 1, padRightBytes(String("TIME: ") + timeManager_getTime(), 20));
  writeColsOverwrite(0, 3, padRightBytes(String(getTextEN(TXT_BACK_SMALL)), 20));
  uiRefreshMirror();

  unsigned long lastUpdate = 0;
  String oldDate = ""; String oldTime = "";
  while (true) {
    server.handleClient();
    unsigned long now = millis();
    if (now - lastUpdate >= 1000) {
      lastUpdate = now;
      String nd = timeManager_getDate();
      String nt = timeManager_getTime();
      if (nd != oldDate) { writeColsOverwrite(0,0,padRightBytes(String("DATE: ") + nd,20)); oldDate = nd; }
      if (nt != oldTime) { writeColsOverwrite(0,1,padRightBytes(String("TIME: ") + nt,20)); oldTime = nt; }
      uiRefreshMirror();
    }
    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) { menuDraw(); return; }
    delay(20);
  }
}

void menuShowMeasurements() {
  int page = 0; int lastPage = -1; const int maxPage = 2;
  char buf[128];

  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  writeColsOverwrite(0, 0, padRightBytes(String(getTextEN(TXT_MEASUREMENTS)), 20));
  writeColsOverwrite(0, 3, padRightBytes(String(getTextEN(TXT_BACK_SMALL)), 20));
  uiRefreshMirror();

  while (true) {
    server.handleClient();
    if (page != lastPage) {
      if (page == 0) {
        snprintf(buf, sizeof(buf), "WEIGHT: %5.1f kg", test_weight);
        writeColsOverwrite(0,1,padRightBytes(String(buf),20));
        snprintf(buf, sizeof(buf), "T_INT:  %4.1f%s", test_temp_int, DEGREE_SYMBOL_UTF);
        writeColsOverwrite(0,2,padRightBytes(String(buf),20));
      } else if (page == 1) {
        snprintf(buf, sizeof(buf), "T_EXT:  %4.1f%s", test_temp_ext, DEGREE_SYMBOL_UTF);
        writeColsOverwrite(0,1,padRightBytes(String(buf),20));
        snprintf(buf, sizeof(buf), "H_EXT:  %3.0f%%", test_hum_ext);
        writeColsOverwrite(0,2,padRightBytes(String(buf),20));
      } else {
        snprintf(buf, sizeof(buf), "ACC: X%.2f Y%.2f", test_acc_x, test_acc_y);
        writeColsOverwrite(0,1,padRightBytes(String(buf),20));
        snprintf(buf, sizeof(buf), "Z: %.2f", test_acc_z);
        writeColsOverwrite(0,2,padRightBytes(String(buf),20));
      }
      lastPage = page;
      uiRefreshMirror();
    }
    Button b = getButton();
    if (b == BTN_UP_PRESSED) { page--; if (page < 0) page = maxPage; }
    if (b == BTN_DOWN_PRESSED) { page++; if (page > maxPage) page = 0; }
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) { menuDraw(); return; }
    delay(80);
  }
}

void menuCalTare() {
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  long offset = calib_doTare(CALIB_SAMPLES, CALIB_SKIP);
  char buf[64]; snprintf(buf, sizeof(buf), "OFFSET:%ld", offset);
  writeColsOverwrite(0,0,padRightBytes(String("TARE DONE"),20));
  writeColsOverwrite(0,1,padRightBytes(String(buf),20));
  uiRefreshMirror();
  delay(800);
  menuDraw();
}

void menuCalCalibrate() {
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  writeColsOverwrite(0,0,padRightBytes(String("CALIBRATE: READ RAW"),20));
  writeColsOverwrite(0,2,padRightBytes(String("SEL to show value"),20));
  uiRefreshMirror();

  while (true) {
    server.handleClient();
    Button b = getButton();
    if (b == BTN_SELECT_PRESSED) {
      long raw = calib_readRawAverage(CALIB_SAMPLES, CALIB_SKIP);
      char line[64]; snprintf(line,sizeof(line),"RAW: %ld", raw);
      menuClear(); for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
      writeColsOverwrite(0,1,padRightBytes(String(line),20));
      writeColsOverwrite(0,3,padRightBytes(String(getTextEN(TXT_BACK_SMALL)),20));
      uiRefreshMirror();
      while (true) {
    server.handleClient();
        Button ack = getButton();
        if (ack == BTN_BACK_PRESSED || ack == BTN_SELECT_PRESSED) { menuDraw(); return; }
        delay(60);
      }
    }
    if (b == BTN_BACK_PRESSED) { menuDraw(); return; }
    delay(80);
  }
}

void menuCalRaw() {
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  writeColsOverwrite(0,0,padRightBytes(String("RAW VALUE"),20));
  long raw = calib_readRawAverage(CALIB_SAMPLES, CALIB_SKIP);
  char buf[64]; snprintf(buf,sizeof(buf),"RAW: %ld", raw);
  writeColsOverwrite(0,1,padRightBytes(String(buf),20));
  writeColsOverwrite(0,3,padRightBytes(String(getTextEN(TXT_BACK_SMALL)),20));
  uiRefreshMirror();
  while (true) {
    server.handleClient(); Button b = getButton(); if (b==BTN_BACK_PRESSED || b==BTN_SELECT_PRESSED) { menuDraw(); return; } delay(60); }
}

void menuCalSave() {
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  if (calib_hasSavedFactor()) {
    float f = calib_getSavedFactor();
    long o = calib_getSavedOffset();
    char b1[64], b2[64];
    snprintf(b1,sizeof(b1),"FACTOR: %.3f", f);
    snprintf(b2,sizeof(b2),"OFFSET:%ld", o);
    writeColsOverwrite(0,0,padRightBytes(String(b1),20));
    writeColsOverwrite(0,1,padRightBytes(String(b2),20));
  } else {
    writeColsOverwrite(0,1,padRightBytes(String("NO CALIBRATION"),20));
  }
  writeColsOverwrite(0,3,padRightBytes(String(getTextEN(TXT_BACK_SMALL)),20));
  uiRefreshMirror();
  while (true) {
    server.handleClient(); Button b = getButton(); if (b==BTN_BACK_PRESSED || b==BTN_SELECT_PRESSED) { menuDraw(); return; } delay(80); }
}

void menuShowConnectivity() {
  Serial.println("[MENU] menuShowConnectivity() ENTER");
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  uiRefreshMirror();
  while (true) {
    server.handleClient();
    Serial.println("[MENU] menuShowConnectivity() loop iteration");
    bool wifiOK = (WiFi.status()==WL_CONNECTED);
    bool lteOK = modem_isNetworkRegistered();
    int pref = getNetworkPreference();
    
    // Dual mode display (WiFi + LTE both active)
    if (wifiOK && lteOK) {
      writeColsOverwrite(0,0,padRightBytes(String("DUAL: WiFi+LTE"),20));
      char line[128];
      snprintf(line,sizeof(line),"WiFi: %s", WiFi.SSID().c_str());
      writeColsOverwrite(0,1,padRightBytes(String(line),20));
      snprintf(line,sizeof(line),"LTE: %ddBm (Data)", (int)modem_getRSSI());
      writeColsOverwrite(0,2,padRightBytes(String(line),20));
      writeColsOverwrite(0,3,padRightBytes(String("SEL:Change BACK:Exit"),20));
    }
    // WiFi only
    else if (wifiOK) {
      int32_t rssi = WiFi.RSSI();
      writeColsOverwrite(0,0,padRightBytes(String(getTextEN(TXT_WIFI_CONNECTED)),20));
      char line[128]; snprintf(line,sizeof(line),"%s %s", getTextEN(TXT_SSID), WiFi.SSID().c_str());
      writeColsOverwrite(0,1,padRightBytes(String(line),20));
      snprintf(line,sizeof(line),"%s %ddBm", getTextEN(TXT_RSSI), rssi);
      writeColsOverwrite(0,2,padRightBytes(String(line),20));
      writeColsOverwrite(0,3,padRightBytes(String(getTextEN(TXT_BACK_SMALL)),20));
    } else if (lteOK) {
      int16_t r = modem_getRSSI();
      writeColsOverwrite(0,0,padRightBytes(String(getTextEN(TXT_LTE_REGISTERED)),20));
      char line[128]; snprintf(line,sizeof(line),"%s %ddBm", getTextEN(TXT_RSSI), r);
      writeColsOverwrite(0,1,padRightBytes(String(line),20));
      writeColsOverwrite(0,2,padRightBytes(String("MODE: LTE"),20));
      writeColsOverwrite(0,3,padRightBytes(String(getTextEN(TXT_BACK_SMALL)),20));
    } else {
      writeColsOverwrite(0,0,padRightBytes(String(getTextEN(TXT_NO_CONNECTIVITY)),20));
      writeColsOverwrite(0,1,padRightBytes(String(""),20));
      writeColsOverwrite(0,2,padRightBytes(String(""),20));
      writeColsOverwrite(0,3,padRightBytes(String(getTextEN(TXT_BACK_SMALL)),20));
    }

    uiUpdateNetworkIndicator();
    uiRefreshMirror();

    Button b = getButton();
    if (b == BTN_SELECT_PRESSED) {
      int sel = wifiOK ? 0 : (lteOK ? 1 : 0);
      auto drawChoice = [&](int selected) {
        menuClear();
        for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
        writeColsOverwrite(0,0,padRightBytes(String("Choose network:"),20));
        writeColsOverwrite(0,1,padRightBytes((selected==0?"> WiFi":"  WiFi"),20));
        writeColsOverwrite(0,2,padRightBytes((selected==1?"> LTE":"  LTE"),20));
        writeColsOverwrite(0,3,padRightBytes(String("UP/DOWN=Sel  SEL=OK"),20));
        uiRefreshMirror();
      };
      drawChoice(sel);
      while (true) {
    server.handleClient();
        Button c = getButton();
        if (c == BTN_UP_PRESSED || c == BTN_DOWN_PRESSED) { sel = 1 - sel; drawChoice(sel); }
        else if (c == BTN_BACK_PRESSED) { menuDraw(); return; }
        else if (c == BTN_SELECT_PRESSED) {
          if (sel == 0) { setNetworkPreference(CONNECTIVITY_WIFI); menuDraw(); return; }
          else { setNetworkPreference(CONNECTIVITY_LTE); menuDraw(); return; }
        }
        delay(80);
      }
    }

    if (b == BTN_BACK_PRESSED) {
      Serial.println("[MENU] menuShowConnectivity() EXIT (BACK pressed)");
      menuDraw();
      return;
    }
    delay(200);
  }
}

void menuShowWeather() {
  Serial.println("[MENU] menuShowWeather() ENTER");
  menuClear();
  for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
  Serial.println("[MENU] Reading location preferences...");
  Preferences p; p.begin("beehive", true);
  String placeName = p.getString("loc_name","");
  String country = p.getString("loc_country","");
  String latS = p.getString("owm_lat","");
  String lonS = p.getString("owm_lon","");
  p.end();

  double lat = DEFAULT_LAT, lon = DEFAULT_LON;
  if (latS.length() && lonS.length()) { lat = latS.toDouble(); lon = lonS.toDouble(); }

  char line0[128], line1[128], line2[128], line3[128];
  snprintf(line0,sizeof(line0),"WEATHER=====>SEL==>");
  snprintf(line1,sizeof(line1),"LAT:%6.2f LON:%6.2f", lat, lon);
  if (placeName.length()>0) {
    if (country.length()>0) { String pc = placeName + ", " + country; snprintf(line2,sizeof(line2), "%s", pc.c_str()); }
    else snprintf(line2,sizeof(line2), "%s", placeName.c_str());
  } else snprintf(line2,sizeof(line2), " ");
  snprintf(line3,sizeof(line3), "%s", getTextEN(TXT_BACK_SMALL));

  Serial.println("[MENU] Displaying location info...");
  writeColsOverwrite(0,0,padRightBytes(String(line0),20));
  writeColsOverwrite(0,1,padRightBytes(String(line1),20));
  writeColsOverwrite(0,2,padRightBytes(String(line2),20));
  writeColsOverwrite(0,3,padRightBytes(String(line3),20));
  uiRefreshMirror();
  delay(1200);

  Serial.println("[MENU] Showing 'Fetching Weather' message...");
  menuClear();
  if (currentLanguage == LANG_EN) writeColsOverwrite(0,0,padRightBytes(String(getTextEN(TXT_FETCHING_WEATHER)),20));
  else writeColsOverwrite(0,0,padRightBytes(String(getTextGR(TXT_FETCHING_WEATHER)),20));
  uiRefreshMirror();

  Serial.println("[MENU] Calling weather_fetch() - THIS MAY BLOCK IF WIFI NOT AVAILABLE");
  weather_fetch();
  Serial.println("[MENU] weather_fetch() returned");

  int page = 0, lastPage = -1;
  WeatherDay wd;
  while (true) {
    server.handleClient();
    int total = weather_daysCount();
    int maxPage = (total > 0) ? (total - 1) : 0;
    if (page != lastPage) {
      if (!weather_hasData()) {
        writeColsOverwrite(0,0,padRightBytes(String(getTextEN(TXT_WEATHER_NO_DATA)),20));
        writeColsOverwrite(0,1,padRightBytes(String(""),20));
        writeColsOverwrite(0,2,padRightBytes(String(""),20));
        writeColsOverwrite(0,3,padRightBytes(String(currentLanguage == LANG_EN ? getTextEN(TXT_BACK_SMALL) : getTextGR(TXT_BACK_SMALL)),20));
      } else {
        if (page < 0) page = 0;
        if (page > maxPage) page = maxPage;
        weather_getDay(page, wd);
        char buf[128];
        snprintf(buf, sizeof(buf), "%s", wd.date.c_str());
        writeColsOverwrite(0,0,padRightBytes(String(buf),20));
        writeColsOverwrite(0,1,padRightBytes(String(wd.desc.c_str()),20));
        snprintf(buf, sizeof(buf), "T:%5.1f" DEGREE_SYMBOL_UTF "C H:%3.0f%%", wd.temp_min, wd.humidity);
        writeColsOverwrite(0,2,padRightBytes(String(buf),20));
        snprintf(buf, sizeof(buf), "P:%5.0fhPa %s", wd.pressure, currentLanguage == LANG_EN ? getTextEN(TXT_BACK_SMALL) : getTextGR(TXT_BACK_SMALL));
        writeColsOverwrite(0,3,padRightBytes(String(buf),20));
      }
      lastPage = page;
      uiRefreshMirror();
    }
    Button b = getButton();
    if (b == BTN_UP_PRESSED) { page--; if (page < 0) page = 0; }
    if (b == BTN_DOWN_PRESSED) { page++; if (page > maxPage) page = 0; }
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      Serial.println("[MENU] menuShowWeather() EXIT (button pressed)");
      menuDraw();
      return;
    }
    delay(80);
  }
}

// -------------------- Data Sending Menu ------------------
void menuShowDataSending() {
  // Intervals in minutes
  // Intervals in minutes: 1, 5, 15, 30, 60, 2h(120), 6h(360), Daily(1440)
  const int intervals[] = { 1, 5, 15, 30, 60, 120, 360, 1440 };
  const char* labels[]  = { "1 min", "5 min", "15 min", "30 min", "60 min", "2 hrs", "6 hrs", "Daily" };
  const int count = 8;

  // Load current setting
  Preferences p;
  p.begin("beehive", true);
  int currentMin = p.getInt("ts_interval", 60); // default 60 min
  p.end();

  int selected = 4; // default to 60 min index (0-based: 1,5,15,30,60)
  for (int i=0; i<count; ++i) {
    if (intervals[i] == currentMin) { selected = i; break; }
  }

  Serial.println("[MENU] menuShowDataSending() ENTER");
  auto draw = [&](int sel) {
    Serial.printf("[MENU] Drawing DataSending selection: %d\n", sel);
    menuClear();
    for (int r = 0; r < 4; ++r) ui_setMarkerCharAtRow(r, ' ');
    
    // Header
    if (currentLanguage == LANG_EN) writeColsOverwrite(0,0,padRightBytes(String(getTextEN(TXT_DATA_SENDING)),20));
    else writeColsOverwrite(0,0,padRightBytes(String(getTextGR(TXT_DATA_SENDING)),20));

    // Selection
    String s = String(getTextEN(TXT_INTERVAL_SELECT)) + String(labels[sel]);
    if (currentLanguage == LANG_GR) s = String(getTextGR(TXT_INTERVAL_SELECT)) + String(labels[sel]);
    
    writeColsOverwrite(0,1,padRightBytes(s,20));
    writeColsOverwrite(0,2,padRightBytes(String("UP/DOWN change"),20));
    writeColsOverwrite(0,3,padRightBytes(String("SEL=Save  BACK=Exit"),20));
    uiRefreshMirror();
  };

  draw(selected);

  while (true) {
    server.handleClient();
    Button b = getButton();
    if (b == BTN_UP_PRESSED) {
      selected++; if (selected >= count) selected = 0;
      draw(selected);
    } else if (b == BTN_DOWN_PRESSED) {
      selected--; if (selected < 0) selected = count - 1;
      draw(selected);
    } else if (b == BTN_BACK_PRESSED) {
      menuDraw(); return;
    } else if (b == BTN_SELECT_PRESSED) {
      // Save
      Preferences p;
      p.begin("beehive", false);
      p.putInt("ts_interval", intervals[selected]);
      p.end();
      
      uiClear();
      writeColsOverwrite(0,1,padRightBytes(String("SAVED!"),20));
      uiRefreshMirror();
      delay(1000);
      menuDraw(); 
      return;
    }
    delay(80);
  }
}
