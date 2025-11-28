// url=https://github.com/manolena/Beehive-Monitor/blob/main/text_strings.cpp
#include "text_strings.h"

// English strings
const char* getTextEN(TextId id) {
  switch (id) {
    case TXT_STATUS:             return "STATUS";
    case TXT_TIME:               return "TIME";
    case TXT_MEASUREMENTS:       return "MEASUREMENTS";
    case TXT_WEATHER:            return "WEATHER";
    case TXT_CONNECTIVITY:       return "CONNECTIVITY";
    case TXT_PROVISION:          return "PROVISION";
    case TXT_CALIBRATION:        return "CALIBRATION";
    case TXT_LANGUAGE:           return "LANGUAGE";
    case TXT_SD_INFO:            return "SD INFO";
    case TXT_BACK:               return "BACK";

    case TXT_FETCHING_WEATHER:   return "FETCHING WEATHER   ";
    case TXT_WEATHER_NO_DATA:    return "WEATHER: NO DATA   ";
    case TXT_LAST_FETCH:         return "LAST FETCH";
    case TXT_PRESS_ANY:          return "PRESS ANY";
    case TXT_PRESS_SELECT:       return "PRESS SELECT";

    case TXT_ENTER_API_KEY:      return "ENTER API (SEL=next)";
    case TXT_KEY_STORED:         return "KEY STORED";
    case TXT_KEY_STORED_VERIFY:  return "KEY STORED,  VERIFY";
    case TXT_CANCELLED:          return "CANCELLED";
    case TXT_ENTER_CITY:         return "ENTER CITY";
    case TXT_ENTER_COUNTRY:      return "ENTER COUNTRY (2)";
    case TXT_GEOCODE_SAVED:      return "GEOCODE SAVED       ";
    case TXT_GEOCODE_FAILED:     return "GEOCODE FAILED      ";
    case TXT_VERIFYING_KEY:      return "VERIFYING KEY...    ";

    case TXT_WEIGHT:             return "WEIGHT:";
    case TXT_T_INT:              return "T_INT:";
    case TXT_H_INT:              return "H_INT:";
    case TXT_T_EXT:              return "T_EXT:";
    case TXT_H_EXT:              return "H_EXT:";
    case TXT_PRESSURE:           return "PRESS:";
    case TXT_ACCEL:              return "ACC:";

    case TXT_TARE:               return "TARE";
    case TXT_CALIBRATE_KNOWN:    return "CALIBRATE";
    case TXT_RAW_VALUE:          return "RAW VALUE";
    case TXT_SAVE_FACTOR:        return "SAVE FACTOR";
    case TXT_TARE_DONE:          return "TARE DONE          ";
    case TXT_CALIBRATION_DONE:   return "CALIBRATION DONE   ";
    case TXT_FACTOR_SAVED:       return "FACTOR SAVED        ";

    // New calibration helper English strings
    case TXT_TARE_PROMPT:        return "REMOVE ALL WEIGHTS";
    case TXT_PLACE_WEIGHT:       return "PLACE %s";           // format with weight string e.g. "2.000 kg"
    case TXT_MEASURING:          return "MEASURING...";
    case TXT_SAVE_FAILED:        return "SAVE FAILED";
    case TXT_NO_CALIBRATION:     return "NO CALIBRATION";

    case TXT_WIFI_CONNECTED:     return "WiFi: CONNECTED    ";
    case TXT_LTE_REGISTERED:     return "LTE: REGISTERED    ";
    case TXT_NO_CONNECTIVITY:    return "NO CONNECTIVITY    ";
    case TXT_SSID:               return "SSID:";
    case TXT_RSSI:               return "RSSI:";
    case TXT_MODE:               return "MODE:";

    case TXT_SD_CARD_INFO:       return "SD CARD INFO        ";
    case TXT_SD_OK:              return "SD OK              ";
    case TXT_NO_CARD:            return "NO CARD            ";

    case TXT_LANGUAGE_EN:        return "LANGUAGE: EN        ";
    case TXT_LANGUAGE_GR:        return "LANGUAGE: GR        ";

    case TXT_BACK_SMALL:         return "< BACK";
    case TXT_OK:                 return "OK";
    case TXT_ERROR:              return "ERROR";

    default: return "";
  }
}

// Greek strings (ALL CAPS). Use UTF-8 escapes where present.
const char* getTextGR(TextId id) {
  switch (id) {
    case TXT_STATUS:             return "ΚΑΤΑΣΤΑΣΗ"; // STATUS
    case TXT_TIME:               return "ΩΡΑ"; // TIME
    case TXT_MEASUREMENTS:       return "ΜΕΤΡΗΣΕΙΣ"; // MEASUREMENTS
    case TXT_WEATHER:            return "ΚΑΙΡΟΣ"; // WEATHER
    case TXT_CONNECTIVITY:       return "ΣΥΝΔΕΣΙΜΟΤΗΤΑ"; // CONNECTIVITY
    case TXT_PROVISION:          return "ΠΡΟΒΛΕΨΗ"; // PROVISION
    case TXT_CALIBRATION:        return "ΒΑΘΜΟΝΟΜΗΣΗ"; // CALIBRATION
    case TXT_LANGUAGE:           return "ΓΛΩΣΣΑ"; // LANGUAGE
    case TXT_SD_INFO:            return "ΠΛΗΡΟΦΟΡΙΕΣ SD"; // INFORMATION SD
    case TXT_BACK:               return "ΠΙΣΩ"; // BACK

    case TXT_FETCHING_WEATHER:   return "ΛΑΜΒΑΝΕΙ ΚΑΙΡΟ"; // FETCHING WEATHER
    case TXT_WEATHER_NO_DATA:    return "ΟΧΙ ΔΕΔΟΜΕΝΑ ΚΑΙΡΟΥ"; // NO WEATHER DATA
    case TXT_LAST_FETCH:         return "ΤΕΛΕΥΤΑΙΟΣ ΚΑΙΡΟΣ"; // LAST FETCH
    case TXT_PRESS_ANY:          return "ΠΑΤΗΣΕ ΚΟΥΜΠΙ"; // PRESS ANY KEY
    case TXT_PRESS_SELECT:       return "ΠΑΤΗΣΕ SEL"; // PRESS SELECT (keeps 'SEL' ASCII)

    case TXT_ENTER_API_KEY:      return "ΕΙΣΑΓ. ΑΡΙ SEL=ΕΠΟΜ"; // ENTER API KEY
    case TXT_KEY_STORED:         return "ΑΡΙ ΑΠΟΘΗΚΕΥΤΗΚΕ"; // KEY STORED
    case TXT_KEY_STORED_VERIFY:  return "ΕΠΑΛΗΘΕΥΣΗ ΑΡΙ"; // VERIFY STORED KEY
    case TXT_CANCELLED:          return "ΑΚΥΡΩΣΗ"; // CANCELLED
    case TXT_ENTER_CITY:         return "ΕΙΣΑΓΩΓΗ ΠΟΛΗΣ"; // ENTER CITY
    case TXT_ENTER_COUNTRY:      return "ΕΙΣΑΓΩΓΗ ΧΩΡΑΣ"; // ENTER COUNTRY
    case TXT_GEOCODE_SAVED:      return "ΑΠΟΘΗΚΕΥΤΗΚΕ GEOLOC"; // GEOCODE SAVED
    case TXT_GEOCODE_FAILED:     return "ΣΦΑΛΜΑ GEOLOC"; // GEOCODE FAILED
    case TXT_VERIFYING_KEY:      return "ΕΠΑΛΗΘΕΥΣΗ ΑΡΙ..."; // VERIFYING KEY

    case TXT_WEIGHT:             return "ΒΑΡΟΣ:"; // WEIGHT:
    case TXT_T_INT:              return "ΕΣΩΤ. ΘΕΡΜ.:"; // INTERIOR TEMP:
    case TXT_H_INT:              return "ΕΣΩΤ. ΥΓΡΑΣΙΑ:"; // INTERIOR HUMIDITY:
    case TXT_T_EXT:              return "ΕΞΩΤ. ΘΕΡΜ.:"; // EXTERIOR TEMP:
    case TXT_H_EXT:              return "ΕΞΩΤ. ΥΓΡΑΣΙΑ:"; // EXTERIOR HUMIDITY:
    case TXT_PRESSURE:           return "ΑΤΜ. ΠΙΕΣΗ:"; // PRESSURE:
    case TXT_ACCEL:              return "Χ,Υ,Ζ:"; // ACCELEROMETER:

    case TXT_TARE:               return "ΜΗΔΕΝΙΣΜΟΣ ΑΠΟΒΑΡΟΥ"; // TARE
    case TXT_CALIBRATE_KNOWN:    return "ΒΑΘΜΟΝΟΜΗΣΗ"; // CALIBRATION KNOWN
    case TXT_RAW_VALUE:          return "RAW ΤΙΜΗ"; // RAW VALUE
    case TXT_SAVE_FACTOR:        return "ΑΠΟΘΗΚΕΥΣΗ FACTOR"; // SAVE FACTOR
    case TXT_TARE_DONE:          return "ΑΠΟΒΑΡΟ OK"; //TARE DONE
    case TXT_CALIBRATION_DONE:   return "ΒΑΘΜΟΝΟΜΗΣΗ OK"; //CALIBRATION DONE
    case TXT_FACTOR_SAVED:       return "FACTOR ΑΠΟΘΗΚΕΥΤΗΚΕ"; //FACTOR SAVED

    // New calibration helper Greek strings
    case TXT_TARE_PROMPT:        return "ΑΦΑΙΡΕΣΤΕ ΒΑΡΗ"; // REMOVE WEIGHT
    case TXT_PLACE_WEIGHT:       return "ΒΑΛΤΕ ΓΝΩΣΤΟ ΒΑΡΟΣ"; // PLACE KNOWN WEIGHT
    case TXT_MEASURING:          return "ΜΕΤΡΗΣΗ ΒΑΡΟΥΣ..."; // MEASURING...
    case TXT_SAVE_FAILED:        return "ΣΦΑΛΜΑ ΑΠΟΘΗΚΕΥΣΗΣ"; // SAVE FAILED
    case TXT_NO_CALIBRATION:     return "ΔΕΝ ΒΑΘΜΟΝΟΜΗΘΗΚΕ"; // NO CALIBRATION

    case TXT_WIFI_CONNECTED:     return "WiFi: ΣΥΝΔΕΘΗΚΕ"; // WiFi: CONNECTED
    case TXT_LTE_REGISTERED:     return "LTE: ΣΤΟ ΔΙΚΤΥΟ ΟΚ"; // LTE: REGISTERED
    case TXT_NO_CONNECTIVITY:    return "ΔΕΝ ΥΠΑΡΧΕΙ ΣΥΝΔΕΣΗ"; // NO CONNECTIVITY
    case TXT_SSID:               return "SSID:";
    case TXT_RSSI:               return "RSSI:";
    case TXT_MODE:               return "MODE:";

    case TXT_SD_CARD_INFO:       return "ΠΛΗΡΟΦΟΡΙΕΣ SD"; // INFORMATION SD
    case TXT_SD_OK:              return "ΚΑΡΤΑ SD OK"; // SD OK
    case TXT_NO_CARD:            return "ΔΕΝ ΥΠΑΡΧΕΙ SD"; // NO SD

    case TXT_LANGUAGE_EN:        return "LANGUAGE: ENGLISH   ";
    case TXT_LANGUAGE_GR:        return "ΓΛΩΣΣΑ: ΕΛΛΗΝΙΚΑ";

    case TXT_BACK_SMALL:         return "< ΠΙΣΩ"; // < BACK
    case TXT_OK:                 return "OK";
    case TXT_ERROR:              return "\u039a\u0391\u03a4\u0391\u03a3\u03a4\u0391\u03a3\u0397"; // ΚΑΤΑΣΤΑΣΗ (placeholder)

    default: return "";
  }
}
