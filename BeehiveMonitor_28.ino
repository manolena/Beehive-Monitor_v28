// BeehiveMonitor_28.ino - startup with shared WebServer and lcd mirror endpoint integrated
#include <Arduino.h>
#include <math.h> // για NAN / isnan
#include "config.h"
#include "ui.h"
#include "menu_manager.h"
#include "modem_manager.h"
#include "time_manager.h"
#include "weather_manager.h"
#include "key_server.h"
#include "thingspeak_client.h"
#include "serial_commands.h"

#include "network_manager.h"
#include "provisioning_server.h"
#include "lcd_server_simple.h"
#include "lcd_endpoint.h"
#include "sms_handler.h"

#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h> // Web server used for provisioning endpoints

// Sensor / telemetry globals (definitions) - initialize floats to NAN to indicate 'no reading'
float test_weight = NAN;
float test_temp_int = NAN;
float test_hum_int = NAN;
float test_temp_ext = NAN;
float test_hum_ext = NAN;
float test_pressure = NAN;
float test_acc_x = NAN;
float test_acc_y = NAN;
float test_acc_z = NAN;
float test_batt_voltage = NAN;

// integer sentinels
int   test_batt_percent = -999; // sentinel: unknown
int   test_rssi = -999;         // sentinel: unknown

// single WebServer instance used by provisioning and other HTTP endpoints
WebServer server(80);

extern bool wifi_connectFromPrefs(unsigned long timeoutMs);
extern void tryStartLTE();
extern void dumpWifiPrefs(); // diagnostics function (defined in diagnostics.cpp)

void setup() {
  Serial.begin(115200);
  delay(50);

  // Ensure lcdMutex exists early (safe-guard, uiInit also does this)
  if (!lcdMutex) {
    lcdMutex = xSemaphoreCreateMutex();
    if (lcdMutex) ;
    else ;
  }

  uiInit();
  showSplashScreen();
  
  menuInit();
  menuDraw();

  network_init();

  dumpWifiPrefs();

  sms_init();
  modem_hw_init();
  modemManager_init();
  timeManager_init();  

  // apply network preference
  int np = getNetworkPreference();
  if (np == CONNECTIVITY_LTE) tryStartLTE();
  else if (np == CONNECTIVITY_WIFI) { if (!wifi_connectFromPrefs(8000)) Serial.println(F("[NET] Forced WiFi failed")); }
  else { tryStartLTE(); if (wifi_connectFromPrefs(5000)) {} }

  // Start HTTP server and register provisioning endpoints
  server.begin();
  provisioning_register(server);

  // Register LCD root UI BEFORE key server so root (/) serves the LCD/provision page.
  lcd_register(server);

  keyServer_registerRoutes(server);

  // Note: lcd_register is intentionally registered before keyServer to ensure "/" serves the UI.
}

void loop() {
  menuUpdate();
  timeManager_update();
  manageAutoNetwork();

  keyServer_loop();
  serial_commands_poll();
  sms_loop();

  server.handleClient();

  delay(10);
}