#include "sms_handler.h"
#include "modem_manager.h"
#include <TinyGsmClient.h>
#include <Arduino.h>

static unsigned long s_lastCheck = 0;
static const unsigned long SMS_CHECK_INTERVAL = 30UL * 1000UL;

static String sms_modemReadStream(TinyGsm &modem, unsigned long timeout=1000) {
  Stream &s = modem.stream; unsigned long start=millis(); String resp;
  while (millis()-start<timeout) {
    while (s.available()) { char c=(char)s.read(); resp+=c; }
    if (resp.length()) {
      unsigned long grace=millis();
      while (millis()-grace<120) { while (s.available()) { char c=(char)s.read(); resp+=c; } delay(5); }
      break;
    }
    delay(10);
  }
  return resp;
}

static String sms_atSendAndRead(TinyGsm &modem, const char *cmd, unsigned long timeout=1000) {
  modem.sendAT(cmd);
  return sms_modemReadStream(modem, timeout);
}

void sms_init() { s_lastCheck = millis(); }

void sms_scan_now() {
  TinyGsm &modem = modem_get();
  if (!modem_isNetworkRegistered()) return;
  sms_atSendAndRead(modem, "+CMGF=1", 800);
  modem.sendAT("+CMGL=\"REC UNREAD\"");
  String resp = sms_modemReadStream(modem, 2000);
  if (resp.indexOf("+CMGL:")>=0) {
    // Process responses (left minimal to avoid accidental deletion issues)
    // Implementation in production should parse entries and act accordingly.
  }
}

void sms_loop() {
  unsigned long now = millis();
  if (now - s_lastCheck < SMS_CHECK_INTERVAL) return;
  s_lastCheck = now;
  if (!modem_isNetworkRegistered()) return;
  sms_scan_now();
}