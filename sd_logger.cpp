#include "sd_logger.h"
#include "config.h"
#include <SD.h>
#include <time.h>

// External sensor globals
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
extern int test_batt_percent;
extern double test_lat;
extern double test_lon;
extern int test_rssi;
extern int connectivityMode;
extern bool sd_present;

// Static variables
static bool sdlog_enabled = false;
static String current_filename = "";
static int record_count = 0;
static String last_timestamp = "";

// CSV header
static const char* CSV_HEADER = 
  "Timestamp,Date,Time,Weight_kg,Temp_Int_C,Hum_Int_%,Temp_Ext_C,Hum_Ext_%,"
  "Pressure_hPa,Acc_X,Acc_Y,Acc_Z,Battery_V,Battery_%,Latitude,Longitude,RSSI_dBm,Network";

// Helper: Get current date/time strings
static void getDateTime(String &timestamp, String &date, String &time_str) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // Fallback if time not available
    timestamp = "0000-00-00T00:00:00";
    date = "0000-00-00";
    time_str = "00:00:00";
    return;
  }
  
  char buf[32];
  
  // ISO 8601 timestamp
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  timestamp = String(buf);
  
  // Date only
  strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);
  date = String(buf);
  
  // Time only
  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
  time_str = String(buf);
}

// Helper: Get filename for today
static String getFilenameForToday() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "/beehive_00000000.csv"; // Fallback
  }
  
  char buf[32];
  strftime(buf, sizeof(buf), "/beehive_%Y%m%d.csv", &timeinfo);
  return String(buf);
}

// Helper: Format float for CSV (empty if NAN)
static String formatFloat(float value, int decimals = 1) {
  if (isnan(value)) return "";
  return String(value, decimals);
}

// Helper: Format int for CSV (empty if sentinel)
static String formatInt(int value, int sentinel = -999) {
  if (value == sentinel) return "";
  return String(value);
}

// Helper: Get network name
static String getNetworkName() {
  if (connectivityMode == CONNECTIVITY_WIFI) return "WiFi";
  if (connectivityMode == CONNECTIVITY_LTE) return "LTE";
  return "Offline";
}

// Initialize SD logging
void sdlog_init() {
  if (!sd_present) {
    sdlog_enabled = false;
    Serial.println("[SDLOG] SD card not present, logging disabled");
    return;
  }
  
  sdlog_enabled = true;
  record_count = 0;
  Serial.println("[SDLOG] SD logging initialized");
}

// Write sensor data to CSV
bool sdlog_write() {
  if (!sdlog_enabled || !sd_present) {
    return false;
  }
  
  // Get current filename
  String filename = getFilenameForToday();
  
  // Check if file exists (need to write header if new file)
  bool file_exists = SD.exists(filename.c_str());
  bool is_new_day = (filename != current_filename);
  
  if (is_new_day) {
    current_filename = filename;
    record_count = 0;
    Serial.print("[SDLOG] New day, file: ");
    Serial.println(filename);
  }
  
  // Open file for append
  File dataFile = SD.open(filename.c_str(), FILE_APPEND);
  if (!dataFile) {
    Serial.println("[SDLOG] ERROR: Failed to open file for writing");
    return false;
  }
  
  // Write header if new file
  if (!file_exists) {
    dataFile.println(CSV_HEADER);
    Serial.println("[SDLOG] Wrote CSV header");
  }
  
  // Get timestamp
  String timestamp, date, time_str;
  getDateTime(timestamp, date, time_str);
  last_timestamp = timestamp;
  
  // Build CSV row
  String row = "";
  row += timestamp + ",";
  row += date + ",";
  row += time_str + ",";
  row += formatFloat(test_weight, 2) + ",";
  row += formatFloat(test_temp_int, 1) + ",";
  row += formatFloat(test_hum_int, 1) + ",";
  row += formatFloat(test_temp_ext, 1) + ",";
  row += formatFloat(test_hum_ext, 1) + ",";
  row += formatFloat(test_pressure, 1) + ",";
  row += formatFloat(test_acc_x, 3) + ",";
  row += formatFloat(test_acc_y, 3) + ",";
  row += formatFloat(test_acc_z, 3) + ",";
  row += formatFloat(test_batt_voltage, 2) + ",";
  row += formatInt(test_batt_percent) + ",";
  row += String(test_lat, 6) + ",";
  row += String(test_lon, 6) + ",";
  row += formatInt(test_rssi) + ",";
  row += getNetworkName();
  
  // Write row
  dataFile.println(row);
  dataFile.close();
  
  record_count++;
  
  Serial.print("[SDLOG] Wrote record #");
  Serial.print(record_count);
  Serial.print(" to ");
  Serial.println(filename);
  
  return true;
}

// Get current filename
String sdlog_getCurrentFilename() {
  if (current_filename.isEmpty()) {
    return getFilenameForToday();
  }
  return current_filename;
}

// Check if enabled
bool sdlog_isEnabled() {
  return sdlog_enabled && sd_present;
}

// Get record count
int sdlog_getRecordCount() {
  return record_count;
}

// Get last timestamp
String sdlog_getLastTimestamp() {
  return last_timestamp;
}
