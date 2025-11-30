#pragma once
#include <Arduino.h>

// Initialize SD logging system
// Call once in setup() after SD card is initialized
void sdlog_init();

// Write current sensor data to SD card
// Returns true if successful, false otherwise
// Automatically creates daily files and headers
bool sdlog_write();

// Get current log filename (e.g. "beehive_20251130.csv")
String sdlog_getCurrentFilename();

// Check if SD logging is available
bool sdlog_isEnabled();

// Get number of records written today
int sdlog_getRecordCount();

// Get last log timestamp (for display in SD INFO menu)
String sdlog_getLastTimestamp();
