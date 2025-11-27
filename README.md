# Beehive-Monitor

A comprehensive beehive monitoring system using ESP32 with LTE connectivity, LCD display, and various sensors.

## Version

**Current Version: v28** (2025-11-26)

### Release v28 — Highlights

This release focuses on improving the web provisioning experience and on providing a reliable, near‑real‑time LCD mirror for remote debugging and monitoring. Key items in v28:

- Professional Windows-like provisioning UI with a 4-line LCD emulator (deep blue background, white bold text) that mirrors the device LCD output.
- Near-real-time updates via Server-Sent Events (/lcd/stream) in addition to the existing /lcd.json polling endpoint.
- Mirror buffer API (lcd_set_line_simple / lcd_get_line_simple) and an in-memory mirror implementation used by the web UI and SSE dispatcher.
- Improved Greek/UTF‑8 handling and safe FreeRTOS wrappers to increase robustness across modules.
- Stabilized menu manager and safe calibration flow to avoid blocking operations.

See CHANGELOG.md for the full list of changes.

## Features

- **Multi-language support**: English and Greek
- **Dual connectivity**: LTE (A7670 modem) and WiFi with automatic failover
- **Sensor integration**: Weight, temperature, humidity, pressure, accelerometer
- **LCD display**: 20x4 I2C LCD with Greek character support
- **Web interface**: WiFi provisioning, LCD JSON endpoint and SSE mirror
- **ThingSpeak integration**: Automatic data upload
- **Weather data**: Integration with Open-Meteo API
- **SD card logging**: Data persistence
- **Menu system**: Interactive button-driven navigation

## Building and Uploading

This is an Arduino/PlatformIO project for ESP32; v28 was validated for the ESP32 Dev Module.

1. Install Arduino IDE or use Arduino CLI/PlatformIO
2. Install required libraries:
   - TinyGSM
   - LiquidCrystal_I2C (or compatible I2C LCD library for ESP32)
   - WiFi (built-in)
   - SD (built-in)
   - Preferences (built-in)
3. Configure WiFi credentials in `config.h` (optional)
4. Upload `BeehiveMonitor_28.ino` to your ESP32

## License

See LICENSE file for details.

## Contributors

- Original author
- GitHub Copilot (v26 -> v28 improvements)