#include "text_strings.h"

// English strings - all padded to 20 characters
const char* getTextEN(TextId id) {
  switch (id) {
    case TXT_STATUS:             return "STATUS              ";
    case TXT_TIME:               return "TIME                ";
    case TXT_MEASUREMENTS:       return "MEASUREMENTS        ";
    case TXT_WEATHER:            return "WEATHER             ";
    case TXT_CONNECTIVITY:       return "CONNECTIVITY        ";
    case TXT_DATA_SENDING:       return "DATA SENDING        ";
    case TXT_PROVISION:          return "PROVISION           ";
    case TXT_CALIBRATION:        return "CALIBRATION         ";
    case TXT_LANGUAGE:           return "LANGUAGE            ";
    case TXT_SD_INFO:            return "SD INFO             ";
    case TXT_BACK:               return "BACK                ";

    case TXT_FETCHING_WEATHER:   return "FETCHING WEATHER    ";
    case TXT_WEATHER_NO_DATA:    return "WEATHER: NO DATA    ";
    case TXT_LAST_FETCH:         return "LAST FETCH          ";
    case TXT_PRESS_ANY:          return "PRESS ANY           ";
    case TXT_PRESS_SELECT:       return "PRESS SELECT        ";

    case TXT_ENTER_API_KEY:      return "ENTER API (SEL=next)";
    case TXT_KEY_STORED:         return "KEY STORED          ";
    case TXT_KEY_STORED_VERIFY:  return "KEY STORED, VERIFY  ";
    case TXT_CANCELLED:          return "CANCELLED           ";
    case TXT_ENTER_CITY:         return "ENTER CITY          ";
    case TXT_ENTER_COUNTRY:      return "ENTER COUNTRY (2)   ";
    case TXT_GEOCODE_SAVED:      return "GEOCODE SAVED       ";
    case TXT_GEOCODE_FAILED:     return "GEOCODE FAILED      ";
    case TXT_VERIFYING_KEY:      return "VERIFYING KEY...    ";

    case TXT_WEIGHT:             return "WEIGHT:             ";
    case TXT_T_INT:              return "T_INT:              ";
    case TXT_H_INT:              return "H_INT:              ";
    case TXT_T_EXT:              return "T_EXT:              ";
    case TXT_H_EXT:              return "H_EXT:              ";
    case TXT_PRESSURE:           return "PRESS:              ";
    case TXT_ACCEL:              return "ACC:                ";

    case TXT_TARE:               return "TARE                ";
    case TXT_CALIBRATE_KNOWN:    return "CALIBRATE           ";
    case TXT_RAW_VALUE:          return "RAW VALUE           ";
    case TXT_SAVE_FACTOR:        return "SAVE FACTOR         ";
    case TXT_TARE_DONE:          return "TARE DONE           ";
    case TXT_CALIBRATION_DONE:   return "CALIBRATION DONE    ";
    case TXT_FACTOR_SAVED:       return "FACTOR SAVED        ";

    case TXT_TARE_PROMPT:        return "REMOVE ALL WEIGHTS  ";
    case TXT_PLACE_WEIGHT:       return "PLACE %s            ";
    case TXT_MEASURING:          return "MEASURING...        ";
    case TXT_SAVE_FAILED:        return "SAVE FAILED         ";
    case TXT_NO_CALIBRATION:     return "NO CALIBRATION      ";

    case TXT_CALIBRATE_READ_RAW: return "CALIBRATE: READ RAW ";
    case TXT_SEL_TO_SHOW_VALUE:  return "SEL to show value   ";
    case TXT_RAW_LABEL:          return "RAW:                ";
    case TXT_FACTOR_LABEL:       return "FACTOR:             ";
    case TXT_OFFSET_LABEL:       return "OFFSET:             ";

    case TXT_WIFI_CONNECTED:     return "WiFi: CONNECTED     ";
    case TXT_LTE_REGISTERED:     return "LTE: REGISTERED     ";
    case TXT_NO_CONNECTIVITY:    return "NO CONNECTIVITY     ";
    case TXT_SSID:               return "SSID:               ";
    case TXT_RSSI:               return "RSSI:               ";
    case TXT_MODE:               return "MODE:               ";
    case TXT_MODE_LTE:           return "MODE: LTE           ";
    case TXT_MODE_WIFI:          return "MODE: WiFi          ";
    case TXT_CHOOSE_NETWORK:     return "Choose network:     ";
    case TXT_UP_DOWN_SEL_OK:     return "UP/DOWN=Sel  SEL=OK ";

    case TXT_SD_CARD_INFO:       return "SD CARD INFO        ";
    case TXT_SD_OK:              return "SD OK               ";
    case TXT_NO_CARD:            return "NO CARD             ";

    case TXT_LANGUAGE_EN:        return "LANGUAGE: EN        ";
    case TXT_LANGUAGE_GR:        return "LANGUAGE: GR        ";

    case TXT_INTERVAL_SELECT:    return "INTERVAL: ";
    case TXT_UP_DOWN_CHANGE:     return "UP/DOWN change      ";
    case TXT_SEL_SAVE_BACK_EXIT: return "SEL=Save  BACK=Exit ";
    case TXT_SAVED:              return "SAVED!              ";

    case TXT_DATE_LABEL:         return "DATE:               ";
    case TXT_TIME_LABEL:         return "TIME:               ";

    case TXT_WEATHER_HEADER:     return "WEATHER=====>SEL==>";
    case TXT_LAT_LABEL:          return "LAT:                ";
    case TXT_LON_LABEL:          return "LON:                ";

    case TXT_BATTERY_LABEL:      return "BATTERY:            ";
    case TXT_WEIGHT_KG:          return "WEIGHT:             ";
    case TXT_ACC_LABEL:          return "ACC:                ";

    case TXT_BACK_SMALL:         return "< BACK              ";
    case TXT_OK:                 return "OK                  ";
    case TXT_ERROR:              return "ERROR               ";

    default: return "                    ";
  }
}

// Greek strings (ALL CAPS except units like hPa, WiFi, LTE, C), padded to 20 characters
const char* getTextGR(TextId id) {
  switch (id) {
    case TXT_STATUS:             return "ΚΑΤΑΣΤΑΣΗ           ";
    case TXT_TIME:               return "ΩΡΑ                 ";
    case TXT_MEASUREMENTS:       return "ΜΕΤΡΗΣΕΙΣ           ";
    case TXT_WEATHER:            return "ΚΑΙΡΟΣ              ";
    case TXT_CONNECTIVITY:       return "ΣΥΝΔΕΣΙΜΟΤΗΤΑ       ";
    case TXT_DATA_SENDING:       return "ΑΠΟΣΤΟΛΗ ΔΕΔΟΜΕΝΩΝ  ";
    case TXT_PROVISION:          return "ΠΡΟΒΛΕΨΗ            ";
    case TXT_CALIBRATION:        return "ΒΑΘΜΟΝΟΜΗΣΗ         ";
    case TXT_LANGUAGE:           return "ΓΛΩΣΣΑ              ";
    case TXT_SD_INFO:            return "ΠΛΗΡΟΦΟΡΙΕΣ SD      ";
    case TXT_BACK:               return "ΠΙΣΩ                ";

    case TXT_FETCHING_WEATHER:   return "ΛΑΜΒΑΝΕΙ ΚΑΙΡΟ...   ";
    case TXT_WEATHER_NO_DATA:    return "ΟΧΙ ΔΕΔΟΜΕΝΑ ΚΑΙΡΟΥ ";
    case TXT_LAST_FETCH:         return "ΤΕΛΕΥΤΑΙΟΣ ΚΑΙΡΟΣ   ";
    case TXT_PRESS_ANY:          return "ΠΑΤΗΣΕ ΚΟΥΜΠΙ       ";
    case TXT_PRESS_SELECT:       return "ΠΑΤΗΣΕ SEL          ";

    case TXT_ENTER_API_KEY:      return "ΕΙΣΑΓ. ΑΡΙ SEL=ΕΠΟΜ ";
    case TXT_KEY_STORED:         return "ΑΡΙ ΑΠΟΘΗΚΕΥΤΗΚΕ    ";
    case TXT_KEY_STORED_VERIFY:  return "ΕΠΑΛΗΘΕΥΣΗ ΑΡΙ      ";
    case TXT_CANCELLED:          return "ΑΚΥΡΩΣΗ             ";
    case TXT_ENTER_CITY:         return "ΕΙΣΑΓΩΓΗ ΠΟΛΗΣ      ";
    case TXT_ENTER_COUNTRY:      return "ΕΙΣΑΓΩΓΗ ΧΩΡΑΣ      ";
    case TXT_GEOCODE_SAVED:      return "ΑΠΟΘΗΚΕΥΤΗΚΕ GEOLOC ";
    case TXT_GEOCODE_FAILED:     return "ΣΦΑΛΜΑ GEOLOC       ";
    case TXT_VERIFYING_KEY:      return "ΕΠΑΛΗΘΕΥΣΗ ΑΡΙ...   ";

    case TXT_WEIGHT:             return "ΒΑΡΟΣ:              ";
    case TXT_T_INT:              return "ΕΣΩΤ. ΘΕΡΜ.:        ";
    case TXT_H_INT:              return "ΕΣΩΤ. ΥΓΡΑΣΙΑ:      ";
    case TXT_T_EXT:              return "ΕΞΩΤ. ΘΕΡΜ.:        ";
    case TXT_H_EXT:              return "ΕΞΩΤ. ΥΓΡΑΣΙΑ:      ";
    case TXT_PRESSURE:           return "ΑΤΜ. ΠΙΕΣΗ:         ";
    case TXT_ACCEL:              return "Χ,Υ,Ζ:              ";

    case TXT_TARE:               return "ΜΗΔΕΝΙΣΜΟΣ ΑΠΟΒΑΡΟΥ ";
    case TXT_CALIBRATE_KNOWN:    return "ΒΑΘΜΟΝΟΜΗΣΗ         ";
    case TXT_RAW_VALUE:          return "RAW ΤΙΜΗ            ";
    case TXT_SAVE_FACTOR:        return "ΑΠΟΘΗΚΕΥΣΗ FACTOR   ";
    case TXT_TARE_DONE:          return "ΑΠΟΒΑΡΟ OK          ";
    case TXT_CALIBRATION_DONE:   return "ΒΑΘΜΟΝΟΜΗΣΗ OK      ";
    case TXT_FACTOR_SAVED:       return "FACTOR ΑΠΟΘΗΚΕΥΤΗΚΕ ";

    case TXT_TARE_PROMPT:        return "ΑΦΑΙΡΕΣΤΕ ΒΑΡΗ      ";
    case TXT_PLACE_WEIGHT:       return "ΒΑΛΤΕ ΓΝΩΣΤΟ ΒΑΡΟΣ  ";
    case TXT_MEASURING:          return "ΜΕΤΡΗΣΗ ΒΑΡΟΥΣ...   ";
    case TXT_SAVE_FAILED:        return "ΣΦΑΛΜΑ ΑΠΟΘΗΚΕΥΣΗΣ  ";
    case TXT_NO_CALIBRATION:     return "ΔΕΝ ΒΑΘΜΟΝΟΜΗΘΗΚΕ   ";

    case TXT_CALIBRATE_READ_RAW: return "ΒΑΘΜΟΝΟΜΗΣΗ: RAW    ";
    case TXT_SEL_TO_SHOW_VALUE:  return "SEL ΓΙΑ ΤΙΜΗ        ";
    case TXT_RAW_LABEL:          return "RAW:                ";
    case TXT_FACTOR_LABEL:       return "FACTOR:             ";
    case TXT_OFFSET_LABEL:       return "OFFSET:             ";

    case TXT_WIFI_CONNECTED:     return "WiFi: ΣΥΝΔΕΘΗΚΕ     ";
    case TXT_LTE_REGISTERED:     return "LTE: ΣΤΟ ΔΙΚΤΥΟ OK  ";
    case TXT_NO_CONNECTIVITY:    return "ΔΕΝ ΥΠΑΡΧΕΙ ΣΥΝΔΕΣΗ ";
    case TXT_SSID:               return "SSID:               ";
    case TXT_RSSI:               return "RSSI:               ";
    case TXT_MODE:               return "MODE:               ";
    case TXT_MODE_LTE:           return "MODE: LTE           ";
    case TXT_MODE_WIFI:          return "MODE: WiFi          ";
    case TXT_CHOOSE_NETWORK:     return "ΕΠΙΛΟΓΗ ΔΙΚΤΥΟΥ:    ";
    case TXT_UP_DOWN_SEL_OK:     return "UP/DOWN=ΕΠ  SEL=OK  ";

    case TXT_SD_CARD_INFO:       return "ΠΛΗΡΟΦΟΡΙΕΣ SD      ";
    case TXT_SD_OK:              return "ΚΑΡΤΑ SD OK         ";
    case TXT_NO_CARD:            return "ΔΕΝ ΥΠΑΡΧΕΙ SD      ";

    case TXT_LANGUAGE_EN:        return "LANGUAGE: ENGLISH   ";
    case TXT_LANGUAGE_GR:        return "ΓΛΩΣΣΑ: ΕΛΛΗΝΙΚΑ    ";

    case TXT_INTERVAL_SELECT:    return "ΔΙΑΣΤΗΜΑ: ";
    case TXT_UP_DOWN_CHANGE:     return "UP/DOWN ΑΛΛΑΓΗ      ";
    case TXT_SEL_SAVE_BACK_EXIT: return "SEL=ΑΠΟΘ BACK=ΕΞΟΔ  ";
    case TXT_SAVED:              return "ΑΠΟΘΗΚΕΥΤΗΚΕ!       ";

    case TXT_DATE_LABEL:         return "ΗΜΕΡΟΜΗΝΙΑ:         ";
    case TXT_TIME_LABEL:         return "ΩΡΑ:                ";

    case TXT_WEATHER_HEADER:     return "ΚΑΙΡΟΣ=====>SEL===>";
    case TXT_LAT_LABEL:          return "LAT:                ";
    case TXT_LON_LABEL:          return "LON:                ";

    case TXT_BATTERY_LABEL:      return "ΜΠΑΤΑΡΙΑ:           ";
    case TXT_WEIGHT_KG:          return "ΒΑΡΟΣ:              ";
    case TXT_ACC_LABEL:          return "ΕΠΙΤ.:              ";

    case TXT_BACK_SMALL:         return "< ΠΙΣΩ              ";
    case TXT_OK:                 return "OK                  ";
    case TXT_ERROR:              return "ΣΦΑΛΜΑ              ";

    default: return "                    ";
  }
}
