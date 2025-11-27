#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "key_server.h"

void handleKeyRoot(WebServer &server) {
  String html = "<html><head><meta charset='utf-8'><title>KeyServer</title></head><body>"
                "<h3>KeyServer</h3><p>Endpoints:</p><ul>"
                "<li>/key/status - status</li>"
                "<li>/key/reboot - reboot device</li>"
                "</ul></body></html>";
  server.sendHeader("Content-Type", "text/html; charset=utf-8");
  server.send(200, "text/html", html);
}

static void handleKeyStatus(WebServer &server) {
  String s = "{ \"ok\": true, \"ip\": \"" + WiFi.localIP().toString() + "\" }";
  server.sendHeader("Content-Type", "application/json");
  server.send(200, "application/json", s);
}

static void handleKeyReboot(WebServer &server) {
  server.sendHeader("Content-Type", "text/plain");
  server.send(200, "text/plain", "Rebooting");
  Serial.println("[KeyServer] Reboot request received via /key/reboot");
  delay(200);
  ESP.restart();
}

void keyServer_registerRoutes(WebServer &server) {
  server.on("/", HTTP_GET, [&server]() { handleKeyRoot(server); });
  server.on("/key/status", HTTP_GET, [&server]() { handleKeyStatus(server); });
  server.on("/key/reboot", HTTP_GET, [&server]() { handleKeyReboot(server); });

  Serial.println("[KeyServer] routes registered on shared WebServer (port 80)");
  Serial.print("[KeyServer] IP: ");
  Serial.println(WiFi.localIP().toString());
}

void keyServer_init() {
  Serial.println("[KeyServer] init (no local server started)");
}

void keyServer_stop() {
  Serial.println("[KeyServer] stop (no-op for shared server)");
}

void keyServer_loop() {
  // no-op; main loop handles server.handleClient()
}