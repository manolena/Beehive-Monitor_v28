#ifndef KEY_SERVER_H
#define KEY_SERVER_H

#include <WebServer.h>

// Register KeyServer routes on an existing WebServer instance.
// The caller is responsible for calling server.begin() and server.handleClient().
void keyServer_registerRoutes(WebServer &server);

// Compatibility helpers (kept for existing code)
void keyServer_init();
void keyServer_loop();
void keyServer_stop(); // stop/cleanup hook (no-op for shared-server model)

#endif // KEY_SERVER_H