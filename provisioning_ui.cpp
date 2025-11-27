#include "provisioning_ui.h"
#include "network_manager.h"
#include "config.h"
#include "ui.h"
#include "menu_manager.h"
#include "text_strings.h"
#include <Preferences.h>

// Namespace used elsewhere in network_manager
static const char *PREF_WIFI_NS = "beehive";

// ------------------------------------------------------------------
// Simple UI entrypoint used by menu_manager: ask user to open web UI
// ------------------------------------------------------------------
void provisioning_ui_enterCityCountry() {
  uiClear();
  if (currentLanguage == LANG_EN) {
    uiPrint(0,0,"OPEN BROWSER TO      ");
    uiPrint(0,1,"http://192.168.1.10:8080/");
    uiPrint(0,2,"ENTER CRED THEN SAVE ");
    uiPrint(0,3,getTextEN(TXT_BACK_SMALL));
  } else {
    lcdPrintGreek_P(F("ΑΝΟΙΞΤΕ BROWSER:"), 0, 0);
    lcdPrintGreek("http://192.168.1.10:8080/", 0, 1);
    lcdPrintGreek_P(F("ΕΙΣΑΓΕΤΕ ΚΑΙ SAVE"), 0, 2);
    lcdPrintGreek(getTextGR(TXT_BACK_SMALL), 0, 3);
  }

  // Wait for user to press BACK/SELECT to return to menu
  while (true) {
    Button b = getButton();
    if (b == BTN_BACK_PRESSED || b == BTN_SELECT_PRESSED) {
      menuDraw();
      return;
    }
    delay(80);
  }
}

// ------------------------------------------------------------------
// provisioning_saveWifiCredentials
// Saves credentials into Preferences (write), logs and triggers network action.
// Returns true on success.
// ------------------------------------------------------------------
bool provisioning_saveWifiCredentials(const String &ssid, const String &psk) {
  // Basic validation
  if (ssid.length() == 0) {
    Serial.println(F("[PROV] Save failed: empty SSID"));
    return false;
  }

  // Persist credentials in the same namespace that network_manager expects
  Preferences p;
  p.begin(PREF_WIFI_NS, false); // writeable
  p.putString("wifi_ssid1", ssid);
  p.putString("wifi_psk1", psk);
  p.end();

  // Diagnostic log (mask psk)
  String masked;
  if (psk.length() <= 2) masked = String("<len=") + psk.length() + ">";
  else {
    masked = psk.substring(0,1);
    for (size_t i = 1; i + 1 < psk.length(); ++i) masked += '*';
    masked += psk.substring(psk.length()-1);
  }

  Serial.printf("[PROV] WiFi creds saved: ssid='%s' psk='%s'\n", ssid.c_str(), masked.c_str());

  // Trigger network preference -> this will persist pref, suppress modem attach and attempt WiFi connect
  setNetworkPreference(CONNECTIVITY_WIFI);

  // Also attempt an immediate connect (best-effort) and report
  bool ok = wifi_connectFromPrefs(8000);
  if (ok) {
    Serial.println(F("[PROV] Immediate WiFi connect SUCCESS after save"));
  } else {
    Serial.println(F("[PROV] Immediate WiFi connect FAILED after save"));
  }

  return ok;
}

bool provisioning_saveWifiCredentials_c(const char *ssid, const char *psk) {
  return provisioning_saveWifiCredentials(String(ssid), String(psk));
}