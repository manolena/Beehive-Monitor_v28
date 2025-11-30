#include "config.h"
#include "weather_manager.h"
#include "ui.h"               // <<-- ensure this line is present so currentLanguage and LANG_GR are visible
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "time_manager.h"
#include "modem_manager.h"

// For LTE weather fetching
// For LTE weather fetching
// TinyGsmClient.h is already included in modem_manager.h with correct defines
// extern TinyGsm modem;  // This is wrong, use modem_get()

Preferences prefs;

static const char *PREF_NS = "beehive";
static const char *PREF_KEY_LAT = "owm_lat";
static const char *PREF_KEY_LON = "owm_lon";
static const char *PREF_KEY_LOC_NAME = "loc_name";
static const char *PREF_KEY_LOC_COUNTRY = "loc_country";

static double s_lat = DEFAULT_LAT;
static double s_lon = DEFAULT_LON;
static bool   s_hasData = false;
static String s_lastError = "";

static WeatherDay *s_days = nullptr;
static int s_daysCount = 0;

// URL-encode helper (RFC3986-ish). Returns encoded string.
static String urlEncode(const String &str) {
  String encoded;
  encoded.reserve(str.length() * 3);
  const char *p = str.c_str();
  for (; *p; ++p) {
    unsigned char c = (unsigned char)*p;
    if ( (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') ||
         c == '-' || c == '_' || c == '.' || c == '~' ) {
      encoded += (char)c;
    } else if (c == ' ') {
      encoded += '+'; // form-style
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}

void weather_init() {
  prefs.begin(PREF_NS, false);
  String latS = prefs.getString(PREF_KEY_LAT, "");
  String lonS = prefs.getString(PREF_KEY_LON, "");
  if (latS.length() && lonS.length()) {
    s_lat = latS.toDouble();
    s_lon = lonS.toDouble();
  } else {
    s_lat = DEFAULT_LAT;
    s_lon = DEFAULT_LON;
  }
  s_hasData = false;
  s_lastError = "";
  if (s_days) { delete[] s_days; s_days = nullptr; s_daysCount = 0; }
}

// Store coords as strings (persist)
void weather_setCoords(double lat, double lon) {
  prefs.begin(PREF_NS, false);
  prefs.putString(PREF_KEY_LAT, String(lat, 6));
  prefs.putString(PREF_KEY_LON, String(lon, 6));
  prefs.end();
  s_lat = lat;
  s_lon = lon;
  Serial.print("[Weather] coords saved: lat=");
  Serial.print(lat, 6);
  Serial.print(" lon=");
  Serial.println(lon, 6);
}

String weather_getLastError() {
  return s_lastError;
}

// Map Open-Meteo weathercode to short description, language-aware
static String mapWeatherCodeOpenMeteo(int code) {
  if (currentLanguage == LANG_GR) {
    switch(code) {
      case 0: return "ΑΙΘΡΙΟΣ";
      case 1: case 2: case 3: return "ΝΕΦΕΛΩΔΗΣ";
      case 45: case 48: return "ΟΜΙΧΛΗ";
      case 51: case 53: case 55: return "ΧΙΟΝΟΝΕΡΟ";
      case 61: case 63: case 65: return "ΒΡΟΧΗ";
      case 71: case 73: case 75: return "ΧΙΟΝΙ";
      case 80: case 81: case 82: return "ΚΑΤΑΙΓΙΔΑ";
      case 95: case 96: case 99: return "ΚΕΡΑΥΝΟΠΤΩΣΗ";
      default: return "N/A";
    }
  } else {
    switch(code) {
      case 0: return "Clear";
      case 1: case 2: case 3: return "Partly cloudy";
      case 45: case 48: return "Fog";
      case 51: case 53: case 55: return "Drizzle";
      case 61: case 63: case 65: return "Rain";
      case 71: case 73: case 75: return "Snow";
      case 80: case 81: case 82: return "Showers";
      case 95: case 96: case 99: return "Thunder";
      default: return "N/A";
    }
  }
}

// Geocode via Open-Meteo geocoding API, store coords and place name/country in prefs on success
bool weather_geocodeLocation(const char* city, const char* countryCode) {
  if (!city) return false;
  String q = String(city);

  // Build base URL using name parameter (city only).
  String url = String("https://geocoding-api.open-meteo.com/v1/search?name=") + urlEncode(q)
               + "&count=1&language=en&format=json";

  // If a country code was provided, append the dedicated countryCode parameter.
  // Ensure we use the ISO-3166-1 alpha2 form (uppercase).
  if (countryCode && strlen(countryCode) > 0) {
    String cc = String(countryCode);
    cc.toUpperCase();
    url += String("&countryCode=") + urlEncode(cc);
  }

  Serial.print("[Weather] Geocode (OpenMeteo) -> ");
  Serial.println(url);

  if (WiFi.status() != WL_CONNECTED) {
    s_lastError = "WiFi not connected";
    Serial.println("[Weather] Geocode failed: WiFi not connected");
    return false;
  }

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  String body;
  if (code > 0) body = http.getString();
  http.end();

  if (code != 200) {
    s_lastError = String("HTTP_") + String(code) + ": " + body;
    Serial.print("[Weather] Geocode failed: ");
    Serial.println(s_lastError);
    return false;
  }

  // Parse JSON with ArduinoJson v7 API
  const size_t CAP = 10 * 1024;
  DynamicJsonDocument doc(CAP);
  DeserializationError derr = deserializeJson(doc, body);
  if (derr) {
    s_lastError = "JSON parse error";
    Serial.print("[Weather] Geocode JSON parse failed: ");
    Serial.println(derr.c_str());
    return false;
  }

  if (!doc.containsKey("results") || doc["results"].size() == 0) {
    s_lastError = "No geocode result";
    Serial.println("[Weather] Geocode no result");
    return false;
  }

  JsonObject r = doc["results"][0].as<JsonObject>();
  double lat = r["latitude"] | 0.0;
  double lon = r["longitude"] | 0.0;
  if (lat == 0.0 && lon == 0.0) {
    s_lastError = "Invalid coords";
    Serial.println("[Weather] Geocode invalid coords");
    return false;
  }

  // Persist coords
  weather_setCoords(lat, lon);

  // Persist place name & country for UI display
  String placeName = "";
  String country = "";
  if (r.containsKey("name"))    placeName = String((const char*)r["name"].as<const char*>());
  if (r.containsKey("country")) country   = String((const char*)r["country"].as<const char*>());

  prefs.begin(PREF_NS, false);
  if (placeName.length()) prefs.putString(PREF_KEY_LOC_NAME, placeName);
  if (country.length())   prefs.putString(PREF_KEY_LOC_COUNTRY, country);
  prefs.end();

  s_lastError = "";
  Serial.printf("[Weather] Geocode OK: lat=%.6f lon=%.6f", lat, lon);
  if (placeName.length()) {
    Serial.print(" place=");
    Serial.print(placeName);
    if (country.length()) { Serial.print(", country="); Serial.print(country); }
  }
  Serial.println();
  return true;
}

// Fetch Open-Meteo forecast: hourly arrays, sample every 6 hours for next 72h
static bool weather_fetch_open_meteo() {
  Serial.println("[Weather] weather_fetch_open_meteo() ENTER");
  
  // include humidity and surface_pressure in hourly arrays
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=")
    + String(s_lat, 6) + "&longitude=" + String(s_lon, 6)
    + "&hourly=temperature_2m,weathercode,relativehumidity_2m,surface_pressure&forecast_days=3&timezone=auto";

  Serial.print("[Weather] OpenMeteo Request URL: ");
  Serial.println(url);
  Serial.print("[Weather] WiFi status: ");
  Serial.println(WiFi.status());

  if (WiFi.status() != WL_CONNECTED) {
    s_lastError = "WiFi not connected";
    Serial.println("[Weather] OpenMeteo fetch SKIPPED: WiFi not connected (LTE mode?)");
    Serial.println("[Weather] weather_fetch_open_meteo() EXIT (no WiFi)");
    return false;
  }
  
  Serial.println("[Weather] WiFi connected, proceeding with HTTP request...");

  Serial.println("[Weather] Starting HTTPClient.GET() - THIS MAY BLOCK");
  HTTPClient http;
  http.begin(url);
  http.setTimeout(10000);  // 10 second timeout
  int code = http.GET();
  Serial.print("[Weather] HTTPClient.GET() completed, code=");
  Serial.println(code);
  String body;
  if (code > 0) body = http.getString();
  http.end();
  Serial.println("[Weather] HTTP request complete");

  if (code != 200) {
    s_lastError = String("HTTP_") + String(code) + ": " + body;
    Serial.print("[Weather] OpenMeteo HTTP fail: ");
    Serial.println(s_lastError);
    Serial.println(body.substring(0, min((int)body.length(), 512)));
    return false;
  }

  // Parse JSON
  const size_t CAP = 28 * 1024; // adjust if memory issues appear
  DynamicJsonDocument doc(CAP);
  DeserializationError derr = deserializeJson(doc, body);
  if (derr) {
    s_lastError = "JSON parse failed";
    Serial.print("[Weather] OpenMeteo JSON parse failed: ");
    Serial.println(derr.c_str());
    return false;
  }

  if (!doc.containsKey("hourly")) {
    s_lastError = "No hourly data";
    Serial.println("[Weather] No hourly data in OpenMeteo response");
    return false;
  }

  JsonObject hourly = doc["hourly"].as<JsonObject>();
  if (!hourly.containsKey("time") || !hourly.containsKey("temperature_2m")) {
    s_lastError = "Incomplete hourly data";
    Serial.println("[Weather] Hourly data incomplete");
    return false;
  }

  JsonArray times = hourly["time"].as<JsonArray>();
  JsonArray temps = hourly["temperature_2m"].as<JsonArray>();
  JsonArray codes = hourly["weathercode"].as<JsonArray>();
  JsonArray hums  = hourly.containsKey("relativehumidity_2m") ? hourly["relativehumidity_2m"].as<JsonArray>() : JsonArray();
  JsonArray press = hourly.containsKey("surface_pressure") ? hourly["surface_pressure"].as<JsonArray>() : JsonArray();

  int totalHours = times.size();
  if (totalHours <= 0) {
    s_lastError = "No hourly entries";
    Serial.println("[Weather] No hourly entries");
    return false;
  }

  // We'll sample every 6 hours starting from index 0, up to next 72 hours (3 days)
  const int HOURS_TO_COVER = 72;
  const int STEP = 6;
  int maxSamples = (HOURS_TO_COVER / STEP);
  // free previous cache
  if (s_days) { delete[] s_days; s_days = nullptr; s_daysCount = 0; }
  s_days = new WeatherDay[maxSamples];
  int samples = 0;
  for (int offset = 0; offset < HOURS_TO_COVER && (offset < totalHours); offset += STEP) {
    int idx = offset; // index into hourly arrays
    if (idx >= totalHours) break;
    String tISO = String(times[idx].as<const char*>());
    double temp = temps[idx].as<double>();
    int wc = 0;
    if (idx < (int)codes.size()) wc = codes[idx].as<int>();
    double hum = NAN;
    if (!hums.isNull() && idx < (int)hums.size()) hum = hums[idx].as<double>();
    double pr = NAN;
    if (!press.isNull() && idx < (int)press.size()) pr = press[idx].as<double>();

    // convert pressure to hPa if necessary:
    // - Open-Meteo surface_pressure typically returns hPa. But if a value looks like Pa (>2000),
    //   convert by dividing by 100.
    if (!isnan(pr)) {
      if (pr > 2000.0) pr = pr / 100.0;
    }

    // convert ISO "YYYY-MM-DDThh:mm" or "YYYY-MM-DD hh:mm" to "DD-MM HH:MM"
    String hhmm = "00:00";
    int posT = tISO.indexOf('T');
    if (posT < 0) posT = tISO.indexOf(' ');
    if (posT >= 0) hhmm = tISO.substring(posT+1, posT+6);
    int y = tISO.substring(0,4).toInt();
    int m = tISO.substring(5,7).toInt();
    int d = tISO.substring(8,10).toInt();

    char dbuf[20];
    snprintf(dbuf, sizeof(dbuf), "%02d-%02d %s", d, m, hhmm.c_str());
    s_days[samples].date = String(dbuf);
    s_days[samples].temp_min = (float)temp;
    s_days[samples].temp_max = (float)temp;
    s_days[samples].humidity = isnan(hum) ? 0.0f : (float)hum; // percent
    s_days[samples].pressure = isnan(pr) ? 0.0f : (float)pr;   // hPa
    s_days[samples].desc = mapWeatherCodeOpenMeteo(wc);
    samples++;
    if (samples >= maxSamples) break;
  }

  if (samples == 0) {
    s_lastError = "No samples parsed";
    Serial.println("[Weather] No samples parsed from OpenMeteo");
    // free memory
    delete[] s_days;
    s_days = nullptr;
    s_daysCount = 0;
    s_hasData = false;
    return false;
  }

  s_daysCount = samples;
  s_hasData = true;
  s_lastError = "";
  Serial.print("[Weather] OpenMeteo fetch OK, samples=");
  Serial.println(s_daysCount);
  return true;
}

// Fetch weather via LTE modem using TinyGSM HTTP client
static bool weather_fetch_via_lte() {
  Serial.println("[Weather] weather_fetch_via_lte() ENTER");
  
  if (!modem_isNetworkRegistered()) {
    s_lastError = "LTE not registered";
    Serial.println("[Weather] LTE not registered");
    return false;
  }
  
  // Build URL (use HTTPS for LTE)
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=")
    + String(s_lat, 6) + "&longitude=" + String(s_lon, 6)
    + "&hourly=temperature_2m,weathercode,relativehumidity_2m,surface_pressure&forecast_days=3&timezone=auto";
  
  Serial.print("[Weather] LTE Request URL: ");
  Serial.println(url);
  
  // Parse URL for TinyGSM
  String urlStr = String(url);
  int hostStart = urlStr.indexOf("//") + 2;
  int pathStart = urlStr.indexOf("/", hostStart);
  String host = urlStr.substring(hostStart, pathStart);
  String path = urlStr.substring(pathStart);
  
  Serial.print("[Weather] Host: ");
  Serial.println(host);
  Serial.print("[Weather] Path: ");
  Serial.println(path);
  
  // Get modem instance and create client (Standard client, we will enable SSL manually)
  TinyGsm& modem = modem_get();
  TinyGsmClient client(modem);
  
  // Connect to server
  Serial.println("[Weather] Connecting to server via LTE (HTTPS Manual)...");
  
  // Debug signal quality
  int csq = modem.getSignalQuality();
  Serial.print("[Weather] Signal Quality (CSQ): ");
  Serial.println(csq);
  
  // Ensure GPRS is connected
  if (!modem.isGprsConnected()) {
    Serial.println("[Weather] GPRS not connected, attempting to connect...");
    if (!modem.gprsConnect(MODEM_APN)) {
       Serial.println("[Weather] GPRS connect failed");
       s_lastError = "GPRS connect failed";
       return false;
    }
    Serial.println("[Weather] GPRS connected");
  }
  
  // MANUAL SSL ENABLE FOR A7670/SIM7600
  // This tells the modem to use SSL for the next TCP connection
  Serial.println("[Weather] Enabling SSL (AT+CIPSSL=1)...");
  
  // Configure SSL context 0 to use TLS 1.2 (3) or ANY (0)
  // A7670 might need this
  modem.sendAT("+CSSLCFG=\"sslversion\",0,3"); 
  modem.waitResponse();
  
  modem.sendAT("+CIPSSL=1");
  if (modem.waitResponse() != 1) {
     Serial.println("[Weather] Warning: AT+CIPSSL=1 failed or ignored");
  }
  
  // Connect to port 443 for HTTPS
  if (!client.connect(host.c_str(), 443)) {
    s_lastError = "LTE HTTPS connect failed";
    Serial.println("[Weather] LTE HTTPS connection to server failed");
    return false;
  }
  
  Serial.println("[Weather] Connected, sending HTTP GET...");
  
  // Send HTTP GET request
  client.print(String("GET ") + path + " HTTP/1.1\r\n");
  client.print(String("Host: ") + host + "\r\n");
  client.print("Connection: close\r\n\r\n");
  
  // Wait for response
  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 15000) {
    if (client.available()) break;
    delay(10);
  }
  
  if (!client.available()) {
    s_lastError = "LTE timeout";
    Serial.println("[Weather] LTE response timeout");
    client.stop();
    return false;
  }
  
  Serial.println("[Weather] Reading response headers...");
  
  // Skip HTTP headers - look for empty line (just \r\n)
  bool headersEnd = false;
  String line;
  while (client.connected() || client.available()) {
    if (!client.available()) {
      delay(10);
      continue;
    }
    
    line = client.readStringUntil('\n');
    line.trim();  // Remove \r and whitespace
    
    Serial.print("[Weather] Header: ");
    Serial.println(line.length() == 0 ? "(empty - end of headers)" : line.substring(0, min(50, (int)line.length())));
    
    if (line.length() == 0) {
      // Empty line = end of headers
      headersEnd = true;
      break;
    }
  }
  
  if (!headersEnd) {
    s_lastError = "Invalid HTTP response";
    Serial.println("[Weather] Invalid HTTP response - headers not found");
    client.stop();
    return false;
  }
  
  // Read JSON body
  Serial.println("[Weather] Reading JSON body...");
  String body = "";
  body.reserve(4096);  // Pre-allocate for efficiency
  
  while (client.connected() || client.available()) {
    if (client.available()) {
      char c = client.read();
      body += c;
    } else {
      delay(10);
    }
    
    // Safety check - stop if body gets too large
    if (body.length() > 30000) {
      Serial.println("[Weather] Body too large, stopping read");
      break;
    }
  }
  
  client.stop();
  
  Serial.print("[Weather] JSON length: ");
  Serial.println(body.length());
  
  if (body.length() < 100) {
    s_lastError = "Empty response";
    Serial.println("[Weather] Response too short");
    return false;
  }
  
  // Parse JSON (same as WiFi version)
  const size_t CAP = 28 * 1024;
  DynamicJsonDocument doc(CAP);
  DeserializationError derr = deserializeJson(doc, body);
  
  if (derr) {
    s_lastError = String("JSON parse: ") + String(derr.c_str());
    Serial.print("[Weather] JSON parse error: ");
    Serial.println(derr.c_str());
    return false;
  }
  
  // Parse hourly data (same logic as WiFi version)
  JsonObject hourly = doc["hourly"];
  if (!hourly) {
    s_lastError = "No hourly data";
    Serial.println("[Weather] No hourly object");
    return false;
  }
  
  JsonArray times = hourly["time"];
  JsonArray temps = hourly["temperature_2m"];
  JsonArray codes = hourly["weathercode"];
  JsonArray hums  = hourly["relativehumidity_2m"];
  JsonArray press = hourly["surface_pressure"];
  
  if (!times || !temps || !codes) {
    s_lastError = "Missing arrays";
    Serial.println("[Weather] Missing required arrays");
    return false;
  }
  
  int total = times.size();
  Serial.print("[Weather] Total hourly samples: ");
  Serial.println(total);
  
  if (total == 0) {
    s_lastError = "No samples";
    return false;
  }
  
  // Sample every 6 hours
  const int maxSamples = 12;
  if (s_days) delete[] s_days;
  s_days = new WeatherDay[maxSamples];
  s_daysCount = 0;
  
  int samples = 0;
  for (int i = 0; i < total && samples < maxSamples; i += 6) {
    const char* tstr = times[i];
    double temp = temps[i];
    int wc = codes[i];
    double hum = (hums && i < (int)hums.size()) ? (double)hums[i] : NAN;
    double pr  = (press && i < (int)press.size()) ? (double)press[i] : NAN;
    
    if (!tstr) continue;
    
    char dbuf[32];
    snprintf(dbuf, sizeof(dbuf), "%s", tstr);
    s_days[samples].date = String(dbuf);
    s_days[samples].temp_min = (float)temp;
    s_days[samples].temp_max = (float)temp;
    s_days[samples].humidity = isnan(hum) ? 0.0f : (float)hum;
    s_days[samples].pressure = isnan(pr) ? 0.0f : (float)pr;
    s_days[samples].desc = mapWeatherCodeOpenMeteo(wc);
    samples++;
  }
  
  if (samples == 0) {
    s_lastError = "No samples parsed";
    Serial.println("[Weather] No samples parsed");
    delete[] s_days;
    s_days = nullptr;
    s_daysCount = 0;
    s_hasData = false;
    return false;
  }
  
  s_daysCount = samples;
  s_hasData = true;
  Serial.print("[Weather] LTE fetch successful, samples=");
  Serial.println(s_daysCount);
  return true;
}

bool weather_fetch() {
#if USE_OPENMETEO
  // Check which network is active and choose appropriate method
  bool wifiOK = (WiFi.status() == WL_CONNECTED);
  
  Serial.println("[Weather] weather_fetch() called");
  Serial.print("[Weather] WiFi: ");
  Serial.println(wifiOK ? "connected" : "disconnected");
  
  if (wifiOK) {
    // Only use WiFi for weather fetch
    Serial.println("[Weather] Using WiFi for weather fetch");
    bool success = weather_fetch_open_meteo();
    
    if (success) {
      Serial.print("[Weather] Fetch successful, ");
      Serial.print(s_daysCount);
      Serial.println(" samples");
    } else {
      Serial.print("[Weather] Fetch failed: ");
      Serial.println(s_lastError);
    }
    return success;
  } else {
    s_lastError = "WiFi not connected (LTE weather disabled)";
    Serial.println("[Weather] WiFi not connected - skipping weather fetch");
    return false;
  }
#else
  s_lastError = "OpenMeteo disabled in config.h";
  Serial.println("[Weather] No provider enabled");
  return false;
#endif
}

bool weather_hasData() { return s_hasData; }
int weather_daysCount() { return s_daysCount; }

void weather_getDay(int idx, WeatherDay &out) {
  if (!s_hasData || idx < 0 || idx >= s_daysCount) {
    out.date = String("--");
    out.temp_min = 0.0f;
    out.temp_max = 0.0f;
    out.humidity = 0.0f;
    out.pressure = 0.0f;
    out.desc = String("N/A");
    return;
  }
  out = s_days[idx];
}