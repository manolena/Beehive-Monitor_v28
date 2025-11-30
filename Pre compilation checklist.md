# Pre-Compilation Checklist - Beehive Monitor v29
**Timestamp: 29NOV25_015**

## Files to Replace in Your Project

Replace these 3 files in your project directory with the corrected versions:

### 1. text_strings.h
**Source:** `/mnt/user-data/outputs/text_strings.h`
**Destination:** Your project root directory
**Changes:** Added 20 new TextId enum entries

### 2. text_strings.cpp  
**Source:** `/mnt/user-data/outputs/text_strings.cpp`
**Destination:** Your project root directory
**Changes:** 
- All strings padded to 20 characters
- All Greek strings converted to UPPERCASE
- Added translations for 20 new entries

### 3. menu_manager.cpp
**Source:** `/mnt/user-data/outputs/menu_manager.cpp`
**Destination:** Your project root directory
**Changes:**
- Replaced all hardcoded English strings with TextId system
- Added `getText()` helper function
- All menu functions now fully bilingual

## Verification Before Compilation

### ✅ File Integrity Check
All files are present and verified:
- [x] BeehiveMonitor_29.ino (main sketch)
- [x] config.h (all constants defined)
- [x] calibration.h/cpp
- [x] ui.h/cpp (Greek display functions present)
- [x] menu_manager.h/cpp (to be replaced)
- [x] text_strings.h/cpp (to be replaced)
- [x] time_manager.h/cpp
- [x] weather_manager.h/cpp
- [x] modem_manager.h/cpp
- [x] network_manager.h/cpp
- [x] sensors.h/cpp
- [x] thingspeak_client.h/cpp
- [x] All other support files

### ✅ Dependencies Check
All required libraries referenced:
- [x] WiFi.h (ESP32 built-in)
- [x] Wire.h (ESP32 built-in)
- [x] SPI.h (ESP32 built-in)
- [x] SD.h (ESP32 built-in)
- [x] Preferences.h (ESP32 built-in)
- [x] LiquidCrystal_I2C (external - must be installed)
- [x] TinyGSM (external - must be installed)
- [x] Adafruit_MPU6050 (external - must be installed)
- [x] Adafruit_Sensor (external - must be installed)
- [x] ArduinoJson (external - must be installed)
- [x] HTTPClient (ESP32 built-in)
- [x] WebServer (ESP32 built-in)
- [x] HX711 (external - optional, conditionally compiled)

### ✅ External Variables Check (All Defined in .ino)
- [x] `test_weight` = NAN
- [x] `test_temp_int` = NAN
- [x] `test_hum_int` = NAN
- [x] `test_temp_ext` = NAN
- [x] `test_hum_ext` = NAN
- [x] `test_pressure` = NAN
- [x] `test_acc_x` = NAN
- [x] `test_acc_y` = NAN
- [x] `test_acc_z` = NAN
- [x] `test_batt_voltage` = NAN
- [x] `test_batt_percent` = -999
- [x] `test_rssi` = -999
- [x] `test_lat` = 0.0
- [x] `test_lon` = 0.0
- [x] `sd_present` = false
- [x] `lcdMutex` = SemaphoreHandle_t
- [x] `currentLanguage` = LANG_EN (defined in ui.cpp)
- [x] `beepQueue` = QueueHandle_t

### ✅ Greek Character System Check
Verified in `ui.cpp`:

**CGRAM Positions (lines 224-240):**
```cpp
byte Gamma[8]  = { B11111, B10000, B10000, B10000, B10000, B10000, B10000, B00000 }; // Pos 0
byte Delta[8]  = { B00100, B01010, B10001, B10001, B10001, B10001, B11111, B00000 }; // Pos 1
byte Lambda[8] = { B00100, B01010, B10001, B10001, B10001, B10001, B10001, B00000 }; // Pos 2
byte Xi[8]     = { B11111, B00000, B00000, B01110, B00000, B00000, B11111, B00000 }; // Pos 3
byte Pi[8]     = { B11111, B10001, B10001, B10001, B10001, B10001, B10001, B00000 }; // Pos 4
byte Phi[8]    = { B01110, B10101, B10101, B10101, B01110, B00100, B00100, B00000 }; // Pos 5
byte Psi[8]    = { B10101, B10101, B10101, B01110, B00100, B00100, B00100, B00000 }; // Pos 6
byte Omega[8]  = { B01110, B10001, B10001, B10001, B01110, B00000, B11111, B00000 }; // Pos 7
```

**lcdPrintGreek() Function (lines 246-375):**
- [x] Maps UTF-8 Greek characters to HD44780 codes
- [x] Uses CGRAM positions for Γ, Δ, Λ, Ξ, Π, Φ, Ψ, Ω
- [x] Handles degree symbol (°) properly
- [x] Preserves UTF-8 in mirror buffer

**greekToUpper() Function (greek_utils.cpp):**
- [x] Converts all Greek lowercase to UPPERCASE
- [x] Handles accented characters (removes accents)
- [x] Preserves ASCII characters

## Compilation Steps

### Step 1: Backup Current Files
```bash
# Backup your current files before replacing
cp text_strings.h text_strings.h.backup
cp text_strings.cpp text_strings.cpp.backup
cp menu_manager.cpp menu_manager.cpp.backup
```

### Step 2: Replace Files
Copy the 3 corrected files from `/mnt/user-data/outputs/` to your project directory:
- text_strings.h
- text_strings.cpp
- menu_manager.cpp

### Step 3: Verify File Sizes (Sanity Check)
Expected file sizes:
- text_strings.h: ~2.3 KB
- text_strings.cpp: ~11 KB
- menu_manager.cpp: ~30 KB

### Step 4: Compile
```bash
# Using Arduino IDE: File → Verify/Compile
# Or using Arduino CLI:
arduino-cli compile --fqbn esp32:esp32:esp32 BeehiveMonitor_29.ino

# Or using PlatformIO:
pio run
```

## Expected Compilation Results

### ✅ Success Indicators:
- No syntax errors
- No undefined reference errors
- No missing TextId errors
- Sketch compiles successfully
- Binary size within ESP32 limits

### ⚠️ Possible Warnings (Safe to Ignore):
- Unused variable warnings
- Deprecated API warnings from libraries
- String literal conversion warnings

## Post-Compilation Testing

### Phase 1: Basic Boot Test
1. Upload to ESP32
2. Observe Serial output (115200 baud)
3. Verify splash screen appears on LCD
4. Check for initialization messages

### Phase 2: Language Test
1. Navigate to LANGUAGE menu
2. Toggle between EN/GR
3. Verify:
   - ✓ All menu items change language
   - ✓ No text overflow on LCD
   - ✓ Greek characters display correctly
   - ✓ Special Greek chars (Γ, Δ, Λ, Ξ, Π, Φ, Ψ, Ω) work

### Phase 3: Menu Navigation Test
Navigate through all menus and verify language consistency:

**Main Menu:**
- [ ] STATUS - Check both languages
- [ ] TIME - Check both languages  
- [ ] MEASUREMENTS - Check both languages
- [ ] WEATHER - Check both languages
- [ ] CONNECTIVITY - Check both languages (including network selection)
- [ ] DATA SENDING - Check both languages (including interval selection)
- [ ] PROVISION - Check both languages
- [ ] CALIBRATION - Check all sub-menus in both languages
- [ ] LANGUAGE - Toggle works
- [ ] SD INFO - Check both languages

**Calibration Sub-Menu:**
- [ ] TARE - Check both languages
- [ ] CALIBRATE - Check both languages
- [ ] RAW VALUE - Check both languages
- [ ] SAVE FACTOR - Check both languages

### Phase 4: String Padding Test
Check LCD for:
- [ ] No characters bleeding to next lines
- [ ] No leftover characters from previous messages
- [ ] All lines clear properly when switching menus
- [ ] Exactly 20 characters per line (no more, no less)

### Phase 5: Greek Character Test
In Greek mode, verify special characters:
- [ ] Γ (GAMMA) displays correctly
- [ ] Δ (DELTA) displays correctly
- [ ] Λ (LAMBDA) displays correctly
- [ ] Ξ (XI) displays correctly
- [ ] Π (PI) displays correctly
- [ ] Φ (PHI) displays correctly
- [ ] Ψ (PSI) displays correctly
- [ ] Ω (OMEGA) displays correctly
- [ ] ° (degree symbol) displays correctly
- [ ] All other Greek capitals display correctly

## Troubleshooting

### Issue: Compilation Error - "TXT_XXX was not declared"
**Cause:** Missing TextId entry
**Solution:** 
1. Check which TextId is missing
2. Verify you replaced text_strings.h with the corrected version
3. Re-copy the corrected text_strings.h

### Issue: Compilation Error - "getText was not declared"
**Cause:** menu_manager.cpp not replaced
**Solution:**
1. Verify you replaced menu_manager.cpp with the corrected version
2. Re-copy the corrected menu_manager.cpp

### Issue: Greek Text Shows Lowercase
**Cause:** greekToUpper() not being called
**Solution:**
1. Verify ui.cpp is using the correct version
2. Check that `lcdPrintGreek()` calls `greekToUpper()`
3. This should already be correct in uploaded ui.cpp

### Issue: Greek Characters Show as '?'
**Cause:** CGRAM not initialized or wrong character mapping
**Solution:**
1. Verify `initGreekChars()` is called in `uiInit()` (ui.cpp line 517)
2. Check LCD I2C address is correct (0x27 in config.h)
3. Verify lcdPrintGreek() mapping in ui.cpp lines 246-375

### Issue: Text Overflows to Next Line
**Cause:** String not padded to 20 characters
**Solution:**
1. Verify you replaced text_strings.cpp with corrected version
2. Check the specific string in text_strings.cpp
3. All strings should have exactly 20 characters

## Critical Files Summary

### Files Modified (Replace These):
1. **text_strings.h** - Header with TextId enums
2. **text_strings.cpp** - Bilingual string database
3. **menu_manager.cpp** - Menu logic with bilingual support

### Files NOT Modified (Keep Original):
- All other .h and .cpp files
- BeehiveMonitor_29.ino
- config.h
- ui.cpp (already correct)
- ui.h
- greek_utils.cpp/h
- All other project files

## Quick Reference: TextId Additions

New entries added to text_strings.h:
```cpp
// Calibration
TXT_CALIBRATE_READ_RAW
TXT_SEL_TO_SHOW_VALUE
TXT_RAW_LABEL
TXT_FACTOR_LABEL
TXT_OFFSET_LABEL

// Connectivity
TXT_MODE_LTE
TXT_MODE_WIFI
TXT_CHOOSE_NETWORK
TXT_UP_DOWN_SEL_OK

// Data Sending
TXT_UP_DOWN_CHANGE
TXT_SEL_SAVE_BACK_EXIT
TXT_SAVED

// Time
TXT_DATE_LABEL
TXT_TIME_LABEL

// Weather
TXT_WEATHER_HEADER
TXT_LAT_LABEL
TXT_LON_LABEL

// Status
TXT_BATTERY_LABEL
TXT_WEIGHT_KG
TXT_ACC_LABEL
```

## Example Fixed Code

### Before (Hardcoded):
```cpp
writeColsOverwrite(0,0,padRightBytes(String("TARE DONE"),20));
```

### After (Bilingual):
```cpp
writeColsOverwrite(0,0,padRightBytes(String(getText(TXT_TARE_DONE)),20));
```

### Before (English Only):
```cpp
snprintf(line,sizeof(line),"LAT:%6.2f LON:%6.2f", lat, lon);
```

### After (Bilingual):
```cpp
snprintf(line,sizeof(line),"%s%6.2f %s%6.2f", getText(TXT_LAT_LABEL), lat, getText(TXT_LON_LABEL), lon);
```

## Final Verification Checklist

Before you consider the project complete:

- [ ] All 3 files replaced in project directory
- [ ] Project compiles without errors
- [ ] Sketch uploads to ESP32 successfully
- [ ] LCD displays correctly
- [ ] English language works in all menus
- [ ] Greek language works in all menus
- [ ] Language toggle works
- [ ] No text overflow on LCD
- [ ] Greek special characters display correctly
- [ ] All menu navigation works
- [ ] No leftover characters on screen

## Support Information

If you encounter any compilation errors:
1. Note the exact error message
2. Note the file and line number
3. Provide the error details for assistance

---
**Ready to Compile!**

Download the 3 corrected files, replace them in your project, and compile.

**Timestamp: 29NOV25_015**