#include "provisioning_ui.h"
#include "network_manager.h"
#include "config.h"
#include "ui.h"
#include "menu_manager.h"
#include "text_strings.h"
#include <Preferences.h>

// Namespace used elsewhere in network_manager
static const char *PREF_WIFI_NS = "beehive";

#include <WebServer.h>
extern WebServer server;

// ------------------------------------------------------------------
// Helper: Manual text entry (letter by letter)
// Returns entered text, or empty string if cancelled
// Long SELECT press finishes entry
// ------------------------------------------------------------------
static String manualTextEntry(const char* prompt, int maxLen) {
  const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz-";
  const int charsetLen = strlen(charset);
  
  String result = "";
  int charIndex = 0; // current character in charset
  int cursorPos = 0; // position in result string
  
  auto drawScreen = [&]() {
    uiClear();
    uiPrint(0, 0, prompt);
    
    // Show current text being built with cursor
    char line[24];
    if (result.length() == 0) {
      snprintf(line, sizeof(line), ">_");
    } else {
      snprintf(line, sizeof(line), ">%s_", result.c_str());
    }
    uiPrint(0, 1, line);
    
    // Show current character selection
    char charLine[24];
    snprintf(charLine, sizeof(charLine), "Char: %c", charset[charIndex]);
    uiPrint(0, 2, charLine);
    
    uiPrint(0, 3, "UP/DN SEL+ LONGSEL=Done");
    uiRefreshMirror();
  };
  
  drawScreen();
  
  // Long press detection
  unsigned long selectPressStart = 0;
  bool selectPressed = false;
  const unsigned long longPressDuration = 800; // ms
  
  while (true) {
    server.handleClient();
    // Check for long SELECT press
    if (digitalRead(BTN_SELECT) == LOW) {
      if (!selectPressed) {
        selectPressed = true;
        selectPressStart = millis();
      } else {
        // Check if held long enough
        if (millis() - selectPressStart >= longPressDuration) {
          // Long press detected - finish entry
          if (result.length() > 0) {
            return result;
          }
        }
      }
    } else {
      selectPressed = false;
    }
    
    Button b = getButton();
    
    if (b == BTN_UP_PRESSED) {
      charIndex++;
      if (charIndex >= charsetLen) charIndex = 0;
      drawScreen();
    }
    else if (b == BTN_DOWN_PRESSED) {
      charIndex--;
      if (charIndex < 0) charIndex = charsetLen - 1;
      drawScreen();
    }
    else if (b == BTN_SELECT_PRESSED) {
      // Short press: add character at current position
      if (result.length() < maxLen) {
        result += charset[charIndex];
        charIndex = 0; // reset to 'A'
        drawScreen();
      }
    }
    else if (b == BTN_BACK_PRESSED) {
      // Backspace: remove last character, or cancel if empty
      if (result.length() > 0) {
        result.remove(result.length() - 1);
        drawScreen();
      } else {
        return ""; // cancelled
      }
    }
    
    delay(50);
  }
}

// ------------------------------------------------------------------
// UI entrypoint: Direct manual entry for City and Country
// Long SELECT press switches from City to Country, then saves
// ------------------------------------------------------------------
void provisioning_ui_enterCityCountry() {
  // Phase 1: Enter City
  uiClear();
  uiPrint(0, 0, "GEOLOCATION ENTRY   ");
  uiPrint(0, 1, "Enter City name     ");
  uiPrint(0, 2, "LONGSEL to continue ");
  uiPrint(0, 3, "BACK to cancel      ");
  uiRefreshMirror();
  delay(1500);
  
  String city = manualTextEntry("Enter City:", 19);
  if (city.length() == 0) {
    // Cancelled
    menuDraw();
    return;
  }
  
  // Phase 2: Enter Country
  uiClear();
  uiPrint(0, 0, "GEOLOCATION ENTRY   ");
  uiPrint(0, 1, "Enter Country name  ");
  uiPrint(0, 2, "LONGSEL to save     ");
  uiPrint(0, 3, "BACK to cancel      ");
  uiRefreshMirror();
  delay(1500);
  
  String country = manualTextEntry("Enter Country:", 19);
  // Country can be empty (optional)
  
  // Save to preferences
  Preferences p;
  p.begin("beehive", false);
  p.putString("loc_name", city);
  p.putString("loc_country", country);
  p.end();
  
  // Show confirmation
  uiClear();
  uiPrint(0, 0, "SAVED!              ");
  char line[24];
  snprintf(line, sizeof(line), "City: %s", city.c_str());
  uiPrint(0, 1, line);
  if (country.length() > 0) {
    snprintf(line, sizeof(line), "Country: %s", country.c_str());
    uiPrint(0, 2, line);
  }
  uiRefreshMirror();
  delay(2000);
  menuDraw();
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