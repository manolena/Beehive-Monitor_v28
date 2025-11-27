// provisioning_server.cpp - provides /provisioning.html (form) and /lcd.json (live mirror)

#include "provisioning_server.h"
#include <Preferences.h>
#include "lcd_server_simple.h"
#include <Arduino.h>

static const char* PREF_WIFI_NS = "beehive";

// Simple provisioning HTML (mobile form)
static void handleHtmlForm(WebServer &server) {
  static const char html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta charset="utf-8">
  <title>Provisioning</title>
  <style>body{font-family:Arial;padding:12px;background:#f7f7f7}.card{background:#fff;padding:12px;border-radius:8px}</style>
</head>
<body>
  <div class="card">
    <h3>Provisioning (Weather)</h3>
    <form method="post" action="/provisioning.html" enctype="application/x-www-form-urlencoded">
      <label>City / Location</label><input name="loc_name" placeholder="Athens" /><br/>
      <label>Country (ISO)</label><input name="loc_country" placeholder="GR" /><br/>
      <label>Latitude</label><input name="owm_lat" placeholder="37.98" /><br/>
      <label>Longitude</label><input name="owm_lon" placeholder="23.72" /><br/>
      <button type="submit">Save</button>
    </form>
    <hr/>
    <small>Tip: leave lat/lon empty to use geocoding from city name (if supported).</small>
  </div>
</body>
</html>
  )rawliteral";

  server.sendHeader("Content-Type", "text/html; charset=utf-8");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", html);
}

// Simple HTML form POST handler: stores fields into Preferences
static void handleHtmlFormPost(WebServer &server) {
  bool any = false;
  Preferences p; p.begin(PREF_WIFI_NS, false);
  if (server.hasArg("loc_name")) { p.putString("loc_name", server.arg("loc_name")); any = true; }
  if (server.hasArg("loc_country")) { p.putString("loc_country", server.arg("loc_country")); any = true; }
  if (server.hasArg("owm_lat")) { p.putString("owm_lat", server.arg("owm_lat")); any = true; }
  if (server.hasArg("owm_lon")) { p.putString("owm_lon", server.arg("owm_lon")); any = true; }
  p.end();

  if (any) {
    server.sendHeader("Content-Type", "text/html; charset=utf-8");
    server.send(200, "text/html", "<html><body>Saved. <a href=\"/provisioning.html\">Back</a></body></html>");
  } else {
    server.send(400, "text/html", "<html><body>No fields provided</body></html>");
  }
}

// Build /lcd.json response with mirror lines and timestamp
static void handleLcdJson(WebServer &server) {
  String resp;
  resp.reserve(256);
  resp += "{";
  resp += "\"lines\":[";
  for (int i = 0; i < 4; ++i) {
    String L = lcd_get_line_simple(i);
    while (L.length() < 20) L += ' ';
    L.replace("\\", "\\\\");
    L.replace("\"", "\\\"");
    resp += "\"" + L + "\"";
    if (i < 3) resp += ",";
  }
  resp += "],";
  resp += "\"ts\":"; resp += String(millis());
  resp += "}";

  server.sendHeader("Content-Type", "application/json; charset=utf-8");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", resp);
}

void provisioning_register(WebServer &server) {
  server.on("/provisioning.html", HTTP_GET, [&server]() { handleHtmlForm(server); });
  server.on("/provisioning.html", HTTP_POST, [&server]() { handleHtmlFormPost(server); });

  // lcd mirror JSON
  server.on("/lcd.json", HTTP_GET, [&server]() { handleLcdJson(server); });

  Serial.println("[PROVISION] routes registered (html + lcd.json)");
}