#include "lcd_endpoint.h"
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "lcd_server_simple.h"
#include "provisioning_server.h"
#include <WiFi.h>

static String html_page = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Beehive Monitor - LCD & Provision</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  :root{
    --bg:#f3f3f3;
    --panel:#ffffff;
    --muted:#666;
    --accent:#0078D4;
    --accent-hover:#005a9e;
    --input-bg:#ffffff;
    --input-border:#cfcfcf;
    --lcd-bg: rgb(50,50,255);
    --lcd-line:#ffffff;
    --radius:6px;
  }
  html,body{height:100%; margin:0; font-family: "Segoe UI", Tahoma, Arial, system-ui, -apple-system, sans-serif; background:var(--bg); color:#111;}
  .container{max-width:920px; margin:18px auto; padding:18px;}
  h1{font-size:20px; margin:0 0 12px 0; font-weight:700;}
  .panel{background:var(--panel); border-radius:var(--radius); padding:18px; box-shadow:0 1px 4px rgba(0,0,0,0.08); margin-bottom:14px;}
  .sectionTitle{font-weight:700; font-size:14px; margin-bottom:10px; color:#222;}

  form#wloc, form#wifi { display:grid; grid-template-columns: 180px 1fr 220px; grid-column-gap:12px; align-items:center; }
  form#wloc .label, form#wifi .label { text-align:right; padding-right:8px; font-weight:700; color:#222; font-size:15px; }
  form#wloc input[type="text"], form#wifi input[type="text"], form#wifi input[type="password"] {
    height:40px; padding:8px 10px; font-size:15px; border:1px solid var(--input-border); border-radius:4px; background:var(--input-bg);
  }
  form#wloc .controls, form#wifi .controls { display:flex; gap:10px; align-items:center; justify-self:start; grid-column: 3 / 4; }
  button.btn { height:40px; padding:0 14px; font-size:15px; border-radius:4px; border: none; background:var(--accent); color:#fff; cursor:pointer; }
  button.secondary { background:#e1e1e1; color:#111; }
  button.btn:hover{ background:var(--accent-hover); }
  .hint { grid-column: 1 / 4; font-size:13px; color:var(--muted); margin-top:10px; }

  .status { margin-top:8px; font-size:13px; color:#006600; }

  /* LCD emulator (4x20 / up to 4 lines) */
  .lcd-wrap { max-width:100%; margin:18px auto; display:flex; flex-direction:column; align-items:center; }
  .lcd { width:560px; max-width:100%; background:var(--lcd-bg); color:var(--lcd-line); border-radius:8px; padding:18px;
         box-shadow: inset 0 0 12px rgba(0,0,0,0.6), 0 6px 20px rgba(0,0,0,0.12); border: 1px solid rgba(255,255,255,0.06); }
  .lcd .row { font-family: Arial, 'Courier New', monospace; font-weight:700; font-size:28px; line-height:1.0; height:36px; white-space:pre; overflow:hidden; display:block; color:var(--lcd-line); }
  .lcd .labelRow { font-size:12px; color:#dfe; margin-top:8px; text-align:center; }

  @media (max-width:820px){
    form#wloc, form#wifi{ grid-template-columns: 120px 1fr; grid-template-rows: auto auto; }
    .lcd { padding:12px; }
    .lcd .row { font-size:20px; height:28px; }
  }

  .footer { font-size:12px; color:var(--muted); margin-top:6px; text-align:center; }
</style>
</head>
<body>
<div class="container">
  <h1>Beehive Monitor — Provision & LCD Mirror</h1>

  <div class="panel" aria-labelledby="locTitle">
    <div class="sectionTitle" id="locTitle">Location for Weather</div>

    <form id="wloc" onsubmit="return submitLoc();">
      <div class="label"><label for="city">City</label></div>
      <div><input id="city" name="city" type="text" required placeholder="Athens"></div>
      <div class="controls">
        <button class="btn" type="submit">Save &amp; Geocode</button>
      </div>

      <div class="label"><label for="country">Country</label></div>
      <div><input id="country" name="country" type="text" placeholder="Greece"></div>
      <div></div>

      <div class="hint">Enter a city (required). Country is optional but helps geocoding accuracy. The device will store the city/country and (when available) latitude/longitude for the Weather menu.</div>
    </form>

    <div id="locStatus" class="status" aria-live="polite"></div>
  </div>

  <div class="panel" aria-labelledby="wifiTitle">
    <div class="sectionTitle" id="wifiTitle">Wi‑Fi Credentials (up to 2 networks)</div>

    <form id="wifi" onsubmit="return submitWifi();">
      <div class="label"><label for="ssid1">SSID 1</label></div>
      <div><input id="ssid1" name="ssid1" type="text" placeholder="MyHomeSSID"></div>
      <div class="controls">
        <button class="btn" type="submit">Save Wi‑Fi</button>
      </div>

      <div class="label"><label for="pass1">Password 1</label></div>
      <div><input id="pass1" name="pass1" type="password" placeholder="••••••••"></div>
      <div></div>

      <div class="label"><label for="ssid2">SSID 2</label></div>
      <div><input id="ssid2" name="ssid2" type="text" placeholder="OfficeSSID"></div>
      <div class="controls">
        <button class="secondary" type="button" onclick="fillFromDevice()">Load</button>
      </div>

      <div class="label"><label for="pass2">Password 2</label></div>
      <div><input id="pass2" name="pass2" type="password" placeholder="••••••••"></div>
      <div></div>

      <div class="hint">Provide up to two SSIDs and passwords. The device will store them securely in preferences. Use "Load" to fetch stored values from device (if any).</div>
    </form>

    <div id="wifiStatus" class="status" aria-live="polite"></div>
  </div>

  <div class="panel">
    <div class="sectionTitle">LCD Mirror (4x20)</div>
    <div class="lcd-wrap">
      <div class="lcd" id="lcdScreen" role="region" aria-label="LCD mirror">
        <div class="row" id="r0">                    </div>
        <div class="row" id="r1">                    </div>
        <div class="row" id="r2">                    </div>
        <div class="row" id="r3">                    </div>
      </div>
      <div class="labelRow">Device LCD 4 × 20 — updates every 250 ms</div>
      <div class="labelRow">Manos Mar 2025</div>
    </div>
  </div>

  <div class="footer">Connected to device: <span id="ipinfo"></span></div>
</div>

<script>
const ipEl = document.getElementById('ipinfo');
if (location && location.host) ipEl.textContent = location.host;

async function submitLoc() {
  const city = document.getElementById('city').value.trim();
  const country = document.getElementById('country').value.trim();
  const status = document.getElementById('locStatus');
  status.textContent = 'Saving...';
  try {
    const form = new URLSearchParams();
    form.append('city', city);
    form.append('country', country);
    const res = await fetch('/set_location', { method: 'POST', body: form });
    const j = await res.json();
    if (j.ok) status.textContent = 'Saved: ' + (j.loc || '');
    else status.textContent = 'Failed: ' + (j.error || 'unknown');
  } catch (e) {
    status.textContent = 'Error: ' + e;
  }
  setTimeout(()=> status.textContent='', 4000);
  return false;
}

async function submitWifi() {
  const ssid1 = document.getElementById('ssid1').value.trim();
  const pass1 = document.getElementById('pass1').value;
  const ssid2 = document.getElementById('ssid2').value.trim();
  const pass2 = document.getElementById('pass2').value;
  const status = document.getElementById('wifiStatus');
  status.textContent = 'Saving...';
  try {
    const form = new URLSearchParams();
    form.append('ssid1', ssid1);
    form.append('pass1', pass1);
    form.append('ssid2', ssid2);
    form.append('pass2', pass2);
    const res = await fetch('/set_wifi', { method: 'POST', body: form });
    const j = await res.json();
    if (j.ok) status.textContent = 'Wi‑Fi saved';
    else status.textContent = 'Failed: ' + (j.error || 'unknown');
  } catch (e) {
    status.textContent = 'Error: ' + e;
  }
  setTimeout(()=> status.textContent='', 4000);
  return false;
}

async function fillFromDevice() {
  try {
    const r = await fetch('/wifi.json?ts=' + Date.now(), { cache: 'no-cache' });
    if (r.ok) {
      const j = await r.json();
      if (j.ssid1) document.getElementById('ssid1').value = j.ssid1;
      if (j.ssid2) document.getElementById('ssid2').value = j.ssid2;
    }
  } catch(e) { /* ignore */ }
}

function cleanLine(s) {
  if (!s) return '';
  return s.replace(/[^\x20-\x7E]/g,'');
}

async function pollLCD(){
  try {
    const r = await fetch('/lcd.json?ts=' + Date.now(), { cache: 'no-cache' });
    if (r.ok){
      const j = await r.json();
      // support multiple shapes
      let arr = [];
      if (Array.isArray(j.lines)) arr = j.lines.slice();
      else if (typeof j.line0 !== 'undefined' || typeof j.line1 !== 'undefined') {
        arr[0] = j.line0 || '';
        arr[1] = j.line1 || '';
      } else if (j.lines && typeof j.lines === 'string') {
        arr = j.lines.split(/\r?\n/);
      }
      // normalize to 4 lines
      for (let i=0;i<4;i++) {
        let v = (arr[i]||'').toString();
        v = cleanLine(v);
        if (v.length > 20) v = v.substring(0,20);
        while (v.length < 20) v += ' ';
        document.getElementById('r' + i).textContent = v;
      }
    }
  } catch(e){
    // ignore transient errors
  }
}

setInterval(pollLCD, 250);
pollLCD();
</script>
</body>
</html>
)rawliteral";

static String urlEncode(const String &str) {
  String encoded = "";
  char buf[8];
  for (size_t i = 0; i < str.length(); ++i) {
    char c = str[i];
    if ( ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c=='-' || c=='_' || c=='.' || c=='~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      snprintf(buf, sizeof(buf), "%%%02X", (uint8_t)c);
      encoded += buf;
    }
  }
  return encoded;
}

void lcd_register(WebServer &server) {
  // Root UI page
  server.on("/", HTTP_GET, [&server]() {
    server.send(200, "text/html", html_page);
  });

  // LCD JSON (polling)
  server.on("/lcd.json", HTTP_GET, [&server]() {
    String l0 = lcd_get_line_simple(0);
    String l1 = lcd_get_line_simple(1);
    // ensure 20 chars
    while (l0.length() < 20) l0 += ' ';
    while (l1.length() < 20) l1 += ' ';
    if (l0.length() > 20) l0 = l0.substring(0,20);
    if (l1.length() > 20) l1 = l1.substring(0,20);

    String js = "{";
    js += "\"line0\":\"" + l0 + "\",";
    js += "\"line1\":\"" + l1 + "\"";
    js += "}";
    server.send(200, "application/json", js);
  });

  // WiFi JSON (read-only safe view)
  server.on("/wifi.json", HTTP_GET, [&server]() {
    Preferences pref;
    pref.begin("beehive", true);
    String s1 = pref.getString("ssid1", "");
    String s2 = pref.getString("ssid2", "");
    pref.end();
    String js = "{";
    js += "\"ssid1\":\"" + s1 + "\",";
    js += "\"ssid2\":\"" + s2 + "\"";
    js += "}";
    server.send(200, "application/json", js);
  });

  // Set location: POST form fields 'city' and 'country'
  server.on("/set_location", HTTP_POST, [&server]() {
    String city = server.arg("city");
    String country = server.arg("country");

    if (city.length() < 1) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"city required\"}");
      return;
    }

    // Attempt geocoding via Nominatim (OpenStreetMap)
    String lat = "";
    String lon = "";
    String displayName = city;
    if (country.length()) displayName += ", " + country;

    HTTPClient http;
    String url = "https://nominatim.openstreetmap.org/search?format=json&limit=1&q=" + urlEncode(city + (country.length() ? (", " + country) : ""));
    bool geoOk = false;
    if (WiFi.status() == WL_CONNECTED) {
      http.begin(url);
      http.setUserAgent("BeehiveMonitor/1.0 (+https://example.local)");
      int code = http.GET();
      if (code == 200) {
        String payload = http.getString();
        int pLat = payload.indexOf("\"lat\"");
        int pLon = payload.indexOf("\"lon\"");
        if (pLat >= 0) {
          int q1 = payload.indexOf('"', pLat + 5);
          int q2 = payload.indexOf('"', q1 + 1);
          if (q1 >= 0 && q2 > q1) {
            String tmp = payload.substring(q1+1, q2);
            tmp.trim();
            lat = tmp;
          }
        }
        if (pLon >= 0) {
          int q1 = payload.indexOf('"', pLon + 5);
          int q2 = payload.indexOf('"', q1 + 1);
          if (q1 >= 0 && q2 > q1) {
            String tmp = payload.substring(q1+1, q2);
            tmp.trim();
            lon = tmp;
          }
        }
        if (lat.length() && lon.length()) geoOk = true;
      }
      http.end();
    }

    // Save preferences
    Preferences pref;
    pref.begin("beehive", false);
    pref.putString("loc_name", city);
    pref.putString("loc_country", country);
    if (geoOk) {
      pref.putString("owm_lat", lat);
      pref.putString("owm_lon", lon);
    } else {
      pref.remove("owm_lat");
      pref.remove("owm_lon");
    }
    pref.end();

    String out = "{ \"ok\": true, \"loc\": \"" + displayName + "\"";
    if (geoOk) out += ", \"lat\":\"" + lat + "\", \"lon\":\"" + lon + "\"";
    out += " }";
    server.send(200, "application/json", out);
  });

  // Set Wi-Fi credentials: POST ssid1,pass1,ssid2,pass2
  server.on("/set_wifi", HTTP_POST, [&server]() {
    String ssid1 = server.arg("ssid1");
    String pass1 = server.arg("pass1");
    String ssid2 = server.arg("ssid2");
    String pass2 = server.arg("pass2");

    if (ssid1.length() < 1 && ssid2.length() < 1) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"no ssid provided\"}");
      return;
    }

    Preferences pref;
    pref.begin("beehive", false);
    if (ssid1.length()) pref.putString("ssid1", ssid1);
    if (pass1.length()) pref.putString("pass1", pass1);
    if (ssid2.length()) pref.putString("ssid2", ssid2);
    if (pass2.length()) pref.putString("pass2", pass2);
    pref.end();

    String out = "{ \"ok\": true }";
    server.send(200, "application/json", out);
  });

  // Optional: fallback 404 for unknown
  server.onNotFound([&server]() {
    server.send(404, "text/plain", "Not found");
  });
}
