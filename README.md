# 🐝 Beehive Monitor v28

[![Version](https://img.shields.io/badge/version-28-blue.svg)](https://github.com/yourusername/beehive-monitor)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-ESP32-orange.svg)](https://www.espressif.com/en/products/socs/esp32)

A comprehensive IoT beehive monitoring system with dual connectivity (WiFi/LTE), multi-language support, and cloud integration.

![Beehive Monitor](https://via.placeholder.com/800x400.png?text=Beehive+Monitor+v28)

---

## 📋 Table of Contents

- [Features](#-features)
- [Hardware Requirements](#-hardware-requirements)
- [Software Requirements](#-software-requirements)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Usage](#-usage)
- [Menu Structure](#-menu-structure)
- [Web Interface](#-web-interface)
- [API Endpoints](#-api-endpoints)
- [Troubleshooting](#-troubleshooting)
- [Contributing](#-contributing)
- [License](#-license)

---

## ✨ Features

### 🌍 Connectivity
- **Dual Network Support**: WiFi and LTE (A7670/SIM7600 modem)
- **Automatic Failover**: Seamless switching between networks
- **Network Preference**: User-selectable primary connection
- **Web Interface**: Real-time LCD mirror and provisioning

### 📊 Sensors
- **Weight Monitoring**: HX711 load cell with calibration
- **Temperature**: Internal (SI7021) and External (BME280/BME680)
- **Humidity**: Internal and External sensors
- **Atmospheric Pressure**: BME280/BME680 (hPa)
- **Accelerometer**: MPU6050 (X, Y, Z axes)
- **GPS**: Integrated via LTE modem
- **Battery**: Voltage and percentage monitoring

### 🚨 Alarm System
- **Motion Detection**: Accelerometer-based theft/movement alerts
- **SMS Notifications**: Instant alerts via LTE modem
- **Configurable Threshold**: Adjustable sensitivity

### 🌤️ Weather Integration
- **Open-Meteo API**: 3-day weather forecast
- **6-Hour Intervals**: Temperature, humidity, pressure
- **Geocoding**: Location-based weather data
- **Bilingual Descriptions**: English and Greek

### ☁️ Cloud Integration
- **ThingSpeak**: Automatic data upload
- **Configurable Intervals**: 1min to Daily uploads
- **GPS Tracking**: Location data included
- **Sensor Data**: All measurements synchronized

### 🖥️ User Interface
- **20x4 LCD Display**: I2C interface with Greek characters
- **Button Navigation**: 4-button control (UP/DOWN/SELECT/BACK)
- **Multi-Language**: English ⟷ Greek (Ελληνικά)
- **Interactive Menus**: 11 main items + calibration submenu
- **Real-Time Updates**: Auto-refreshing screens

### 💾 Data Persistence
- **NVS Storage**: Settings and calibration data
- **SD Card Support**: Optional data logging
- **Preferences**: WiFi credentials, network mode, intervals

---

## 🔧 Hardware Requirements

### Core Components
- **ESP32 Dev Module** (or compatible)
- **20x4 I2C LCD** (HD44780 compatible, address 0x27)
- **A7670 or SIM7600 LTE Modem** (with SIM card)
- **4 Push Buttons** (UP, DOWN, SELECT, BACK)

### Sensors
- **HX711 Load Cell Amplifier** + Load Cell
- **SI7021** Temperature/Humidity Sensor (internal)
- **BME280 or BME680** Temp/Humidity/Pressure Sensor (external)
- **MPU6050** Accelerometer/Gyroscope
- **Battery Voltage Divider** (10kΩ resistors)

### Optional
- **SD Card Module** (SPI interface)
- **Buzzer** (GPIO 5)

### Pin Configuration

| Component | GPIO Pin |
|-----------|----------|
| **I2C (LCD, Sensors)** | SDA: 21, SCL: 22 |
| **HX711 Load Cell** | DOUT: 19, SCK: 18 |
| **SD Card** | MISO: 2, MOSI: 15, SCLK: 14, CS: 13 |
| **LTE Modem** | RX: 27, TX: 26, PWR: 4 |
| **Buttons** | UP: 12, DOWN: 23, SELECT: 33, BACK: 32 |
| **Battery ADC** | GPIO 35 |
| **Buzzer** | GPIO 5 |

---

## 💻 Software Requirements

### Development Environment
- **Arduino IDE** 1.8.x or 2.x
- **ESP32 Board Support** (via Board Manager)

### Required Libraries
```
TinyGSM (>= 0.11.7)          // LTE modem communication
LiquidCrystal_I2C            // LCD display
ArduinoJson (>= 7.x)         // JSON parsing
Adafruit_MPU6050             // Accelerometer
Adafruit_Sensor              // Sensor abstraction
WiFi (built-in)              // WiFi connectivity
HTTPClient (built-in)        // HTTP requests
Preferences (built-in)       // NVS storage
SD (built-in)                // SD card support
SPI (built-in)               // SPI communication
Wire (built-in)              // I2C communication
```

### Installation via Arduino Library Manager
1. Open Arduino IDE
2. Go to **Sketch** → **Include Library** → **Manage Libraries**
3. Search and install each library listed above

---

## 📥 Installation

### 1. Clone Repository
```bash
git clone https://github.com/yourusername/beehive-monitor.git
cd beehive-monitor/BeehiveMonitor_29
```

### 2. Configure Settings
Edit `config.h` to set your credentials:

```cpp
// WiFi Credentials (dual network support)
#define WIFI_SSID1 "YourNetwork1"
#define WIFI_PASS1 "YourPassword1"
#define WIFI_SSID2 "YourNetwork2"
#define WIFI_PASS2 "YourPassword2"

// ThingSpeak API Key
#define THINGSPEAK_WRITE_APIKEY "YOUR_API_KEY"

// LTE APN
#define MODEM_APN "internet"  // Change to your carrier's APN

// Alarm Phone Number
#define ALARM_PHONE_NUMBER "+1234567890"  // For SMS alerts

// Accelerometer Threshold (m/s²)
#define ACCEL_THRESHOLD 2.0  // Adjust sensitivity
```

### 3. Upload to ESP32
1. Connect ESP32 via USB
2. Select **Tools** → **Board** → **ESP32 Dev Module**
3. Select correct **Port**
4. Click **Upload** (or press Ctrl+U)

### 4. Monitor Serial Output
- Open **Serial Monitor** (115200 baud)
- Watch for initialization messages
- Note the assigned IP address

---

## ⚙️ Configuration

### First-Time Setup

#### 1. Language Selection
- Navigate to **LANGUAGE** menu
- Press **SELECT** to toggle between EN ⟷ GR

#### 2. Network Configuration
- Navigate to **CONNECTIVITY** menu
- Press **SELECT** to choose WiFi or LTE
- Device will connect automatically

#### 3. Location Setup (for Weather)
- Navigate to **PROVISION** menu
- Press **SELECT** to enter city/country
- Use buttons to input location
- Weather data will be fetched automatically

### 4. SD Card Data Logging
- **Automatic Logging**: Sensor data is saved to the SD card automatically.
- **Format**: CSV files (Excel compatible).
- **Interval**: Synchronized with "DATA SENDING" interval (default 60 min).
- **Files**: Daily files created automatically (e.g., `beehive_20251130.csv`).
- **Columns**: Timestamp, Date, Time, Weight, Temp (Int/Ext), Humidity (Int/Ext), Pressure, Accelerometer (X/Y/Z), Battery, GPS, Network.

### 5. Data Upload Interval
- Navigate to **DATA SENDING** menu
- Use **UP/DOWN** to select interval
- Press **SELECT** to save

#### 5. Scale Calibration
- Navigate to **CALIBRATION** menu
- Select **TARE** to zero the scale
- Select **CALIBRATE** to set known weight
- Follow on-screen instructions

---

## 📱 Usage

### Button Controls
- **UP**: Navigate up / Increase value
- **DOWN**: Navigate down / Decrease value
- **SELECT**: Confirm / Enter submenu
- **BACK**: Cancel / Return to previous menu

### Main Menu Items

| Menu Item | Description |
|-----------|-------------|
| **STATUS** | View date/time, GPS, weight, battery |
| **TIME** | Display current date and time |
| **MEASUREMENTS** | Sensor readings (3 pages) |
| **WEATHER** | 3-day forecast (6-hour intervals) |
| **CONNECTIVITY** | Network status and selection |
| **DATA SENDING** | Configure upload interval |
| **PROVISION** | Set location for weather |
| **CALIBRATION** | Scale calibration submenu |
| **LANGUAGE** | Toggle English ⟷ Greek |
| **SD INFO** | SD card status |
| **BACK** | Exit to main screen |

### Calibration Submenu
1. **TARE**: Zero the scale (remove all weights)
2. **CALIBRATE**: Use known weight to calibrate
3. **RAW VALUE**: View raw sensor reading
4. **SAVE FACTOR**: Display saved calibration
5. **BACK**: Return to main menu

---

## 🌐 Web Interface

### Accessing the Interface
1. Connect to same WiFi network as device
2. Open browser and navigate to: `http://<device-ip>/`
3. View real-time LCD mirror

### Features
- **Real-Time LCD Mirror**: Exact replica of physical display
- **Server-Sent Events**: Live updates without polling
- **Provisioning**: Configure settings via web
- **UTF-8 Support**: Full Greek character display

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | LCD mirror web interface |
| `/lcd.json` | GET | Current LCD state (JSON) |
| `/lcd/stream` | GET | Server-Sent Events stream |
| `/provision` | GET/POST | WiFi provisioning |
| `/api/key` | POST | Store API keys |

---

## 📊 Menu Structure

For a complete menu structure diagram, see [menu_structure.md](menu_structure.md).

```
Main Menu
├── 1. STATUS
├── 2. TIME
├── 3. MEASUREMENTS (3 pages)
├── 4. WEATHER
├── 5. CONNECTIVITY
├── 6. DATA SENDING
├── 7. PROVISION
├── 8. CALIBRATION
│   ├── 1. TARE
│   ├── 2. CALIBRATE
│   ├── 3. RAW VALUE
│   ├── 4. SAVE FACTOR
│   └── 5. BACK
├── 9. LANGUAGE
├── 10. SD INFO
└── 11. BACK
```

---

## 🔍 Troubleshooting

### WiFi Connection Issues
- Verify credentials in `config.h`
- Check router compatibility (2.4GHz only)
- Monitor Serial output for error messages

### LTE Modem Not Responding
- Check SIM card insertion
- Verify APN settings for your carrier
- Ensure modem power pin (GPIO 4) is connected
- Check TX/RX connections (not swapped)

### LCD Not Displaying
- Verify I2C address (default 0x27)
- Check SDA/SCL connections (GPIO 21/22)
- Run I2C scanner to detect address

### Sensors Not Reading
- Check I2C connections for SI7021, BME280, MPU6050
- Verify sensor addresses don't conflict
- Monitor Serial output for initialization errors

### Weather Not Fetching
- Ensure WiFi is connected
- Verify location is set in PROVISION menu
- Check Open-Meteo API availability

### ThingSpeak Upload Failing
- Verify API key in `config.h`
- Check network connectivity
- Ensure interval is not too frequent (min 15s)

### Accelerometer Alarm Not Triggering
- Adjust `ACCEL_THRESHOLD` in `config.h`
- Verify phone number is set correctly
- Check LTE modem SMS functionality

---

## 🛠️ Development

### Project Structure
```
BeehiveMonitor_28/
├── BeehiveMonitor_28.ino      # Main sketch
├── config.h                    # Configuration and pin definitions
├── ui.cpp / ui.h               # LCD and button handling
├── menu_manager.cpp / .h       # Menu system
├── sensors.cpp / .h            # Sensor integration
├── weather_manager.cpp / .h    # Weather API integration
├── modem_manager.cpp / .h      # LTE modem control
├── network_manager.cpp / .h    # Network management
├── thingspeak_client.cpp / .h  # ThingSpeak upload
├── time_manager.cpp / .h       # NTP time sync
├── calibration.cpp / .h        # Scale calibration
├── text_strings.cpp / .h       # Bilingual text
├── greek_utils.cpp / .h        # Greek character handling
├── lcd_endpoint.cpp / .h       # Web LCD mirror
├── provisioning_ui.cpp / .h    # Location input
├── sms_handler.cpp / .h        # SMS alerts
└── README.md                   # This file
```

### Building from Source
```bash
# Using Arduino CLI
arduino-cli compile --fqbn esp32:esp32:esp32 BeehiveMonitor_29.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 BeehiveMonitor_29.ino
```

---

## 📝 Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

**Current Version: v28** (2025-11-28)

### Recent Updates
- Fixed Greek translations in Weather menu
- Corrected "hPa" display (was showing "HPA")
- Verified accelerometer alarm system
- Enhanced menu structure documentation

---

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 👥 Authors

- **Original Author** - Initial work
- **AI Assistant** - v26-v28 improvements and documentation

---

## 🙏 Acknowledgments

- **TinyGSM Library** - LTE modem support
- **Open-Meteo** - Free weather API
- **ThingSpeak** - IoT data platform
- **Adafruit** - Sensor libraries
- **ESP32 Community** - Support and examples

---

## 📞 Support

For issues and questions:
- Open an [Issue](https://github.com/yourusername/beehive-monitor/issues)
- Check [Troubleshooting](#-troubleshooting) section
- Review [CHANGELOG.md](CHANGELOG.md) for known issues

---

**Made with ❤️ for beekeepers**
