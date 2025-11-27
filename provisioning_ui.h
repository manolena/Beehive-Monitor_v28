#ifndef PROVISIONING_UI_H
#define PROVISIONING_UI_H

#include <Arduino.h>

// Initialize provisioning subsystem (web handlers elsewhere)
void provisioning_init();

// Called from UI menu to open city/country provisioning flow (keeps behavior local)
void provisioning_ui_enterCityCountry();

// Save WiFi credentials (called by web handler). Returns true on success.
bool provisioning_saveWifiCredentials(const String &ssid, const String &psk);

// Convenience wrapper used by simple web handlers (ssid/psk as C strings)
bool provisioning_saveWifiCredentials_c(const char *ssid, const char *psk);

#endif // PROVISIONING_UI_H