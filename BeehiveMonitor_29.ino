// BeehiveMonitor_29.ino - top-level sketch (clean: only setup() and loop())

#include <Arduino.h>
#include <math.h> // for NAN / isnan
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
#include "sensors.h"
#include "sd_logger.h"

#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h> // Web server used for provisioning endpoints

#include <SPI.h>
#include <SD.h>

// SD presence flag (set during setup)
bool sd_present = false;

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

// GPS coordinates
double test_lat = 0.0;
double test_lon = 0.0;

// integer sentinels
int   test_batt_percent = -999; // sentinel: unknown
int   test_rssi = -999;         // sentinel: unknown

// single WebServer instance used by provisioning and other HTTP endpoints
WebServer server(80);

// External helpers (implemented elsewhere)
extern bool wifi_connectFromPrefs(unsigned long timeoutMs);
extern void tryStartLTE();
extern void dumpWifiPrefs(); // diagnostics function (defined in diagnostics.cpp)

void setup() {
  Serial.begin(115200);
  delay(50);

  // Ensure lcdMutex exists early (safe-guard, uiInit also does this)
  if (!lcdMutex) {
    lcdMutex = xSemaphoreCreateMutex();
  }

  uiInit();
  showSplashScreen();

  // SD card initialization (added). Sets global sd_present.
#ifdef SD_CS
  Serial.print("[SD] Attempting SD init. CS=");
  Serial.println(SD_CS);
  // Try to initialize SPI explicitly with the pins defined in config.h if available
#ifdef SD_SCLK
#ifdef SD_MISO
#ifdef SD_MOSI
  Serial.print("[SD] SPI pins (SCLK,MISO,MOSI)=(");
  Serial.print(SD_SCLK); Serial.print(",");
  Serial.print(SD_MISO); Serial.print(",");
  Serial.print(SD_MOSI); Serial.println(")");
  // Initialize SPI with the pins the user declared. On ESP32 this remaps SPI to those pins.
  // Use SPI.begin(sck, miso, mosi);
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);
#else
  // MOSI missing
  Serial.println("[SD] SD_MOSI not defined - using SPI.begin() default pins");
  SPI.begin();
#endif
#else
  Serial.println("[SD] SD_MISO not defined - using SPI.begin() default pins");
  SPI.begin();
#endif
#else
  Serial.println("[SD] SD_SCLK not defined - using SPI.begin() default pins");
  SPI.begin();
#endif

  // First try: default SD.begin(CS)
  if (SD.begin(SD_CS)) {
    sd_present = true;
    Serial.println("[SD] Initialized successfully (SD.begin(CS))");
  } else {
    Serial.println("[SD] SD.begin(CS) FAILED - trying explicit pin call SD.begin(CS, SCLK, MISO, MOSI)");

    // Second try: explicit pins order (ESP32 signature: SD.begin(CS, sck, miso, mosi))
#ifdef SD_SCLK
#ifdef SD_MISO
#ifdef SD_MOSI
    // Try again with lower frequency (1MHz) using the global SPI object
    // Note: SPI.begin(SCLK, MISO, MOSI) was already called above, so the global SPI object is configured.
    if (SD.begin(SD_CS, SPI, 1000000)) {
      sd_present = true;
      Serial.println("[SD] Initialized successfully (SD.begin(CS, SPI, 1MHz))");
    } else {
      sd_present = false;
      Serial.println("[SD] Initialization FAILED (retry with 1MHz)");
    }
#else
    sd_present = false;
    Serial.println("[SD] Cannot try explicit SD.begin - SD_MOSI not defined");
#endif
#else
    sd_present = false;
    Serial.println("[SD] Cannot try explicit SD.begin - SD_MISO not defined");
#endif
#else
    sd_present = false;
    Serial.println("[SD] Cannot try explicit SD.begin - SD_SCLK not defined");
#endif
  }
#else
  Serial.println("[SD] SD_CS not defined in config.h - skipping SD initialization");
  sd_present = false;
#endif

  // Initialize SD logging (if SD card present)
  sdlog_init();

  menuInit();
  menuDraw();

  network_init();

  dumpWifiPrefs();

  sms_init();
  // modem_hw_init(); // Called inside modemManager_init()
  modemManager_init();
  timeManager_init();

  // apply network preference and ensure WiFi connects
  int np = getNetworkPreference();
  if (np == CONNECTIVITY_LTE) {
    tryStartLTE();
  } else if (np == CONNECTIVITY_WIFI) { 
    Serial.println(F("[SETUP] Connecting to WiFi..."));
    if (!wifi_connectFromPrefs(10000)) {
      Serial.println(F("[NET] Forced WiFi failed"));
    } else {
      Serial.print(F("[SETUP] WiFi connected! IP: "));
      Serial.println(WiFi.localIP());
    }
  } else { 
    // AUTO mode - try LTE first, then WiFi
    tryStartLTE(); 
    Serial.println(F("[SETUP] Attempting WiFi connection..."));
    if (wifi_connectFromPrefs(8000)) {
      Serial.print(F("[SETUP] WiFi connected! IP: "));
      Serial.println(WiFi.localIP());
    }
  }

  // Start HTTP server and register provisioning endpoints
  server.begin();
  provisioning_register(server);

  // Register LCD root UI BEFORE key server so root (/) serves the LCD/provision page.
  lcd_register(server);

  keyServer_registerRoutes(server);

  // Print final IP address for web access
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("================================="));
    Serial.print(F("Web Server Running at: http://"));
    Serial.print(WiFi.localIP());
    Serial.println(F("/"));
    Serial.println(F("================================="));
  } else {
    Serial.println(F("[SETUP] Warning: WiFi not connected - web server unavailable"));
  }

  // Note: lcd_register is intentionally registered before keyServer to ensure "/" serves the UI.
}

// =========================================================
// Alarm Logic
// =========================================================
void trigger_alarm(String reason) {
  Serial.println("[ALARM] Triggered: " + reason);
  
  // 1. Send SMS
  // Check if phone number is configured (not default placeholder)
  String phone = ALARM_PHONE_NUMBER;
  if (phone.length() > 5 && phone != "+306912345678") {
    sms_send(phone, "ALARM: " + reason);
  } else {
    Serial.println("[ALARM] SMS skipped - phone number not configured");
  }

  // 2. Optional: Show on LCD?
  // lcd_show_alarm(reason); // (not implemented yet)
}

// =========================================================
// Main Loop
// =========================================================
static unsigned long last_gps_update = 0;

void loop() {
  menuUpdate();
  timeManager_update();
  manageAutoNetwork();

  keyServer_loop();
  serial_commands_poll();
  sms_loop();

  server.handleClient();

  // -------------------------------------------------------
  // Sensor & Alarm Updates
  // -------------------------------------------------------
  
  // TEST: Trigger alarm once after boot for testing
  // COMMENTED OUT - System is stable, no need for test SMS anymore
  /*
  static bool test_alarm_triggered = false;
  static unsigned long boot_time = millis();
  if (!test_alarm_triggered && (millis() - boot_time > 30000)) {
    test_alarm_triggered = true;
    Serial.println("[TEST] Triggering test alarm 30s after boot...");
    trigger_alarm("TEST: Accelerometer simulation after reboot");
  }
  */
  
  // 1. Accelerometer: Check frequently (every loop or fast interval)
  // The sensor module handles the threshold check and calls trigger_alarm if needed.
  sensors_update_accel();

  // 2. GPS & ThingSpeak: Check periodically based on configured interval
  // We read the interval from preferences (or default 60 min).
  // Ideally we cache this, but reading Prefs every loop is slow? 
  // Actually, let's cache it static or read every few seconds.
  static unsigned long last_interval_check = 0;
  static unsigned long current_interval_ms = GPS_UPDATE_INTERVAL; // default from config.h

  unsigned long now = millis();

  if (now - last_interval_check > 5000) {
    last_interval_check = now;
    Preferences p;
    p.begin("beehive", true);
    int mins = p.getInt("ts_interval", 0); // 0 means not set, use default
    p.end();
    if (mins > 0) current_interval_ms = (unsigned long)mins * 60000UL;
    else current_interval_ms = GPS_UPDATE_INTERVAL;
  }

  if (now - last_gps_update > current_interval_ms || last_gps_update == 0) {
    // Check if user is active - if so, defer update to avoid freezing UI
    if (isUserActive()) {
      // Only log once per second to avoid spam
      static unsigned long last_defer_log = 0;
      if (now - last_defer_log > 1000) {
        Serial.println("[MAIN] Periodic update deferred (User Active)");
        last_defer_log = now;
      }
      // Defer by resetting last_gps_update slightly so we retry soon
      // But don't change last_gps_update, just skip this iteration
      return; 
    }

    last_gps_update = now;
    Serial.printf("[MAIN] Starting periodic Data Send (Interval: %lu ms)...\n", current_interval_ms);
    
    menuUpdate(); // Keep UI alive
    
    // Update GPS
    if (sensors_update_gps()) {
      Serial.println("[MAIN] GPS update successful");
    } else {
      Serial.println("[MAIN] GPS update failed (no fix or modem error)");
    }

    menuUpdate(); // Keep UI alive

    // Upload to ThingSpeak (Sensor data + GPS)
    if (thingspeak_upload_current()) {
       Serial.println("[MAIN] ThingSpeak upload successful");
    } else {
       Serial.println("[MAIN] ThingSpeak upload failed");
    }

    // Log to SD card (same interval as ThingSpeak)
    if (sdlog_write()) {
      Serial.println("[MAIN] SD log written successfully");
    } else {
      Serial.println("[MAIN] SD log write failed or disabled");
    }
  }

  delay(10);
}
