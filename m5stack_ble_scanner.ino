/**
 * M5Stack Atom S3R BLE Scanner
 * 
 * For Arduino IDE with M5Stack Atom S3R
 * 
 * Required libraries (install via Library Manager):
 * - M5Unified (by M5Stack)
 * - ArduinoJson (by Benoit Blanchon, v7+)
 * - NimBLE-Arduino (by h2zero) - RECOMMENDED, smaller and faster
 *   OR use built-in ESP32 BLE (larger, slower)
 * 
 * Board settings in Arduino IDE:
 * - Board: M5Stack-ATOMS3
 * - USB CDC On Boot: Enabled
 * - USB Mode: Hardware CDC and JTAG
 * 
 * Features:
 * - Scans BLE advertising packets (ADV_IND, ADV_SCAN_IND, SCAN_RSP, etc.)
 * - Collects device name, manufacturer data, service UUIDs
 * - Displays device list on LCD
 * - Uploads data to server via WiFi
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include "config.h"

// ============ Data Structures ============

// BLE Device data structure
typedef struct {
  uint8_t mac[6];
  char name[32];
  int8_t rssi;
  int8_t txPower;
  uint8_t addressType;      // 0=public, 1=random
  uint8_t advType;          // Advertisement type
  uint16_t appearance;
  char manufacturerData[64]; // Hex string
  uint16_t manufacturerId;
  char serviceUUIDs[128];   // Comma-separated
  uint32_t firstSeen;
  uint32_t lastSeen;
  uint16_t seenCount;
  bool isApple;
  bool isGoogle;
  bool isMicrosoft;
  bool isBeacon;
} BLEDeviceData;

// Device display info
typedef struct {
  char name[20];
  char mac[18];
  int8_t rssi;
  uint16_t count;
  bool isRandom;
  char type[12];
} DeviceDisplay;

// Device buffer
BLEDeviceData deviceBuffer[MAX_DEVICES_BUFFER];
volatile uint16_t deviceCount = 0;

// Display list
DeviceDisplay displayList[MAX_DISPLAY_DEVICES];
uint16_t displayCount = 0;

// Scanner state
volatile bool isScanning = false;
unsigned long scanStartTime = 0;

// Flag for new data available
volatile bool newDataAvailable = false;

// Display state
uint16_t displayScrollOffset = 0;
unsigned long lastDisplayRefresh = 0;
bool needDisplayUpdate = true;

// Server connection status
enum ServerStatus {
  STATUS_UNKNOWN,
  STATUS_CONNECTING,
  STATUS_CONNECTED,
  STATUS_ERROR,
  STATUS_UPLOADING,
  STATUS_SCANNING
};
ServerStatus serverStatus = STATUS_UNKNOWN;

// Statistics
uint32_t totalAdvertisements = 0;
uint32_t totalDevices = 0;
uint32_t uploadedDevices = 0;
uint32_t successfulUploads = 0;
uint32_t failedUploads = 0;

// BLE Scanner
NimBLEScan* pBLEScan = nullptr;

// Colors
#define COLOR_BG        TFT_BLACK
#define COLOR_TEXT      TFT_WHITE
#define COLOR_HEADER    0x000F  // Dark blue
#define COLOR_OK        TFT_GREEN
#define COLOR_ERROR     TFT_RED
#define COLOR_WARNING   TFT_YELLOW
#define COLOR_BLE       0x07FF  // Cyan
#define COLOR_APPLE     TFT_WHITE
#define COLOR_RANDOM    TFT_MAGENTA
#define COLOR_COUNT     TFT_DARKGREY

// ============ Advertisement Type Names ============

const char* getAdvTypeName(uint8_t advType) {
  switch (advType) {
    case 0: return "ADV_IND";      // Connectable undirected
    case 1: return "ADV_DIR";      // Connectable directed
    case 2: return "ADV_SCAN";     // Scannable undirected  
    case 3: return "ADV_NONC";     // Non-connectable undirected
    case 4: return "SCAN_RSP";     // Scan response
    default: return "UNKNOWN";
  }
}

// ============ Helper Functions ============

void macToString(const uint8_t* mac, char* str) {
  sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void bytesToHex(const uint8_t* data, size_t len, char* str, size_t maxLen) {
  size_t i;
  for (i = 0; i < len && (i * 2 + 1) < maxLen; i++) {
    sprintf(str + (i * 2), "%02X", data[i]);
  }
  str[i * 2] = '\0';
}

/**
 * Find device in buffer by MAC
 */
int findDevice(const uint8_t* mac) {
  for (uint16_t i = 0; i < deviceCount; i++) {
    if (memcmp(deviceBuffer[i].mac, mac, 6) == 0) {
      return i;
    }
  }
  return -1;
}

/**
 * Detect manufacturer from ID
 */
void detectManufacturer(BLEDeviceData* device) {
  device->isApple = (device->manufacturerId == 0x004C);
  device->isGoogle = (device->manufacturerId == 0x00E0);
  device->isMicrosoft = (device->manufacturerId == 0x0006);
  
  // Detect iBeacon (Apple manufacturer data starting with 0x0215)
  if (device->isApple && strlen(device->manufacturerData) >= 4) {
    if (strncmp(device->manufacturerData, "0215", 4) == 0) {
      device->isBeacon = true;
    }
  }
  
  // Detect Eddystone (Google UUID)
  if (strstr(device->serviceUUIDs, "FEAA") != NULL) {
    device->isBeacon = true;
    device->isGoogle = true;
  }
}

/**
 * Get device type string for display
 */
void getDeviceTypeString(BLEDeviceData* device, char* typeStr) {
  if (device->isBeacon) {
    strcpy(typeStr, "Beacon");
  } else if (device->isApple) {
    strcpy(typeStr, "Apple");
  } else if (device->isGoogle) {
    strcpy(typeStr, "Google");
  } else if (device->isMicrosoft) {
    strcpy(typeStr, "MS");
  } else if (device->addressType == 1) {
    strcpy(typeStr, "Random");
  } else {
    strcpy(typeStr, "Public");
  }
}

/**
 * Update display list from device buffer
 */
void updateDisplayList() {
  // Make local copy of volatile variable
  uint16_t currentDeviceCount = deviceCount;
  
  displayCount = 0;
  
  for (uint16_t i = 0; i < currentDeviceCount && displayCount < MAX_DISPLAY_DEVICES; i++) {
    BLEDeviceData* dev = &deviceBuffer[i];
    DeviceDisplay* disp = &displayList[displayCount];
    
    // Name (truncated)
    if (dev->name[0] != '\0') {
      strncpy(disp->name, dev->name, 19);
    } else {
      // Use MAC as name if no name available
      sprintf(disp->name, "%02X%02X%02X", dev->mac[3], dev->mac[4], dev->mac[5]);
    }
    disp->name[19] = '\0';
    
    // MAC
    macToString(dev->mac, disp->mac);
    
    // Other info
    disp->rssi = dev->rssi;
    disp->count = dev->seenCount;
    disp->isRandom = (dev->addressType == 1);
    
    // Type
    getDeviceTypeString(dev, disp->type);
    
    displayCount++;
  }
  
  needDisplayUpdate = true;
  
  #if DEBUG_VERBOSE > 1
  Serial.printf("[DISPLAY] Updated list: %d devices\n", displayCount);
  #endif
}

// ============ BLE Scan Callback ============

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    totalAdvertisements++;
    
    // Get MAC address
    NimBLEAddress addr = advertisedDevice->getAddress();
    uint8_t mac[6];
    
    // Parse MAC from string representation
    String macStr = String(addr.toString().c_str());
    sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    // Get RSSI
    int8_t rssi = advertisedDevice->getRSSI();
    if (rssi < MIN_RSSI) return;
    
    // Find or create device entry
    int idx = findDevice(mac);
    BLEDeviceData* device;
    
    if (idx >= 0) {
      // Update existing device
      device = &deviceBuffer[idx];
      device->lastSeen = millis() / 1000;
      device->seenCount++;
      device->rssi = rssi;  // Update RSSI
    } else {
      // Add new device
      if (deviceCount >= MAX_DEVICES_BUFFER) return;
      
      device = &deviceBuffer[deviceCount];
      memset(device, 0, sizeof(BLEDeviceData));
      memcpy(device->mac, mac, 6);
      device->firstSeen = millis() / 1000;
      device->lastSeen = device->firstSeen;
      device->seenCount = 1;
      device->rssi = rssi;
      device->txPower = -127;  // Unknown
      deviceCount++;
      totalDevices++;
    }
    
    // Update device info from advertisement
    device->addressType = (addr.getType() == BLE_ADDR_PUBLIC) ? 0 : 1;
    device->advType = advertisedDevice->getAdvType();
    
    // Device name
    if (advertisedDevice->haveName()) {
      strncpy(device->name, advertisedDevice->getName().c_str(), 31);
      device->name[31] = '\0';
    }
    
    // TX Power
    if (advertisedDevice->haveTXPower()) {
      device->txPower = advertisedDevice->getTXPower();
    }
    
    // Appearance
    if (advertisedDevice->haveAppearance()) {
      device->appearance = advertisedDevice->getAppearance();
    }
    
    // Manufacturer Data
    if (advertisedDevice->haveManufacturerData()) {
      std::string mfData = advertisedDevice->getManufacturerData();
      if (mfData.length() >= 2) {
        device->manufacturerId = (uint8_t)mfData[0] | ((uint8_t)mfData[1] << 8);
        bytesToHex((uint8_t*)mfData.data(), 
                   mfData.length() > 30 ? 30 : mfData.length(), 
                   device->manufacturerData, 63);
      }
    }
    
    // Service UUIDs
    if (advertisedDevice->haveServiceUUID()) {
      device->serviceUUIDs[0] = '\0';
      int uuidCount = advertisedDevice->getServiceUUIDCount();
      for (int i = 0; i < uuidCount && i < 5; i++) {
        NimBLEUUID uuid = advertisedDevice->getServiceUUID(i);
        if (i > 0) strcat(device->serviceUUIDs, ",");
        strncat(device->serviceUUIDs, uuid.toString().c_str(), 
                sizeof(device->serviceUUIDs) - strlen(device->serviceUUIDs) - 1);
      }
    }
    
    // Detect manufacturer/beacon type
    detectManufacturer(device);
    
    // Signal that new data is available for display
    newDataAvailable = true;
    
    #if DEBUG_VERBOSE
    char macStrBuf[18];
    macToString(mac, macStrBuf);
    Serial.printf("[BLE] %s '%s' RSSI:%d Type:%s\n",
                  macStrBuf, 
                  device->name[0] ? device->name : "(no name)",
                  rssi,
                  getAdvTypeName(device->advType));
    #endif
  }
  
  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    Serial.printf("[BLE] Scan complete, %d results, reason: %d\n", results.getCount(), reason);
    isScanning = false;
  }
};

ScanCallbacks scanCallbacks;

// ============ Display Functions ============

void drawStatusBar() {
  M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_HEADER);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(2, 6);
  
  switch (serverStatus) {
    case STATUS_UNKNOWN:
      M5.Lcd.setTextColor(COLOR_WARNING);
      M5.Lcd.print("?");
      break;
    case STATUS_CONNECTING:
      M5.Lcd.setTextColor(COLOR_WARNING);
      M5.Lcd.print("~");
      break;
    case STATUS_CONNECTED:
      M5.Lcd.setTextColor(COLOR_OK);
      M5.Lcd.print("+");
      break;
    case STATUS_ERROR:
      M5.Lcd.setTextColor(COLOR_ERROR);
      M5.Lcd.print("X");
      break;
    case STATUS_UPLOADING:
      M5.Lcd.setTextColor(COLOR_BLE);
      M5.Lcd.print("^");
      break;
    case STATUS_SCANNING:
      M5.Lcd.setTextColor(COLOR_OK);
      M5.Lcd.print("*");
      break;
  }
  
  // BLE indicator
  M5.Lcd.setTextColor(COLOR_BLE);
  M5.Lcd.setCursor(12, 6);
  M5.Lcd.print("BLE");
  
  // Device count on right
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.setCursor(SCREEN_WIDTH - 36, 6);
  M5.Lcd.printf("D:%d", (int)deviceCount);
}

void drawInfoBar() {
  M5.Lcd.fillRect(0, 20, SCREEN_WIDTH, 16, TFT_DARKGREY);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(2, 25);
  
  if (isScanning) {
    int remaining = SCAN_DURATION_SEC - ((millis() - scanStartTime) / 1000);
    M5.Lcd.printf("Scan:%ds Adv:%lu", remaining > 0 ? remaining : 0, totalAdvertisements);
  } else {
    M5.Lcd.printf("Dev:%d Up:%lu", (int)deviceCount, successfulUploads);
  }
}

void drawDeviceList() {
  int startY = 38;
  int lineHeight = 13;
  int listHeight = SCREEN_HEIGHT - startY - 2;
  int maxLines = listHeight / lineHeight;
  
  M5.Lcd.fillRect(0, startY, SCREEN_WIDTH, listHeight, COLOR_BG);
  
  #if DEBUG_VERBOSE
  Serial.printf("[DRAW] displayCount=%d, deviceCount=%d\n", displayCount, (int)deviceCount);
  #endif
  
  if (displayCount == 0) {
    M5.Lcd.setTextColor(COLOR_COUNT);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(4, startY + 20);
    M5.Lcd.print("Scanning BLE...");
    M5.Lcd.setCursor(4, startY + 35);
    M5.Lcd.print("Waiting for devices");
    return;
  }
  
  M5.Lcd.setTextSize(1);
  
  if (displayScrollOffset >= displayCount) {
    displayScrollOffset = 0;
  }
  
  for (int i = 0; i < maxLines && (displayScrollOffset + i) < displayCount; i++) {
    DeviceDisplay* entry = &displayList[displayScrollOffset + i];
    int y = startY + (i * lineHeight);
    
    // Alternating background
    if (i % 2 == 1) {
      M5.Lcd.fillRect(0, y, SCREEN_WIDTH, lineHeight, 0x1082);
    }
    
    // Device name/MAC
    if (entry->isRandom) {
      M5.Lcd.setTextColor(COLOR_RANDOM);
    } else {
      M5.Lcd.setTextColor(COLOR_BLE);
    }
    M5.Lcd.setCursor(2, y + 2);
    
    char truncated[12];
    strncpy(truncated, entry->name, 11);
    truncated[11] = '\0';
    M5.Lcd.print(truncated);
    
    // Type indicator
    M5.Lcd.setTextColor(COLOR_COUNT);
    M5.Lcd.setCursor(70, y + 2);
    
    char shortType[4];
    strncpy(shortType, entry->type, 3);
    shortType[3] = '\0';
    M5.Lcd.print(shortType);
    
    // Count
    M5.Lcd.setCursor(92, y + 2);
    M5.Lcd.printf("%d", entry->count > 99 ? 99 : entry->count);
    
    // RSSI bar
    int rssiX = 110;
    int rssiWidth = 15;
    int rssiHeight = 9;
    
    // Background
    M5.Lcd.fillRect(rssiX, y + 2, rssiWidth, rssiHeight, TFT_DARKGREY);
    
    // Signal strength bar
    int barWidth = map(constrain(entry->rssi, -100, -40), -100, -40, 1, rssiWidth);
    uint16_t barColor;
    if (entry->rssi > -60) {
      barColor = COLOR_OK;
    } else if (entry->rssi > -80) {
      barColor = COLOR_WARNING;
    } else {
      barColor = COLOR_ERROR;
    }
    M5.Lcd.fillRect(rssiX, y + 2, barWidth, rssiHeight, barColor);
  }
  
  // Scroll indicator
  if (displayCount > maxLines) {
    int scrollBarHeight = listHeight - 4;
    int thumbHeight = max(10, scrollBarHeight * maxLines / displayCount);
    int thumbPos = (scrollBarHeight - thumbHeight) * displayScrollOffset / (displayCount - maxLines);
    
    M5.Lcd.fillRect(SCREEN_WIDTH - 3, startY + 2, 2, scrollBarHeight, TFT_DARKGREY);
    M5.Lcd.fillRect(SCREEN_WIDTH - 3, startY + 2 + thumbPos, 2, thumbHeight, COLOR_TEXT);
  }
}

void refreshDisplay() {
  drawStatusBar();
  drawInfoBar();
  drawDeviceList();
  needDisplayUpdate = false;
}

void showMessage(const char* line1, const char* line2 = "", uint16_t color = COLOR_TEXT) {
  M5.Lcd.fillScreen(COLOR_BG);
  M5.Lcd.setTextColor(color);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(4, SCREEN_HEIGHT / 2 - 15);
  M5.Lcd.print(line1);
  if (line2[0] != '\0') {
    M5.Lcd.setCursor(4, SCREEN_HEIGHT / 2 + 5);
    M5.Lcd.print(line2);
  }
}

// ============ BLE Scanner Functions ============

void startScanning() {
  Serial.println("\n[BLE] Starting scan...");
  showMessage("Starting BLE scan...", "", COLOR_BLE);
  
  serverStatus = STATUS_SCANNING;
  
  // Initialize BLE if needed
  if (pBLEScan == nullptr) {
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(&scanCallbacks, false);
    pBLEScan->setActiveScan(BLE_ACTIVE_SCAN);
    pBLEScan->setInterval(BLE_SCAN_INTERVAL);
    pBLEScan->setWindow(BLE_SCAN_WINDOW);
  }
  
  // Clear old data
  deviceCount = 0;
  displayCount = 0;
  totalAdvertisements = 0;
  newDataAvailable = false;
  memset(deviceBuffer, 0, sizeof(deviceBuffer));
  
  // Start scan
  isScanning = true;
  scanStartTime = millis();
  pBLEScan->start(SCAN_DURATION_SEC, false);
  
  Serial.println("[BLE] Scan active");
  needDisplayUpdate = true;
}

void stopScanning() {
  Serial.println("[BLE] Stopping scan...");
  
  if (pBLEScan && pBLEScan->isScanning()) {
    pBLEScan->stop();
  }
  isScanning = false;
  
  // Update display list
  updateDisplayList();
  
  Serial.printf("[BLE] Found %d unique devices\n", deviceCount);
}

// ============ WiFi Upload Functions ============

bool connectWiFi() {
  Serial.println("\n========== WiFi Connection ==========");
  Serial.printf("[WIFI] SSID: %s\n", WIFI_SSID);
  Serial.printf("[WIFI] Connecting...\n");
  
  serverStatus = STATUS_CONNECTING;
  showMessage("Connecting WiFi...", WIFI_SSID, COLOR_WARNING);
  
  // Deinit BLE to free memory for WiFi
  pBLEScan = nullptr;
  NimBLEDevice::deinit(true);
  Serial.println("[WIFI] BLE deinitialized");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println("\n[WIFI] ERROR: Connection timeout!");
      Serial.printf("[WIFI] Status: %d\n", WiFi.status());
      serverStatus = STATUS_ERROR;
      return false;
    }
    delay(250);
    Serial.print(".");
    M5.Lcd.print(".");
    dots++;
    if (dots % 40 == 0) Serial.println();
  }
  
  Serial.println();
  Serial.println("[WIFI] Connected successfully!");
  Serial.printf("[WIFI] IP Address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WIFI] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("[WIFI] DNS: %s\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("[WIFI] RSSI: %d dBm\n", WiFi.RSSI());
  Serial.println("======================================\n");
  
  return true;
}

bool uploadDevices() {
  uint16_t currentDeviceCount = deviceCount;  // Local copy of volatile
  
  Serial.println("\n========== HTTP Upload ==========");
  Serial.printf("[UPLOAD] Devices to send: %d\n", currentDeviceCount);
  
  if (currentDeviceCount == 0) {
    Serial.println("[UPLOAD] No devices to upload, skipping");
    serverStatus = STATUS_CONNECTED;
    Serial.println("=================================\n");
    return true;
  }
  
  serverStatus = STATUS_UPLOADING;
  showMessage("Uploading...", String(currentDeviceCount).c_str(), COLOR_BLE);
  
  // Build URL
  String url = String("http://") + SERVER_HOST + ":" + String(SERVER_PORT) + API_ENDPOINT;
  Serial.printf("[UPLOAD] URL: %s\n", url.c_str());
  
  // Build JSON
  Serial.println("[UPLOAD] Building JSON payload...");
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  
  JsonArray devices = doc["devices"].to<JsonArray>();
  
  uint16_t maxCount = (currentDeviceCount < MAX_DEVICES_BUFFER) ? currentDeviceCount : MAX_DEVICES_BUFFER;
  
  for (uint16_t i = 0; i < maxCount; i++) {
    BLEDeviceData* dev = &deviceBuffer[i];
    JsonObject d = devices.add<JsonObject>();
    
    char macStr[18];
    macToString(dev->mac, macStr);
    d["mac"] = macStr;
    
    if (dev->name[0] != '\0') {
      d["name"] = dev->name;
    }
    
    d["rssi"] = dev->rssi;
    d["address_type"] = dev->addressType == 0 ? "public" : "random";
    d["adv_type"] = getAdvTypeName(dev->advType);
    d["seen_count"] = dev->seenCount;
    
    if (dev->txPower != -127) {
      d["tx_power"] = dev->txPower;
    }
    
    if (dev->appearance != 0) {
      d["appearance"] = dev->appearance;
    }
    
    if (dev->manufacturerData[0] != '\0') {
      d["manufacturer_id"] = dev->manufacturerId;
      d["manufacturer_data"] = dev->manufacturerData;
    }
    
    if (dev->serviceUUIDs[0] != '\0') {
      d["service_uuids"] = dev->serviceUUIDs;
    }
    
    d["is_beacon"] = dev->isBeacon;
    
    if (dev->isApple) d["vendor"] = "Apple";
    else if (dev->isGoogle) d["vendor"] = "Google";
    else if (dev->isMicrosoft) d["vendor"] = "Microsoft";
  }
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  Serial.printf("[UPLOAD] Payload size: %d bytes\n", jsonPayload.length());
  Serial.printf("[UPLOAD] Devices in payload: %d\n", maxCount);
  
  // Print first 500 chars of payload for debug
  if (jsonPayload.length() > 500) {
    Serial.printf("[UPLOAD] Payload preview: %s...\n", jsonPayload.substring(0, 500).c_str());
  } else {
    Serial.printf("[UPLOAD] Payload: %s\n", jsonPayload.c_str());
  }
  
  // HTTP POST
  Serial.println("[UPLOAD] Initiating HTTP connection...");
  HTTPClient http;
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(HTTP_TIMEOUT_MS);
  
  Serial.printf("[UPLOAD] Timeout set to: %d ms\n", HTTP_TIMEOUT_MS);
  Serial.println("[UPLOAD] Sending POST request...");
  
  unsigned long httpStart = millis();
  int httpCode = http.POST(jsonPayload);
  unsigned long httpDuration = millis() - httpStart;
  
  Serial.printf("[UPLOAD] Request completed in %lu ms\n", httpDuration);
  Serial.printf("[UPLOAD] HTTP Response Code: %d\n", httpCode);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.printf("[UPLOAD] Response body: %s\n", response.c_str());
    
    if (httpCode == 200) {
      Serial.println("[UPLOAD] SUCCESS!");
      uploadedDevices += maxCount;
      successfulUploads++;
      serverStatus = STATUS_CONNECTED;
      http.end();
      Serial.println("=================================\n");
      return true;
    } else {
      Serial.printf("[UPLOAD] ERROR: Server returned code %d\n", httpCode);
      failedUploads++;
      serverStatus = STATUS_ERROR;
    }
  } else {
    Serial.printf("[UPLOAD] ERROR: HTTP request failed!\n");
    Serial.printf("[UPLOAD] Error code: %d\n", httpCode);
    Serial.printf("[UPLOAD] Error: %s\n", http.errorToString(httpCode).c_str());
    failedUploads++;
    serverStatus = STATUS_ERROR;
  }
  
  http.end();
  Serial.println("=================================\n");
  return false;
}

void disconnectWiFi() {
  Serial.println("[WIFI] Disconnecting...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("[WIFI] Disconnected and WiFi OFF");
}

/**
 * Test server connectivity
 */
bool testServerConnection() {
  Serial.println("\n[TEST] Testing server connectivity...");
  
  HTTPClient http;
  String testUrl = String("http://") + SERVER_HOST + ":" + String(SERVER_PORT) + "/api/health";
  
  Serial.printf("[TEST] Testing URL: %s\n", testUrl.c_str());
  
  http.begin(testUrl);
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.printf("[TEST] Server responded with code: %d\n", httpCode);
    if (httpCode == 200) {
      String response = http.getString();
      Serial.printf("[TEST] Response: %s\n", response.c_str());
      Serial.println("[TEST] Server is reachable!");
      http.end();
      return true;
    }
  } else {
    Serial.printf("[TEST] Failed to connect: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  
  // Try base URL
  Serial.println("[TEST] Trying base URL...");
  String baseUrl = String("http://") + SERVER_HOST + ":" + String(SERVER_PORT) + "/";
  http.begin(baseUrl);
  http.setTimeout(5000);
  
  httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[TEST] Base URL responded with code: %d\n", httpCode);
    http.end();
    return true;
  } else {
    Serial.printf("[TEST] Base URL failed: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  Serial.println("[TEST] Server appears to be unreachable!");
  return false;
}

// ============ Main Functions ============

void setup() {
  // Initialize M5Stack
  auto cfg = M5.config();
  cfg.internal_imu = false;
  M5.begin(cfg);
  
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(COLOR_BG);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.setTextSize(1);
  
  // Welcome screen
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.setTextColor(COLOR_BLE);
  M5.Lcd.println("M5Stack Atom S3R");
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.setTextColor(COLOR_TEXT);
  M5.Lcd.println("BLE Scanner");
  M5.Lcd.setCursor(10, 70);
  M5.Lcd.setTextColor(COLOR_COUNT);
  M5.Lcd.println(DEVICE_ID);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.println("Active scan:");
  M5.Lcd.println(BLE_ACTIVE_SCAN ? "  YES (SCAN_RSP)" : "  NO");
  
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n==========================================");
  Serial.println("   M5Stack Atom S3R BLE Scanner");
  Serial.println("==========================================");
  Serial.printf("Device ID:    %s\n", DEVICE_ID);
  Serial.println("------------------------------------------");
  Serial.println("Server configuration:");
  Serial.printf("  Host:       %s\n", SERVER_HOST);
  Serial.printf("  Port:       %d\n", SERVER_PORT);
  Serial.printf("  Endpoint:   %s\n", API_ENDPOINT);
  Serial.printf("  Full URL:   http://%s:%d%s\n", SERVER_HOST, SERVER_PORT, API_ENDPOINT);
  Serial.println("------------------------------------------");
  Serial.println("WiFi configuration:");
  Serial.printf("  SSID:       %s\n", WIFI_SSID);
  Serial.printf("  Timeout:    %d ms\n", WIFI_CONNECT_TIMEOUT_MS);
  Serial.println("------------------------------------------");
  Serial.println("BLE configuration:");
  Serial.printf("  Active scan: %s\n", BLE_ACTIVE_SCAN ? "Yes" : "No");
  Serial.printf("  Duration:    %d sec\n", SCAN_DURATION_SEC);
  Serial.printf("  Min RSSI:    %d dBm\n", MIN_RSSI);
  Serial.printf("  Buffer:      %d devices\n", MAX_DEVICES_BUFFER);
  Serial.println("==========================================\n");
  
  delay(2000);
  
  // Start scanning
  startScanning();
  refreshDisplay();
}

void loop() {
  M5.update();
  
  // Button handling - scroll
  if (M5.BtnA.wasPressed()) {
    if (displayCount > MAX_VISIBLE_DEVICES) {
      displayScrollOffset += MAX_VISIBLE_DEVICES;
      if (displayScrollOffset >= displayCount) {
        displayScrollOffset = 0;
      }
      needDisplayUpdate = true;
    }
    Serial.printf("[BTN] Scroll to %d\n", displayScrollOffset);
  }
  
  // Long press - force upload
  if (M5.BtnA.pressedFor(2000)) {
    Serial.println("[BTN] Force upload triggered");
    if (isScanning) {
      stopScanning();
    }
    if (connectWiFi()) {
      testServerConnection();
      uploadDevices();
    }
    disconnectWiFi();
    delay(500);
    startScanning();
  }
  
  // Scanning phase
  if (isScanning) {
    // Check for new data and update display
    if (newDataAvailable || (millis() - lastDisplayRefresh >= DISPLAY_REFRESH_MS)) {
      newDataAvailable = false;
      updateDisplayList();
      lastDisplayRefresh = millis();
    }
    
    // Check if scan duration elapsed
    if (millis() - scanStartTime >= (SCAN_DURATION_SEC * 1000)) {
      stopScanning();
      
      if (deviceCount > 0) {
        if (connectWiFi()) {
          // Test server connectivity first
          testServerConnection();
          
          // Then upload
          uploadDevices();
        }
        disconnectWiFi();
      }
      
      // Stats
      Serial.println("\n--- Stats ---");
      Serial.printf("Advertisements: %lu\n", totalAdvertisements);
      Serial.printf("Unique devices: %lu\n", totalDevices);
      Serial.printf("Uploaded: %lu\n", uploadedDevices);
      Serial.printf("Success: %lu, Failed: %lu\n", successfulUploads, failedUploads);
      Serial.println("-------------\n");
      
      delay(CYCLE_DELAY_MS);
      startScanning();
    }
  }
  
  // Update display
  if (needDisplayUpdate) {
    refreshDisplay();
  }
  
  delay(10);
}
