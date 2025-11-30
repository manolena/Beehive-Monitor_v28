// sensors.cpp
// Bridge module: does NOT define test_* globals (they are defined in the .ino).
// Provides sensors_init() and sensors_update() that operate on the globals that
// already exist in the sketch (declared extern in config.h).
//
// IMPORTANT: keep your .ino definitions (the placeholders) as-is; this file
// must NOT re-declare/define them.
#include "config.h"
#include "sensors.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "modem_manager.h"

// The actual globals are defined in the .ino (placeholders).
// Here we only declare them as extern so this translation unit can use them.
extern float test_weight;
extern float test_temp_int;
extern float test_hum_int;
extern float test_temp_ext;
extern float test_hum_ext;
extern float test_pressure;
extern float test_acc_x;
extern float test_acc_y;
extern float test_acc_z;
extern float test_batt_voltage;
extern int   test_batt_percent;
extern int   test_rssi;
extern double test_lat;
extern double test_lon;

// External alarm trigger (implemented in .ino)
extern void trigger_alarm(String reason);

static bool sensors_initialized = false;
static Adafruit_MPU6050 mpu;
static bool mpu_found = false;

bool sensors_init() {
  if (sensors_initialized) return true;

  #if ENABLE_DEBUG
    Serial.println("[SENS] sensors_init() called");
  #endif

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("[SENS] Failed to find MPU6050 chip");
    mpu_found = false;
  } else {
    Serial.println("[SENS] MPU6050 Found!");
    mpu_found = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  // Ensure GPS is enabled on the modem
  // Note: This might take time if modem is not ready, so we do best effort.
  // Ideally modem_manager handles its own init.
  // We just ensure we try to enable GPS if we can.
  // For now, we assume modemManager_init() is called in setup() before this.
  
  sensors_initialized = true;
  return true;
}

bool sensors_update_loadcell() {
  // Replace with real HX711 reading code once load cell is connected.
  // Example: long raw = hx.read_average(...); test_weight = (raw - offset) / factor / 1000.0;
  (void)0;
  return true;
}

bool sensors_update_internal() {
  // Replace with real SI7021 or other internal T/H sensor read.
  (void)0;
  return true;
}

bool sensors_update_external() {
  // Replace with real BME/BMP reading (temp/hum/pressure).
  (void)0;
  return true;
}

bool sensors_update_accel() {
  if (!mpu_found) return false;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float new_x = a.acceleration.x;
  float new_y = a.acceleration.y;
  float new_z = a.acceleration.z;

  // Check for movement (alarm)
  // We compare against the LAST reading (test_acc_x/y/z).
  // If this is the first reading (NAN), we just store it.
  if (!isnan(test_acc_x)) {
    float dx = abs(new_x - test_acc_x);
    float dy = abs(new_y - test_acc_y);
    float dz = abs(new_z - test_acc_z);

    if (dx > ACCEL_THRESHOLD || dy > ACCEL_THRESHOLD || dz > ACCEL_THRESHOLD) {
      String reason = "Motion detected! dX=" + String(dx) + " dY=" + String(dy) + " dZ=" + String(dz);
      Serial.println("[ALARM] " + reason);
      trigger_alarm(reason);
    }
  }

  test_acc_x = new_x;
  test_acc_y = new_y;
  test_acc_z = new_z;

  return true;
}

bool sensors_update_gps() {
  // Try to enable GPS if not already (idempotent-ish)
  modem_enableGPS(true);

  double lat = 0, lon = 0;
  
  // Note: modem.getGPS() can block waiting for AT response.
  // We can't easily make TinyGSM non-blocking, but we can wrap it
  // or just accept it takes a few hundred ms. 
  // If it takes longer (e.g. timeout), it will freeze UI.
  // For now, let's just ensure we don't block forever.
  
  // Actually, better approach: only update GPS if we really need to,
  // or accept a small freeze. But if it freezes for seconds, that's bad.
  
  // Let's try to get GPS.
  if (modem_getGPS(lat, lon)) {
    test_lat = lat;
    test_lon = lon;
    #if ENABLE_DEBUG
      Serial.printf("[SENS] GPS Updated: %.6f, %.6f\n", test_lat, test_lon);
    #endif
    return true;
  }
  return false;
}

bool sensors_update_battery() {
  // If you later measure battery here, update test_batt_voltage and test_batt_percent.
  // For now do nothing because placeholders live in .ino and you asked to keep them.
  (void)0;
  return true;
}

bool sensors_update() {
  if (!sensors_initialized) {
    sensors_init();
  }

  bool ok = true;
  ok &= sensors_update_loadcell();
  ok &= sensors_update_internal();
  ok &= sensors_update_external();
  ok &= sensors_update_accel();
  ok &= sensors_update_battery();
  // Note: sensors_update_gps is NOT called here to avoid blocking/power drain.
  // It is called periodically from loop().

  // Optionally update RSSI if modem available (example):
  // test_rssi = modem_getRSSI();

  return ok;
}