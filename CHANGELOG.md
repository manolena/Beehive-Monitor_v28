# Changelog — v28 (2025-11-26)

## Added
- Web UI: professional Windows-like provisioning + LCD mirror (4-line emulator) with deep-blue background and white bold text.
- Wi‑Fi provisioning form supporting two SSIDs and passwords and endpoints /set_wifi and /wifi.json.
- Server-Sent Events endpoint /lcd/stream for near real-time LCD mirror updates.
- Mirror buffer API: lcd_set_line_simple / lcd_get_line_simple and in-memory mirror implementation.

## Fixed
- Geocoding requests and JSON handling for multiple response formats (lines array and line0/line1).
- Multiple UI robustness fixes (UTF-8 Greek handling utilities, safe FreeRTOS wrappers).
- Restored and stabilized menu_manager with safe calibration flow and mirror updates.

## Changed
- UI emulator font/contrast improvements (Arial bold fallback + monospace for alignment).
- README and version updated to v28.