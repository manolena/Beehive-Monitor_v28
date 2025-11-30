# Changelog

All notable changes to the Beehive Monitor project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [v28] - 2025-11-28

### ✨ Added
- **SD Card Data Logging**: Comprehensive sensor data logging
  - Automatic CSV file generation (Excel compatible)
  - Daily file rotation (e.g., `beehive_20251130.csv`)
  - Synchronized with ThingSpeak upload interval
  - Logs 18 data points including sensors, GPS, and battery
  - SD INFO menu now displays current file, record count, and last timestamp

### 🐛 Fixed
- **Greek Translations**: Fixed missing Greek translations in Weather menu
  - "FETCHING WEATHER" now displays as "ΛΑΜΒΑΝΕΙ ΚΑΙΡΟ..." in Greek
  - "< BACK" button now properly translates to "< ΠΙΣΩ"
  - Fixed hardcoded English strings in `menu_manager.cpp`
- **Unit Display**: Corrected pressure unit display from "HPA" to "hPa"
  - Root cause: `greekToUpper()` was uppercasing all ASCII characters
  - Solution: Disabled ASCII uppercasing in `greek_utils.cpp` while preserving Greek character uppercasing
  - Now correctly displays mixed-case units like "hPa" and "WiFi"

### ✅ Verified
- **Accelerometer Alarm System**: Confirmed alarm mechanism is active and functional
  - Checks accelerometer every loop iteration
  - Triggers SMS when movement exceeds `ACCEL_THRESHOLD`
  - SMS includes detailed motion data (dX, dY, dZ values)
  - Phone number configurable via `ALARM_PHONE_NUMBER` in `config.h`

### 📚 Documentation
- Created comprehensive menu structure documentation with flowcharts
- Added bilingual (EN/GR) menu reference tables
- Documented all screen layouts and navigation paths
- Created detailed README.md with setup and troubleshooting guides

### 🔧 Modified Files
- `text_strings.cpp`: Updated Greek translation for `TXT_FETCHING_WEATHER`
- `menu_manager.cpp`: Fixed 3 instances of hardcoded English strings
- `greek_utils.cpp`: Disabled ASCII uppercasing to preserve unit formatting

---

## [v28] - 2025-11-28

### ✨ Added
- **Professional Web LCD Mirror**: Windows-like provisioning UI with 4-line LCD emulator
  - Deep blue background with white bold text
  - Real-time updates via Server-Sent Events (SSE)
  - `/lcd/stream` endpoint for live updates
  - `/lcd.json` polling endpoint for compatibility
- **Mirror Buffer API**: In-memory LCD state management
  - `lcd_set_line_simple()` / `lcd_get_line_simple()` functions
  - Used by web UI and SSE dispatcher
  - Thread-safe with FreeRTOS mutex protection

### 🔧 Improved
- **Greek/UTF-8 Handling**: Enhanced character rendering and display
- **FreeRTOS Safety**: Added safe wrappers for mutex and queue operations
- **Menu Manager**: Stabilized menu navigation and rendering
- **Calibration Flow**: Improved to avoid blocking operations

### 🐛 Fixed
- Menu rendering issues with Greek characters
- LCD mirror synchronization problems
- Calibration workflow blocking UI updates

---

## [v27] - 2025-11-29

### ✨ Added
- **Network Status Display**: Persistent network indicator in top-right corner
  - Shows "WiFi" or "LTE" on all menu pages
  - Updates automatically on network changes
- **Enhanced Network Management**: Improved WiFi/LTE switching logic
  - Fixed LTE selection persistence bug
  - Prevented ThingSpeak auto-WiFi override when LTE selected
  - Corrected reaffirmation bug in network selection

### 🔧 Improved
- Network preference handling and persistence
- Connection status reliability
- User feedback for network operations

### 🐛 Fixed
- Network status not displaying on main menu
- LTE preference being overridden by WiFi
- Network selection reaffirmation causing incorrect connections

---

## [v26] - 2025-11-29

### ✨ Added
- **Enhanced Provisioning Menu**: Improved geolocation entry
  - Faster alphabet cycling with UP/DOWN buttons
  - Short press SELECT: Add character
  - Long press SELECT: Switch fields/save data
- **Customizable Data Sending Intervals**: New interval options
  - 1, 5, 15, 30, 60 minutes
  - 2 hours, 6 hours, Daily
  - Properly displayed and selectable in menu
- **Full Web LCD Mirroring**: Complete screen synchronization
  - All menu states mirrored to web interface
  - Submenus and child menus included
  - UTF-8 character support

### 🐛 Fixed
- Initial SMS test disabled (no boot messages)
- Data sending menu display issues
- Keyboard unresponsiveness after menu exit
- Web LCD mirror not showing all screens

---

## [v25] - 2025-11-28

### ✨ Added
- **SD Card Support**: Full SD card initialization and logging
  - Auto-detection and status display
  - SPI configuration with custom pins
  - Fallback handling for missing cards
- **Compilation Error Fixes**: Resolved SD card SPI reference issues
  - Fixed `SPIClass&` binding error
  - Proper SPI object initialization

### 🔧 Improved
- SD card initialization robustness
- Error handling and user feedback
- Serial debug output for SD operations

---

## [v24] - 2025-11-27

### ✨ Added
- **Web LCD Emulator**: Real-time LCD content display in browser
  - Accurate 20x4 character rendering
  - Submenu navigation support
  - UTF-8 Greek character display
- **Web Form Interface**: Configuration via web browser
  - WiFi provisioning
  - API key management
  - Settings configuration

### 🐛 Fixed
- Web interface compilation errors
- LCD content synchronization issues
- Submenu display in web mirror

---

## [v23] - 2025-11-26

### ✨ Added
- **Data Sending Menu**: Configure ThingSpeak upload intervals
  - Multiple interval options
  - Persistent settings storage
  - User-friendly selection interface

### 🔧 Improved
- Menu system architecture
- Settings persistence via NVS
- User interface responsiveness

---

## [v22] - 2025-11-25

### ✨ Added
- **Accelerometer Integration**: MPU6050 support
  - X, Y, Z axis monitoring
  - Motion detection for alarm system
  - Configurable sensitivity threshold
- **SMS Alarm System**: Theft/movement alerts
  - Automatic SMS sending via LTE modem
  - Configurable phone number
  - Detailed motion data in alerts

### 🔧 Improved
- Sensor reading reliability
- Alarm triggering logic
- SMS delivery confirmation

---

## [v21] - 2025-11-24

### ✨ Added
- **Weather Integration**: Open-Meteo API support
  - 3-day forecast with 6-hour intervals
  - Temperature, humidity, pressure data
  - Bilingual weather descriptions (EN/GR)
  - Geocoding for location-based weather
- **GPS Support**: Location tracking via LTE modem
  - Automatic coordinate updates
  - Integration with ThingSpeak uploads
  - Display in STATUS menu

### 🔧 Improved
- Weather data fetching reliability
- Location accuracy
- API error handling

---

## [v20] - 2025-11-23

### ✨ Added
- **Dual Network Support**: WiFi and LTE connectivity
  - Automatic failover between networks
  - User-selectable preference
  - Network status display
- **LTE Modem Integration**: A7670/SIM7600 support
  - AT command interface
  - GPRS data connection
  - SMS functionality

### 🔧 Improved
- Network reliability and stability
- Connection management
- Error recovery

---

## [v19] - 2025-11-22

### ✨ Added
- **Bilingual Support**: English and Greek languages
  - Complete UI translation
  - Custom Greek character rendering
  - Language toggle in menu
- **Greek Character Support**: Custom LCD character mapping
  - CGRAM-based Greek letters
  - UTF-8 to LCD conversion
  - Proper uppercase handling

### 🔧 Improved
- Text rendering quality
- Character display accuracy
- Language switching speed

---

## [v18] - 2025-11-21

### ✨ Added
- **ThingSpeak Integration**: Cloud data upload
  - Automatic sensor data sync
  - Configurable upload intervals
  - API key management
- **Time Management**: NTP time synchronization
  - Automatic timezone handling
  - RTC backup
  - Time display in menus

### 🔧 Improved
- Data upload reliability
- Time accuracy
- Cloud connectivity

---

## [v17] - 2025-11-20

### ✨ Added
- **Calibration System**: Scale calibration menu
  - Tare function (zero offset)
  - Known weight calibration
  - Raw value display
  - Factor storage in NVS
- **Sensor Integration**: Multiple sensor support
  - SI7021 (internal temp/humidity)
  - BME280/BME680 (external temp/humidity/pressure)
  - HX711 load cell
  - Battery voltage monitoring

### 🔧 Improved
- Measurement accuracy
- Calibration workflow
- Sensor reading stability

---

## [v16] - 2025-11-19

### ✨ Added
- **Menu System**: Interactive navigation
  - 11 main menu items
  - Calibration submenu
  - Button-driven interface
  - Multi-page screens
- **LCD Display**: 20x4 I2C LCD support
  - Custom character support
  - Smooth rendering
  - Marker-based navigation

### 🔧 Improved
- User interface responsiveness
- Menu navigation logic
- Display update efficiency

---

## [v15] - 2025-11-18

### ✨ Added
- **Initial Release**: Basic beehive monitoring
  - ESP32 platform support
  - WiFi connectivity
  - Basic sensor reading
  - Serial output

---

## Legend

- 🎉 **Added**: New features
- 🔧 **Changed/Improved**: Changes to existing functionality
- 🗑️ **Deprecated**: Soon-to-be removed features
- 🐛 **Fixed**: Bug fixes
- 🔒 **Security**: Security improvements
- ✅ **Verified**: Confirmed functionality
- 📚 **Documentation**: Documentation updates

---

## Upgrade Notes

### v28 → v29
- No breaking changes
- Translation improvements are automatic
- Existing settings preserved

### v27 → v28
- Web interface URL unchanged
- New SSE endpoint available at `/lcd/stream`
- Existing `/lcd.json` endpoint still supported

### v26 → v27
- Network preferences preserved
- May need to reselect network preference once

### v25 → v26
- Data sending intervals reset to default
- Reconfigure in DATA SENDING menu

---

## Known Issues

### Current (v29)
- None reported

### Previous Versions
- **v28**: Minor Greek character rendering issues in web mirror (fixed in v29)
- **v27**: Network indicator not showing on main menu (fixed in v28)
- **v26**: SMS test messages sent on boot (fixed in v27)

---

## Roadmap

### Planned for v30
- [ ] Mobile app integration
- [ ] Bluetooth Low Energy support
- [ ] Advanced analytics dashboard
- [ ] Multi-hive support
- [ ] Battery optimization modes

### Under Consideration
- [ ] Solar panel integration
- [ ] Additional sensor types
- [ ] Voice alerts
- [ ] Email notifications
- [ ] Historical data visualization

---

**For detailed technical documentation, see [README.md](README.md)**

