#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>

void network_init();
int getNetworkPreference();
void setNetworkPreference(int newPref);
void manageAutoNetwork();
bool wifi_connectFromPrefs(unsigned long timeoutMs = 8000);
void tryStartLTE();
void clearNetworkUserBlock();

// New: suppress modem auto-attach for short period (ms)
void network_suppress_modem_attach_ms(unsigned long ms);

#endif // NETWORK_MANAGER_H