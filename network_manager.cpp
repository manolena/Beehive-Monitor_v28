#include "network_manager.h"
#include "config.h"
#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include "modem_manager.h"
#include "key_server.h"

extern WebServer server;

static const char* PREF_APP_NS = "beehive_app";
static const char* PREF_WIFI_NS = "beehive";

// Network mode enum mirrors CONNECTIVITY_* in config.h
enum NetMode { NET_NONE = 0, NET_LTE = 1, NET_WIFI = 2 };

static NetMode currentNet = NET_NONE;
int connectivityMode = CONNECTIVITY_OFFLINE; // Global definition
static int net_pref = 0;
static unsigned long lastAutoTry = 0;

// Block automatic switches for a short time after user action (ms)
static unsigned long userActionBlockUntil = 0;
#define USER_ACTION_BLOCK_MS 15000

bool isUserActive() {
  return (millis() < userActionBlockUntil);
}

// If true indicates user explicitly forced a preference and we should
// avoid auto logic that would override it. Persisted across reboots.
static bool user_forced_net = false;

static void persistUserForcedFlag(bool v) {
  Preferences p; p.begin(PREF_APP_NS, false);
  p.putBool("net_forced", v);
  p.end();
  user_forced_net = v;
}

static void loadUserForcedFlag() {
  Preferences p; p.begin(PREF_APP_NS, true);
  user_forced_net = p.getBool("net_forced", false);
  p.end();
}

// New: suppression for modem auto attach (set by UI actions)
static unsigned long modemSuppressUntil = 0;

void network_suppress_modem_attach_ms(unsigned long ms) {
  modemSuppressUntil = millis() + ms;
  Serial.printf("[NET] modem auto-attach suppressed for %lums\n", ms);
}

bool wifi_connectFromPrefs(unsigned long timeoutMs) {
  Preferences p; p.begin(PREF_WIFI_NS, true);
  String ssid1 = p.getString("wifi_ssid1", "");
  String psk1  = p.getString("wifi_psk1", "");
  String ssid2 = p.getString("wifi_ssid2", "");
  String psk2  = p.getString("wifi_psk2", "");
  p.end();

  auto tryConnect = [&](const String &ssid, const String &psk)->bool {
    if (ssid.length() == 0) return false;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), psk.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
      server.handleClient();  // Keep web server responsive
      delay(50);              // Reduced from 200ms to minimize blocking
    }
    if (WiFi.status() == WL_CONNECTED) {
      currentNet = NET_WIFI;
      connectivityMode = CONNECTIVITY_WIFI;
      Serial.println(F("[NET] WiFi connected from prefs"));
      return true;
    }
    return false;
  };

  if (tryConnect(ssid1, psk1)) return true;
  if (tryConnect(ssid2, psk2)) return true;
  return false;
}

static bool tryStartLTE_internal() {
  unsigned long now = millis();

  // Respect explicit suppression window
  if (now < modemSuppressUntil) {
    Serial.printf("[NET] tryStartLTE_internal: suppressed (remaining=%lums)\n", modemSuppressUntil - now);
    return false;
  }

  // Respect explicit user-forced WiFi preference
  if (user_forced_net && net_pref == CONNECTIVITY_WIFI) {
    Serial.println(F("[NET] tryStartLTE_internal: suppressed by user_forced_net && pref=WiFi"));
    return false;
  }

  TinyGsm &modem = modem_get();
  Serial.println(F("[LTE] Attempting GPRS attach (internal)"));
#if defined(MODEM_APN) && defined(MODEM_GPRS_USER) && defined(MODEM_GPRS_PASS)
  bool ok = modem.gprsConnect(MODEM_APN, MODEM_GPRS_USER, MODEM_GPRS_PASS);
#else
  Serial.println(F("[LTE] MODEM_APN/MODEM_GPRS_* not defined in config.h"));
  return false;
#endif

  if (ok) {
    Serial.println(F("[LTE] GPRS attach OK"));
    keyServer_stop();
    currentNet = NET_LTE;
    connectivityMode = CONNECTIVITY_LTE;
    return true;
  } else {
    Serial.println(F("[LTE] GPRS attach failed"));
    currentNet = NET_NONE;
    connectivityMode = CONNECTIVITY_OFFLINE;
    return false;
  }
}

void tryStartLTE() {
  (void) tryStartLTE_internal();
}

void network_init() {
  Preferences p; p.begin(PREF_APP_NS, false);
  net_pref = p.getInt("net_pref", 0);
  p.end();
  loadUserForcedFlag();
  Serial.printf("[NET] init net_pref=%d user_forced=%d\n", net_pref, user_forced_net ? 1 : 0);
}

int getNetworkPreference() {
  return net_pref;
}

void clearNetworkUserBlock() {
  userActionBlockUntil = 0;
  persistUserForcedFlag(false);
  Serial.println(F("[NET] cleared user block and forced flag"));
}

void setNetworkPreference(int newPref) {
  if (newPref < 0 || newPref > 2) return;

  // If unchanged, do nothing except refresh user-action block and keep forced flag
  if (newPref == net_pref) {
    Serial.println(F("[NET] Preference unchanged - refreshing user-action block"));
    userActionBlockUntil = millis() + USER_ACTION_BLOCK_MS;
    persistUserForcedFlag(true);
    
    // Reaffirmation logic:
    if (net_pref == CONNECTIVITY_WIFI) {
      // Extended suppression and attempt immediate WiFi connect to honor user's reaffirmation.
      network_suppress_modem_attach_ms(120000);
      Serial.println(F("[NET] Preference reaffirmed (WiFi) - attempting immediate WiFi connect"));
      // If modem is registered, disconnect GPRS before trying WiFi to avoid races.
      if (modem_isNetworkRegistered()) {
        Serial.println(F("[NET] Disconnecting modem GPRS before WiFi attempt"));
        modem_get().gprsDisconnect();
        delay(200);
      }
      bool ok = wifi_connectFromPrefs(10000);
      if (ok) Serial.println(F("[NET] WiFi connected per user reaffirm"));
      else Serial.println(F("[NET] Immediate WiFi connect failed after reaffirm"));
    } else if (net_pref == CONNECTIVITY_LTE) {
      Serial.println(F("[NET] Preference reaffirmed (LTE) - ensuring WiFi OFF and LTE ON"));
      // Ensure WiFi is off for LTE-only mode
      if (WiFi.status() == WL_CONNECTED || WiFi.mode(WIFI_MODE_NULL) != WIFI_OFF) {
         WiFi.disconnect(true);
         WiFi.mode(WIFI_OFF);
         delay(100);
      }
      // Ensure LTE is on
      if (!modem_isNetworkRegistered()) {
         tryStartLTE_internal();
      }
    }
    return;
  }

  // persist preference
  net_pref = newPref;
  Preferences p; p.begin(PREF_APP_NS, false);
  p.putInt("net_pref", net_pref);
  p.end();

  // block auto switching briefly to allow user action to settle
  userActionBlockUntil = millis() + USER_ACTION_BLOCK_MS;
  // mark that user explicitly forced the current preference
  persistUserForcedFlag(true);
  Serial.printf("[NET] User set network preference=%d - blocking auto-switch briefly, user_forced=1\n", net_pref);

  // Also suppress modem auto-attach for a while to avoid races
  network_suppress_modem_attach_ms(60000);

  // WIFI chosen explicitly
  if (net_pref == CONNECTIVITY_WIFI) {
    Preferences wp; wp.begin(PREF_WIFI_NS, true);
    String ssid1 = wp.getString("wifi_ssid1", "");
    wp.end();
    if (ssid1.length() > 0) {
      Serial.println(F("[NET] User chose WiFi - attempting connect from saved prefs"));
      // ensure modem GPRS is disconnected before attempting WiFi
      if (modem_isNetworkRegistered()) {
        Serial.println(F("[NET] disconnecting GPRS before WiFi attempt"));
        modem_get().gprsDisconnect();
        delay(200);
      }
      bool ok = wifi_connectFromPrefs(10000);
      if (!ok) {
        Serial.println(F("[NET] WiFi connect from prefs failed - staying on current network until user retries/provisions"));
      } else {
        Serial.println(F("[NET] WiFi connected per user request"));
      }
    } else {
      Serial.println(F("[NET] WiFi selected but no saved credentials; waiting for provisioning"));
    }
    return;
  }

  // LTE chosen explicitly
  if (net_pref == CONNECTIVITY_LTE) {
    Serial.println(F("[NET] User chose LTE - LTE-only mode (remote operation)"));
    // Turn off WiFi for LTE-only remote operation
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    bool ok = tryStartLTE_internal();
    if (!ok) {
      Serial.println(F("[NET] LTE requested but attach failed"));
    } else {
      Serial.println(F("[NET] LTE-only mode active - weather via LTE, no web interface"));
    }
    
    // clearing forced flag if user explicitly requested LTE (we still persist pref)
    persistUserForcedFlag(false);
    return;
  }

  // OFFLINE (or other): reset timers and keep both off
  lastAutoTry = 0;
  currentNet = NET_NONE;
  connectivityMode = CONNECTIVITY_OFFLINE;
  if (WiFi.status() == WL_CONNECTED) { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); }
  if (modem_isNetworkRegistered()) modem_get().gprsDisconnect();
  Serial.println(F("[NET] Network set to OFFLINE by user"));
  persistUserForcedFlag(false);
}

void manageAutoNetwork() {
  unsigned long now = millis();

  // If user recently changed preference, avoid auto switching for a short period
  if (now < userActionBlockUntil) {
    // Still log for diagnostics
    Serial.printf("[NET] manageAutoNetwork: blocked by userActionBlockUntil (remaining=%lums)\n", userActionBlockUntil - now);
    return;
  }

  // If modem suppression is active, avoid any LTE attach
  if (now < modemSuppressUntil) {
    Serial.printf("[NET] manageAutoNetwork: modem attach suppressed (remaining=%lums)\n", modemSuppressUntil - now);
    // still attempt WiFi only if pref requires it
    if (net_pref == CONNECTIVITY_WIFI || (user_forced_net && net_pref == CONNECTIVITY_WIFI)) {
      if (currentNet != NET_WIFI && now - lastAutoTry > 10000) {
        lastAutoTry = now;
        Serial.println(F("[NET] suppressed-mode: trying WiFi only"));
        wifi_connectFromPrefs(8000);
      }
    }
    return;
  }

  // If user has forced WiFi, avoid any LTE automatic attach that would override it
  if (user_forced_net && net_pref == CONNECTIVITY_WIFI) {
    // If WiFi disconnected, try only WiFi; do NOT start LTE
    if (currentNet != NET_WIFI && now - lastAutoTry > 10000) {
      lastAutoTry = now;
      Serial.println(F("[NET] user_forced_net=1 and pref=WiFi -> trying WiFi only"));
      wifi_connectFromPrefs(8000);
    }
    return;
  }

  if (net_pref == CONNECTIVITY_WIFI) {
    if (currentNet != NET_WIFI && now - lastAutoTry > 10000) {
      lastAutoTry = now;
      wifi_connectFromPrefs(8000);
    }
    return;
  }
  if (net_pref == CONNECTIVITY_LTE) {
    if (currentNet != NET_LTE && now - lastAutoTry > 10000) {
      lastAutoTry = now;
      tryStartLTE();
    }
    return;
  }

  // net_pref == AUTO/OFFLINE (original behavior)
  if (WiFi.status() == WL_CONNECTED) {
    if (currentNet != NET_WIFI) {
      currentNet = NET_WIFI;
      connectivityMode = CONNECTIVITY_WIFI;
      if (modem_isNetworkRegistered()) modem_get().gprsDisconnect();
      delay(100);
    }
    return;
  }

  if (currentNet != NET_LTE) {
    if (now - lastAutoTry > 20000) {
      lastAutoTry = now;
      tryStartLTE();
    }
  } else {
    if (now - lastAutoTry > 15000) {
      lastAutoTry = now;
      if (wifi_connectFromPrefs(5000)) {
        if (modem_isNetworkRegistered()) modem_get().gprsDisconnect();
        delay(100);
      }
    }
  }
}