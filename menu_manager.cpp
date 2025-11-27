// menu_manager.cpp
// Restored full menu manager with calibration submenu and safe calibrate flow.

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

extern LiquidCrystal_I2C lcd;

// -------------------- Menu item storage --------------------------------
static MenuItem root;
static MenuItem* currentItem = nullptr;

static MenuItem m_status;
static MenuItem m_time;
static MenuItem m_measure;
static MenuItem m_weather;
static MenuItem m_connectivity;
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

// Forward declarations (externally visible handlers)
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

// -------------------- Mirror helpers ----------------------------------
static void updateMirrorFromState()
{
  String net;
  if (WiFi.status() == WL_CONNECTED) net = "WIFI";
  else if (modem_isNetworkRegistered()) net = "LTE ";
  else net = "    ";

  for (int line = 0; line < 4; ++line) {
    int idx = displayedScroll + line;
    char buf[21];
    if (idx >= 0 && idx < currentMenuCount && currentListArr[idx]) {
      TextId id = currentListArr[idx]->text;
      const char* label = (currentLanguage == LANG_EN) ? getTextEN(id) : getTextGR(id);
      char left[17];
      char marker = (idx == displayedSelectedIndex) ? '>' : ' ';
      snprintf(left, sizeof(left), "%c%-15.15s", marker, label);
      snprintf(buf, sizeof(buf), "%s%s", left, net.c_str());
    } else {
      snprintf(buf, sizeof(buf), " %15s%s", " ", net.c_str());
    }
    String s = String(buf);
    while (s.length() < 20) s += ' ';
    if (s.length() > 20) s = s.substring(0,20);
    lcd_set_line_simple(line, s);
  }
}

// safe marker update
static void setMarkerCharAtRow(uint8_t row, char ch) {
  ui_setMarkerCharAtRow(row, ch);
  updateMirrorFromState();
}

// Render a full menu to the physical LCD (and update mirror)
static void renderFullMenu(MenuItem* list[], int MENU_COUNT, int selectedIndex, int scroll) {
  for (int line = 0; line < 4; ++line) {
    int idx = scroll + line;
    if (idx >= MENU_COUNT) {
      uiPrint(0, line, "                    ");
      continue;
    }
    TextId id = list[idx]->text;
    const char* label = (currentLanguage == LANG_EN) ? getTextEN(id) : getTextGR(id);
    char marker = (idx == selectedIndex) ? '>' : ' ';
    if (currentLanguage == LANG_EN) {
      // print label starting at col 1 to leave room for marker at 0
      uiPrint(1, line, label);
      ui_setMarkerCharAtRow(line, marker);
    } else {
      lcdPrintGreek(label, 1, line);
      ui_setMarkerCharAtRow(line, marker);
    }
  }
}

// -------------------- Public menu API ---------------------------------
void menuInit() {
  // Build main menu linked list
  m_status       = { TXT_STATUS,       menuShowStatus,       &m_time,        nullptr,       &root,     nullptr };
  m_time         = { TXT_TIME,         menuShowTime,         &m_measure,     &m_status,     &root,     nullptr };
  m_measure      = { TXT_MEASUREMENTS, menuShowMeasurements, &m_weather,     &m_time,       &root,     nullptr };
  m_weather      = { TXT_WEATHER,      menuShowWeather,      &m_connectivity,&m_measure,    &root,     nullptr };
  m_connectivity = { TXT_CONNECTIVITY, menuShowConnectivity, &m_provision,   &m_weather,    &root,     nullptr };
  m_provision    = { TXT_PROVISION,    menuShowProvision,    &m_calibration, &m_connectivity,&root,    nullptr };

  // IMPORTANT: m_calibration should not have an action; selecting it navigates into the calibration submenu.
  m_calibration  = { TXT_CALIBRATION,  nullptr,              &cal_root,      &m_provision,  &root,     nullptr };

  m_language     = { TXT_LANGUAGE,     menuSetLanguage,      &m_sdinfo,      &m_calibration,&root,     nullptr };
  m_sdinfo       = { TXT_SD_INFO,      menuShowSDInfo,       &m_back,        &m_language,   &root,     nullptr };
  m_back         = { TXT_BACK,         nullptr,              nullptr,        &m_sdinfo,     &root,     nullptr };

  root.text  = TXT_NONE;
  root.child = &m_status;

  // calibration submenu (child of cal_root)
  m_cal_tare = { TXT_TARE,            menuCalTare,          &m_cal_cal,  nullptr,   &cal_root, nullptr };
  m_cal_cal  = { TXT_CALIBRATE_KNOWN, menuCalCalibrate,     &m_cal_raw,  &m_cal_tare,&cal_root, nullptr };
  m_cal_raw  = { TXT_RAW_VALUE,       menuCalRaw,           &m_cal_save, &m_cal_cal,&cal_root, nullptr };
  m_cal_save = { TXT_SAVE_FACTOR,     menuCalSave,          &m_cal_back, &m_cal_raw,&cal_root, nullptr };
  m_cal_back = { TXT_BACK,            nullptr,              nullptr,     &m_cal_save,&root,    nullptr };

  cal_root   = { TXT_CALIBRATION,     nullptr,              &m_cal_tare, nullptr,   &root,     nullptr };

  currentItem = &m_status;

  currentMenuCount = 0;
  displayedScroll = 0;
  displayedSelectedIndex = 0;
  displayedListStart = nullptr;

  updateMirrorFromState();
}

void menuDraw() {
  uiClear();

  MenuItem* topList[] = {
    &m_status, &m_time, &m_measure, &m_weather, &m_connectivity,
    &m_provision, &m_calibration, &m_language, &m_sdinfo, &m_back
  };

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
  uiUpdateNetworkIndicator();
  updateMirrorFromState();
}

void menuUpdate() {
  Button b = getButton();
  if (b == BTN_NONE) return;

  MenuItem* parent = currentItem->parent;
  if (!parent) parent = &root;

  MenuItem* first = parent->child;
  MenuItem* last  = first;
  while (last && last->next) last = last->next;

  if (b == BTN_UP_PRESSED) {
    MenuItem* oldItem = currentItem;
    if (currentItem->prev) currentItem = currentItem->prev; else currentItem = last;
    int oldIndex = -1, newIndex = -1;
    for (int i = 0; i < currentMenuCount; ++i) {
      if (currentListArr[i] == oldItem) oldIndex = i;
      if (currentListArr[i] == currentItem) newIndex = i;
    }
    if (oldIndex >= 0 && newIndex >= 0) {
      if (oldIndex >= displayedScroll && oldIndex <= displayedScroll + 3 &&
          newIndex >= displayedScroll && newIndex <= displayedScroll + 3) {
        setMarkerCharAtRow(oldIndex - displayedScroll, ' ');
        setMarkerCharAtRow(newIndex - displayedScroll, '>');
        displayedSelectedIndex = newIndex;
        return;
      } else {
        displayedSelectedIndex = newIndex;
        if (newIndex < displayedScroll) displayedScroll = newIndex;
        if (newIndex > displayedScroll + 3) displayedScroll = newIndex - 3;
        renderFullMenu(currentListArr, currentMenuCount, displayedSelectedIndex, displayedScroll);
        updateMirrorFromState();
        return;
      }
    }
    menuDraw();
    return;
  }

  if (b == BTN_DOWN_PRESSED) {
    MenuItem* oldItem = currentItem;
    if (currentItem->next) currentItem = currentItem->next; else currentItem = first;
    int oldIndex = -1, newIndex = -1;
    for (int i = 0; i < currentMenuCount; ++i) {
      if (currentListArr[i] == oldItem) oldIndex = i;
      if (currentListArr[i] == currentItem) newIndex = i;
    }
    if (oldIndex >= 0 && newIndex >= 0) {
      if (oldIndex >= displayedScroll && oldIndex <= displayedScroll + 3 &&
          newIndex >= displayedScroll && newIndex <= displayedScroll + 3) {
        setMarkerCharAtRow(oldIndex - displayedScroll, ' ');
        setMarkerCharAtRow(newIndex - displayedScroll, '>');
        displayedSelectedIndex = newIndex;
        return;
      } else {
        displayedSelectedIndex = newIndex;
        if (newIndex < displayedScroll) displayedScroll = newIndex;
        if (newIndex > displayedScroll + 3) displayedScroll = newIndex - 3;
        renderFullMenu(currentListArr, currentMenuCount, displayedSelectedIndex, displayedScroll);
        updateMirrorFromState();
        return;
      }
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
// Minimal provision handler to satisfy linker and preserve behavior.
// Calls provisioning_ui_enterCityCountry() on SELECT, returns to menu on BACK.
void menuShowProvision() {
  uiClear();
  if (currentLanguage == LANG_EN) {
    uiPrint(0,0,getTextEN(TXT_PROVISION));
    uiPrint(0,1,"1) Geocode City      ");
    uiPrint(0,2,"                    ");
    uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
  } else {
    lcdPrintGreek(getTextGR(TXT_PROVISION),0,0);
    lcdPrintGreek("1) \u03a3\u0395\u0391 \u0393\u0395\u039f",0,1);
    lcdPrintGreek("                    ",0,2);
    lcdPrintGreek(getTextGR(TXT_BACK_SMALL),0,3);
  }

  while (true) {
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
// -------------------- Menu screens ------------------------------------

void menuShowStatus() {
  uiClear();
  unsigned long lastUpdate = 0;
  String oldDateTime = "";
  float oldWeight = NAN;
  float oldBattV = NAN;
  int oldBattP = -999;

  while (true) {
    timeManager_update();
    unsigned long now = millis();
    if (now - lastUpdate >= 1000) {
      lastUpdate = now;
      String dt = timeManager_isTimeValid() ? (timeManager_getDate() + " " + timeManager_getTime()) : String("01-01-1970  00:00:00");
      if (dt != oldDateTime) {
        if (currentLanguage == LANG_EN) uiPrint(0,0,dt.c_str());
        else lcdPrintGreek(dt.c_str(),0,0);
        oldDateTime = dt;
      }
      float w = test_weight;
      char line[21];
      if (!(isnan(w) && isnan(oldWeight)) && fabs((isnan(w)?0:w) - (isnan(oldWeight)?0:oldWeight)) > 0.01f) {
        if (currentLanguage == LANG_EN) snprintf(line,21,"WEIGHT: %5.1f kg   ", w);
        else snprintf(line,21,"\u0392\u0391\u03a1\u039f\u03a3: %5.1fkg     ", w);
        if (currentLanguage == LANG_EN) uiPrint(0,1,line); else lcdPrintGreek(line,0,1);
        oldWeight = w;
      }
      float bv = test_batt_voltage; int bp = test_batt_percent;
      if (!(isnan(bv) && isnan(oldBattV)) && (fabs((isnan(bv)?0:bv) - (isnan(oldBattV)?0:oldBattV)) > 0.01f || bp != oldBattP)) {
        if (currentLanguage == LANG_EN) snprintf(line,21,"BATTERY: %.2fV %3d%% ", bv, bp);
        else snprintf(line,21,"\u039c\u03a0\u0391\u03a4\u0391\u03a1\u0399\u0391:%.2fV %3d%% ", bv, bp);
        if (currentLanguage == LANG_EN) uiPrint(0,2,line); else lcdPrintGreek(line,0,2);
        oldBattV = bv; oldBattP = bp;
      }
      if (currentLanguage == LANG_EN) uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
      else lcdPrintGreek(getTextGR(TXT_BACK_SMALL),0,3);
    }
    Button btn = getButton();
    if (btn == BTN_BACK_PRESSED || btn == BTN_SELECT_PRESSED) { menuDraw(); return; }
    delay(20);
  }
}

// Remaining handlers (menuShowTime, menuShowMeasurements, menuShowWeather, menuShowConnectivity,
// menuShowProvision, menuSetLanguage, menuShowSDInfo, calibration handlers) are present below.

void menuShowTime() {
  uiClear();
  unsigned long lastUpdate = 0;
  String oldDate = ""; String oldTime = ""; TimeSource oldSrc = TSRC_NONE;
  while (true) {
    unsigned long now = millis();
    if (now - lastUpdate >= 1000) {
      lastUpdate = now;
      String d = timeManager_getDate();
      String t = timeManager_getTime();
      TimeSource src = timeManager_getSource();
      const char* srcName = (src == TSRC_WIFI) ? "WIFI" : (src == TSRC_LTE) ? "LTE" : "NONE";
      if (d != oldDate) { if (currentLanguage == LANG_EN) uiPrint(0,0,(String("DATE: ")+d).c_str()); else { char line[21]; snprintf(line,21,"\u0397\u039c/\u039d\u0399\u0391: %s", d.c_str()); lcdPrintGreek(line,0,0); } oldDate = d; }
      if (t != oldTime) { if (currentLanguage == LANG_EN) uiPrint(0,1,(String("TIME: ")+t).c_str()); else { char line[21]; snprintf(line,21,"\u03a9\u03a1\u0391:    %s", t.c_str()); lcdPrintGreek(line,0,1); } oldTime = t; }
      if (src != oldSrc) { if (currentLanguage == LANG_EN) uiPrint(0,2,(String("SRC:  ")+srcName).c_str()); else { char line[21]; snprintf(line,21,"\u03a0\u0397\u0393\u0397:   %s", srcName); lcdPrintGreek(line,0,2); } oldSrc = src; }
      if (currentLanguage == LANG_EN) uiPrint(0,3,getTextEN(TXT_BACK_SMALL)); else lcdPrintGreek(getTextGR(TXT_BACK_SMALL),0,3);
    }
    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) { menuDraw(); return; }
    delay(20);
  }
}

void menuShowMeasurements() {
  int page = 0; int lastPage = -1; const int maxPage = 2; char line[21];
  while (true) {
    if (page != lastPage) {
      uiClear();
      if (currentLanguage == LANG_EN) {
        uiPrint(0,0,getTextEN(TXT_MEASUREMENTS));
        if (page == 0) {
          snprintf(line,21,"WEIGHT: %5.1f kg  ", test_weight); uiPrint(0,1,line);
          snprintf(line,21,"T_INT:  %4.1f%s     ", test_temp_int, DEGREE_SYMBOL_UTF); uiPrint(0,2,line);
          snprintf(line,21,"H_INT:  %3.0f%%     ", test_hum_int); uiPrint(0,3,line);
        } else if (page == 1) {
          snprintf(line,21,"T_EXT:  %4.1f%s     ", test_temp_ext, DEGREE_SYMBOL_UTF); uiPrint(0,1,line);
          snprintf(line,21,"H_EXT:  %3.0f%%     ", test_hum_ext); uiPrint(0,2,line);
          snprintf(line,21,"PRESS: %4.0fhPa    ", test_pressure); uiPrint(0,3,line);
        } else {
          snprintf(line,21,"ACC: X%.2f Y%.2f   ", test_acc_x, test_acc_y); uiPrint(0,1,line);
          snprintf(line,21,"Z: %.2f            ", test_acc_z); uiPrint(0,2,line);
          snprintf(line,21,"BAT: %.2fV %3d%%    ", test_batt_voltage, test_batt_percent); uiPrint(0,3,line);
        }
      } else {
        lcdPrintGreek(getTextGR(TXT_MEASUREMENTS),0,0);
        if (page==0) {
          snprintf(line,21,"\u0392\u0391\u03a1\u039f\u03a3: %5.1fkg     ", test_weight); lcdPrintGreek(line,0,1);
          snprintf(line,21,"\u0398\u0395\u03a1\u039c. \u0395\u03a3\u03a9: %4.1f%s  ", test_temp_int, DEGREE_SYMBOL_UTF); lcdPrintGreek(line,0,2);
          snprintf(line,21,"\u03a5\u0393\u03a1. \u0395\u03a3\u03a9: %3.0f%%   ", test_hum_int); lcdPrintGreek(line,0,3);
        } else if (page==1) {
          snprintf(line,21,"\u0398\u0395\u03a1\u039c. \u0395\u039a\u03a9: %4.1f%s  ", test_temp_ext, DEGREE_SYMBOL_UTF); lcdPrintGreek(line,0,1);
          snprintf(line,21,"\u03a5\u0393\u03a1. \u0395\u039a\u03a9: %3.0f%%   ", test_hum_ext); lcdPrintGreek(line,0,2);
          snprintf(line,21,"\u0391\u03a4\u039c. \u03a0\u0399\u0395\u03a3\u0397:%4.0fhPa", test_pressure); lcdPrintGreek(line,0,3);
        } else {
          snprintf(line,21,"\u0395\u03a0\u0399\u03a4:X%.2f Y%.2f    ", test_acc_x, test_acc_y); lcdPrintGreek(line,0,1);
          snprintf(line,21,"Z:%.2f             ", test_acc_z); lcdPrintGreek(line,0,2);
          snprintf(line,21,"\u039c\u03a0\u0391\u03a4:%.2fV %3d%%    ", test_batt_voltage, test_batt_percent); lcdPrintGreek(line,0,3);
        }
      }
      lastPage = page;
    }
    Button b = getButton();
    if (b == BTN_UP_PRESSED) { page--; if (page < 0) page = maxPage; }
    if (b == BTN_DOWN_PRESSED) { page++; if (page > maxPage) page = 0; }
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) { menuDraw(); return; }
    delay(80);
  }
}

void menuShowSDInfo() {
  uiClear();
  bool ok = SD.begin(SD_CS);
  if (currentLanguage == LANG_EN) {
    uiPrint(0,0,getTextEN(TXT_SD_CARD_INFO));
    uiPrint(0,1, ok ? getTextEN(TXT_SD_OK) : getTextEN(TXT_NO_CARD));
    uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
  } else {
    lcdPrintGreek(getTextGR(TXT_SD_CARD_INFO),0,0);
    lcdPrintGreek(ok ? getTextGR(TXT_SD_OK) : getTextGR(TXT_NO_CARD),0,1);
    lcdPrintGreek(getTextGR(TXT_BACK_SMALL),0,3);
  }
  while (true) { Button b = getButton(); if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) { menuDraw(); return; } delay(50); }
}

void menuSetLanguage() {
  currentLanguage = (currentLanguage == LANG_EN ? LANG_GR : LANG_EN);
  uiClear();
  if (currentLanguage == LANG_EN) uiPrint(0,0,getTextEN(TXT_LANGUAGE_EN));
  else lcdPrintGreek(getTextGR(TXT_LANGUAGE_GR),0,0);
  delay(500);
  menuDraw();
}

void menuShowCalibration() {
  uiClear();
  // simple entry view if invoked directly
  if (currentLanguage == LANG_EN) uiPrint(0,0,getTextEN(TXT_CALIBRATION));
  else lcdPrintGreek(getTextGR(TXT_CALIBRATION),0,0);
  uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
  while (true) {
    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) { menuDraw(); return; }
    delay(60);
  }
}

void menuCalTare() {
  uiClear();
  long offset = calib_doTare(CALIB_SAMPLES, CALIB_SKIP);
  char buf[21]; snprintf(buf,sizeof(buf),"OFFSET:%ld", offset);
  if (currentLanguage==LANG_EN) { uiPrint(0,0,"TARE DONE           "); uiPrint(0,1,buf); }
  else { lcdPrintGreek_P(F("ΑΠΟΒΑΡΟ ΜΕΤΡΗΘΗΚΕ ΟΚ"),0,0); lcdPrintGreek(buf,0,1); }
  delay(800); menuDraw();
}

// Safe calibrate flow: do not call missing external APIs. Read raw average and show it.
// This preserves the menu behaviour and avoids linker errors.
void menuCalCalibrate() {
  uiClear();
  uiPrint(0,0,"CALIBRATE: READ RAW ");
  uiPrint(0,2,"SEL to show value   ");
  Button b;
  while (true) {
    b = getButton();
    if (b == BTN_SELECT_PRESSED) {
      long raw = calib_readRawAverage(CALIB_SAMPLES, CALIB_SKIP);
      char line[21];
      snprintf(line, sizeof(line), "RAW: %ld           ", raw);
      uiClear();
      uiPrint(0,1,line);
      uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
      while (true) {
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
  uiClear(); uiPrint_P(F("RAW VALUE"),0,0);
  long raw = calib_readRawAverage(CALIB_SAMPLES, CALIB_SKIP);
  char line[21]; snprintf(line,sizeof(line),"RAW: %ld       ", raw); uiPrint(0,1,line); uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
  while (true) { Button b = getButton(); if (b==BTN_BACK_PRESSED || b==BTN_SELECT_PRESSED) { menuDraw(); return; } delay(60); }
}

void menuCalSave() {
  uiClear();
  if (calib_hasSavedFactor()) {
    float f = calib_getSavedFactor();
    long o = calib_getSavedOffset();
    char b1[21], b2[21];
    snprintf(b1,sizeof(b1),"FACTOR: %.3f", f);
    snprintf(b2,sizeof(b2),"OFFSET:%ld", o);
    uiPrint(0,0,b1); uiPrint(0,1,b2);
    uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
  } else {
    uiPrint_P(F("NO CALIBRATION"),0,1);
    uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
  }
  while (true) { Button b = getButton(); if (b==BTN_BACK_PRESSED || b==BTN_SELECT_PRESSED) { menuDraw(); return; } delay(80); }
}

void menuShowConnectivity() {
  uiClear();
  while (true) {
    bool wifiOK = (WiFi.status()==WL_CONNECTED);
    bool lteOK = modem_isNetworkRegistered();
    if (wifiOK) {
      int32_t rssi = WiFi.RSSI();
      uiPrint(0,0,getTextEN(TXT_WIFI_CONNECTED));
      char line[21]; snprintf(line,sizeof(line),"%s %s", getTextEN(TXT_SSID), WiFi.SSID().c_str()); uiPrint(0,1,line);
      snprintf(line,sizeof(line),"%s %ddBm", getTextEN(TXT_RSSI), rssi); uiPrint(0,2,line);
      uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
    } else if (lteOK) {
      int16_t r = modem_getRSSI();
      uiPrint(0,0,getTextEN(TXT_LTE_REGISTERED));
      char line[21]; snprintf(line,sizeof(line),"%s %ddBm", getTextEN(TXT_RSSI), r); uiPrint(0,1,line);
      uiPrint(0,2,"MODE: LTE         ");
      uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
    } else {
      uiPrint(0,0,getTextEN(TXT_NO_CONNECTIVITY));
      uiPrint(0,1,"                   ");
      uiPrint(0,2,"                   ");
      uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
    }

    uiUpdateNetworkIndicator();

    Button b = getButton();
    if (b == BTN_SELECT_PRESSED) {
      int sel = wifiOK ? 0 : (lteOK ? 1 : 0);
      auto drawChoice = [&](int selected) {
        uiClear();
        uiPrint(0,0,"Choose network:      ");
        uiPrint(0,1, selected==0 ? "> WiFi              " : "  WiFi              ");
        uiPrint(0,2, selected==1 ? "> LTE               " : "  LTE               ");
        uiPrint(0,3,"UP/DOWN=Sel  SEL=OK ");
      };
      drawChoice(sel);
      while (true) {
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

    if (b == BTN_BACK_PRESSED) { menuDraw(); return; }
    delay(200);
  }
}

void menuShowWeather() {
  uiClear();
  Preferences p; p.begin("beehive", true);
  String placeName = p.getString("loc_name","");
  String country = p.getString("loc_country","");
  String latS = p.getString("owm_lat","");
  String lonS = p.getString("owm_lon","");
  p.end();

  double lat = DEFAULT_LAT, lon = DEFAULT_LON;
  if (latS.length() && lonS.length()) { lat = latS.toDouble(); lon = lonS.toDouble(); }

  char line0[21], line1[21], line2[21], line3[21];
  snprintf(line0,sizeof(line0),"%-20s","WEATHER=====>SEL==>");
  snprintf(line1,sizeof(line1),"LAT:%6.2f LON:%6.2f", lat, lon);
  if (placeName.length()>0) {
    if (country.length()>0) { String pc = placeName + ", " + country; snprintf(line2,sizeof(line2),"%-20s", pc.c_str()); }
    else snprintf(line2,sizeof(line2),"%-20s", placeName.c_str());
  } else snprintf(line2,sizeof(line2),"%-20s"," ");
  snprintf(line3,sizeof(line3),"%-20s", getTextEN(TXT_BACK_SMALL));

  uiClear();
  if (currentLanguage==LANG_EN) { uiPrint(0,0,line0); uiPrint(0,1,line1); uiPrint(0,2,line2); uiPrint(0,3,line3); }
  else { lcdPrintGreek(line0,0,0); lcdPrintGreek(line1,0,1); lcdPrintGreek(line2,0,2); lcdPrintGreek(getTextGR(TXT_BACK_SMALL),0,3); }
  delay(1200);

  uiClear();
  if (currentLanguage == LANG_EN) uiPrint(0,0,getTextEN(TXT_FETCHING_WEATHER));
  else lcdPrintGreek(getTextGR(TXT_FETCHING_WEATHER),0,0);

  weather_fetch();

  int page = 0, lastPage = -1;
  WeatherDay wd;
  while (true) {
    int total = weather_daysCount();
    int maxPage = (total > 0) ? (total - 1) : 0;
    if (page != lastPage) {
      uiClear();
      if (!weather_hasData()) {
        if (currentLanguage == LANG_EN) uiPrint(0,0,getTextEN(TXT_WEATHER_NO_DATA));
        else lcdPrintGreek(getTextGR(TXT_WEATHER_NO_DATA),0,0);
      } else {
        if (page < 0) page = 0;
        if (page > maxPage) page = maxPage;
        weather_getDay(page, wd);
        char line[21];
        if (currentLanguage == LANG_EN) {
          snprintf(line,21,"%s                ", wd.date.c_str()); uiPrint(0,0,line);
          snprintf(line,21,"%-20s", wd.desc.c_str()); uiPrint(0,1,line);
          snprintf(line,21,"T:%5.1f" DEGREE_SYMBOL_UTF "C H:%3.0f%%", wd.temp_min, wd.humidity); uiPrint(0,2,line);
          snprintf(line,21,"P:%5.0fhPa %s", wd.pressure, getTextEN(TXT_BACK_SMALL)); uiPrint(0,3,line);
        } else {
          snprintf(line,21,"%s                ", wd.date.c_str()); lcdPrintGreek(line,0,0);
          lcdPrintGreek(wd.desc.c_str(),0,1);
          snprintf(line,21,"T:%5.1f" DEGREE_SYMBOL_UTF "C H:%3.0f%%", wd.temp_min, wd.humidity); lcdPrintGreek(line,0,2);
          snprintf(line,21,"P:%5.0fhPa %s", wd.pressure, getTextEN(TXT_BACK_SMALL)); lcdPrintGreek(line,0,3);
        }
      }
      lastPage = page;
    }
    Button b = getButton();
    if (b == BTN_UP_PRESSED) { page--; if (page < 0) page = 0; }
    if (b == BTN_DOWN_PRESSED) { page++; if (page > maxPage) page = 0; }
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) { menuDraw(); return; }
    delay(80);
  }
}