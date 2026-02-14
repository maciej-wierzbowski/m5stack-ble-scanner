# Code Improvements Recommended

This document lists recommended code improvements for better reliability and maintainability.

## High Priority

### 1. Buffer Overflow Protection

**Issue:** `bytesToHex()` function at line 137-142 could overflow

**Current code:**
```cpp
void bytesToHex(const uint8_t* data, size_t len, char* str, size_t maxLen) {
  size_t i;
  for (i = 0; i < len && (i * 2 + 1) < maxLen; i++) {
    sprintf(str + (i * 2), "%02X", data[i]);
  }
  str[i * 2] = '\0';
}
```

**Recommended fix:**
```cpp
void bytesToHex(const uint8_t* data, size_t len, char* str, size_t maxLen) {
  if (maxLen < 1) return;
  
  size_t i;
  size_t maxBytes = (maxLen - 1) / 2;  // Reserve space for null terminator
  size_t bytesToConvert = (len < maxBytes) ? len : maxBytes;
  
  for (i = 0; i < bytesToConvert; i++) {
    snprintf(str + (i * 2), maxLen - (i * 2), "%02X", data[i]);
  }
  str[min(i * 2, maxLen - 1)] = '\0';
}
```

### 2. JSON Buffer Sizing

**Issue:** Line 643 uses `JsonDocument` without explicit size

**Current code:**
```cpp
JsonDocument doc;
```

**Recommended fix:**
```cpp
// Calculate approximate size: base + (devices * avg_device_size)
const size_t JSON_BUFFER_SIZE = 1024 + (MAX_DEVICES_BUFFER * 256);
StaticJsonDocument<JSON_BUFFER_SIZE> doc;
// OR for dynamic allocation:
DynamicJsonDocument doc(JSON_BUFFER_SIZE);
```

### 3. Heap Monitoring

**Issue:** No heap monitoring despite memory constraints

**Recommended addition:**
```cpp
void printHeapStats() {
  Serial.printf("[MEM] Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("[MEM] Min free heap: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("[MEM] Heap size: %d bytes\n", ESP.getHeapSize());
}

// Call in setup() and before/after WiFi connection
```

Add to main loop:
```cpp
if (DEBUG_VERBOSE > 1) {
  static unsigned long lastHeapCheck = 0;
  if (millis() - lastHeapCheck > 10000) {  // Every 10s
    printHeapStats();
    lastHeapCheck = millis();
  }
}
```

### 4. String Safety

**Issue:** Multiple uses of `strcat`, `strcpy` without full bounds checking

**Locations:**
- Line 323: `strcat(device->serviceUUIDs, ",")`
- Line 324: `strncat` is better but still risky

**Recommended pattern:**
```cpp
// Instead of strcat/strncat:
size_t currentLen = strlen(device->serviceUUIDs);
size_t remaining = sizeof(device->serviceUUIDs) - currentLen - 1;

if (remaining > 0) {
  if (currentLen > 0) {
    strncat(device->serviceUUIDs, ",", remaining);
    remaining--;
  }
  strncat(device->serviceUUIDs, uuid.toString().c_str(), remaining);
}
```

## Medium Priority

### 5. Error Return Values

**Issue:** `connectWiFi()` doesn't handle all error cases

**Recommended additions:**
```cpp
bool connectWiFi() {
  // Add more specific error handling
  WiFi.mode(WIFI_STA);
  
  if (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) == WL_CONNECT_FAILED) {
    Serial.println("[WIFI] Failed to start connection");
    return false;
  }
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    // ... existing timeout code ...
    
    // Check for specific failures
    if (WiFi.status() == WL_NO_SSID_AVAIL) {
      Serial.println("[WIFI] SSID not found");
      return false;
    }
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("[WIFI] Connection failed (wrong password?)");
      return false;
    }
  }
  return true;
}
```

### 6. Watchdog Timer

**Issue:** Long operations could cause watchdog resets

**Recommended addition:**
```cpp
#include "esp_task_wdt.h"

void setup() {
  // ... existing code ...
  
  // Configure watchdog (60 seconds)
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);
}

void loop() {
  // Feed watchdog
  esp_task_wdt_reset();
  
  // ... existing code ...
}
```

### 7. NTP Time Sync

**Issue:** Timestamps are relative to boot time

**Recommended addition:**
```cpp
#include <time.h>

void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("[NTP] Waiting for time sync");
  
  time_t now = 0;
  struct tm timeinfo = {0};
  int retry = 0;
  const int retry_count = 10;
  
  while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count) {
    Serial.print(".");
    delay(500);
    time(&now);
    localtime_r(&now, &timeinfo);
  }
  
  if (timeinfo.tm_year >= (2020 - 1900)) {
    Serial.println(" OK");
    Serial.printf("[NTP] Time: %s", asctime(&timeinfo));
  } else {
    Serial.println(" FAILED");
  }
}

// Call after WiFi connection:
void connectWiFi() {
  // ... existing code ...
  if (connected) {
    syncTime();  // Add this
  }
  return connected;
}
```

## Low Priority

### 8. Configuration Validation

**Recommended addition to setup():**
```cpp
bool validateConfig() {
  bool valid = true;
  
  if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0) {
    Serial.println("[ERROR] WiFi SSID not configured!");
    valid = false;
  }
  
  if (strcmp(SERVER_HOST, "192.168.1.100") == 0) {
    Serial.println("[WARNING] Using default server IP");
  }
  
  if (SCAN_DURATION_SEC < 5 || SCAN_DURATION_SEC > 300) {
    Serial.println("[WARNING] Unusual scan duration");
  }
  
  return valid;
}

void setup() {
  // ... existing code ...
  
  if (!validateConfig()) {
    Serial.println("[ERROR] Invalid configuration - check config.h");
    M5.Lcd.fillScreen(TFT_RED);
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.println("CONFIG ERROR");
    M5.Lcd.println("Check serial");
    while(1) { delay(1000); }  // Halt
  }
}
```

### 9. Const Correctness

**Recommendation:** Add const to read-only parameters

```cpp
// Before:
void macToString(const uint8_t* mac, char* str)

// After (str is modified, so can't be const):
void macToString(const uint8_t* mac, char* str)  // Already correct!

// But this could be improved:
void detectManufacturer(BLEDeviceData* device)

// To:
void detectManufacturer(BLEDeviceData* device)  // Device IS modified, so OK
```

### 10. Magic Numbers

**Issue:** Some hardcoded values should be constants

**Examples:**
```cpp
// Line 459: truncated to 11 chars
#define MAX_DISPLAY_NAME_LEN 11

// Line 469: type truncated to 3 chars  
#define MAX_TYPE_DISPLAY_LEN 3

// Line 128x128 mentioned multiple places
// Already defined as SCREEN_WIDTH/HEIGHT - good!
```

## Testing Recommendations

1. **Boundary Testing**
   - Test with 0 devices
   - Test with MAX_DEVICES_BUFFER devices
   - Test with very long device names
   - Test with empty manufacturer data

2. **Network Testing**
   - Test with no WiFi available
   - Test with wrong password
   - Test with unreachable server
   - Test with very slow server responses

3. **Memory Testing**
   - Run for extended periods
   - Monitor heap fragmentation
   - Test rapid connect/disconnect cycles

4. **Display Testing**
   - Test with many devices (scrolling)
   - Test rapid device appearance/disappearance
   - Test button during different states

## Implementation Priority

1. **Before first public release:**
   - Buffer overflow protection (#1)
   - JSON buffer sizing (#2)
   - Configuration validation (#8)

2. **Before production deployment:**
   - Heap monitoring (#3)
   - String safety improvements (#4)
   - Better error handling (#5)

3. **Nice to have:**
   - NTP sync (#7)
   - Watchdog timer (#6)
   - Const correctness (#9)
