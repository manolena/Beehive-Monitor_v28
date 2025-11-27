#include <Arduino.h>
#include <Preferences.h>

// Diagnostic: dump saved WiFi prefs (callable from .ino)
void dumpWifiPrefs() {
  Preferences p;
  p.begin("beehive", true); // read-only ok
  String s1 = p.getString("wifi_ssid1", "");
  String k1 = p.getString("wifi_psk1", "");
  String s2 = p.getString("wifi_ssid2", "");
  String k2 = p.getString("wifi_psk2", "");
  p.end();

  auto mask = [](const String &pw)->String {
    if (pw.length() <= 2) return String("<len=") + pw.length() + ">";
    String r = pw.substring(0,1);
    for (size_t i=1;i+1<pw.length();++i) r += '*';
    r += pw.substring(pw.length()-1);
    return r;
  };

  Serial.println(F("=== WiFi prefs dump ==="));
  Serial.printf("ssid1: '%s'\n", s1.c_str());
  Serial.printf("psk1 : '%s'\n", mask(k1).c_str());
  Serial.printf("ssid2: '%s'\n", s2.c_str());
  Serial.printf("psk2 : '%s'\n", mask(k2).c_str());
  Serial.println(F("========================"));
}