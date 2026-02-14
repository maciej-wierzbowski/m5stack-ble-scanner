# M5Stack Atom S3R BLE Scanner

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue.svg)](https://www.espressif.com/)

A powerful BLE (Bluetooth Low Energy) scanner firmware for the M5Stack Atom S3R with LCD display. Captures BLE advertising packets, displays device information on screen, and uploads data to a server via WiFi.

**[Polish version (Wersja polska)](README_PL.md)**

![M5Stack Atom S3R](https://via.placeholder.com/600x300?text=Add+Screenshot+Here)

## Features

- 🔡 **BLE Scanning** - Captures advertising packets from nearby BLE devices
- 📺 **LCD Display** - Real-time status and device list
- 🔍 **Active Scan** - Sends SCAN_REQ to receive SCAN_RSP with additional data
- 🏷️ **Vendor Detection** - Identifies Apple, Google, Microsoft devices
- 📡 **Beacon Detection** - Recognizes iBeacon and Eddystone beacons
- 🌐 **HTTP Upload** - Sends data to server via WiFi
- 💾 **Efficient Memory** - Uses NimBLE for reduced memory footprint

## Hardware Requirements

- **M5Stack Atom S3R** (ESP32-S3 with 128x128 LCD)
- USB-C cable

## Software Requirements

### Arduino IDE Libraries

Install via Library Manager (`Tools` → `Manage Libraries`):

1. **M5Unified** (by M5Stack)
2. **ArduinoJson** (by Benoit Blanchon, v7+)
3. **NimBLE-Arduino** (by h2zero) - Recommended for smaller memory footprint

### Board Settings

| Setting | Value |
|---------|-------|
| Board | **M5Stack-ATOMS3** |
| USB CDC On Boot | **Enabled** |
| USB Mode | **Hardware CDC and JTAG** |

## Installation

### Step 1: Clone Repository

```bash
git clone https://github.com/maciej-wierzbowski/m5stack-ble-scanner.git
cd m5stack-ble-scanner
```

### Step 2: Configure Settings

```bash
# Copy example config
cp config.h.example config.h

# Edit config.h with your settings
# - WiFi SSID and password
# - Server IP and port
# - Device ID
```

### Step 3: Upload

1. Open `m5stack_ble_scanner.ino` in Arduino IDE
2. Select the correct board and port
3. Click **Upload**

## Configuration

Edit `config.h` before uploading:

```cpp
// WiFi credentials
const char* WIFI_SSID = "YourNetwork";
const char* WIFI_PASSWORD = "YourPassword";

// Server endpoint
const char* SERVER_HOST = "192.168.1.100";
const int SERVER_PORT = 8080;

// Unique device identifier
const char* DEVICE_ID = "M5ATOM-BLE-01";
```

## Usage

### Button Controls

| Action | Function |
|--------|----------|
| **Short Press** | Scroll through device list |
| **Long Press (2s)** | Force immediate upload |

### Operation Cycle

```
┌─────────────────────────────────┐
│   BLE SCANNING (30s)            │
│  - Collect ADV packets          │
│  - Display on LCD               │
└───────────┬─────────────────────┘
            ▼
┌─────────────────────────────────┐
│   UPLOAD (WiFi)                 │
│  - Disable BLE (save RAM)       │
│  - HTTP POST to server          │
└───────────┬─────────────────────┘
            ▼
        [REPEAT]
```

## Collected Data

| Field | Description |
|-------|-------------|
| **MAC Address** | Device address |
| **Address Type** | Public or Random |
| **Device Name** | Device name (if available) |
| **RSSI** | Signal strength |
| **Advertisement Type** | ADV_IND, SCAN_RSP, etc. |
| **TX Power** | Transmission power |
| **Manufacturer ID** | Company ID (e.g., 0x004C = Apple) |
| **Manufacturer Data** | Vendor-specific data |
| **Service UUIDs** | BLE service identifiers |
| **Appearance** | Device type indicator |

## JSON Output Format

```json
{
  "device_id": "M5ATOM-BLE-01",
  "devices": [
    {
      "mac": "AA:BB:CC:DD:EE:FF",
      "name": "iPhone",
      "rssi": -65,
      "address_type": "random",
      "adv_type": "ADV_IND",
      "tx_power": -12,
      "manufacturer_id": 76,
      "manufacturer_data": "0215AABBCCDD...",
      "service_uuids": "180F,180A",
      "is_beacon": false,
      "vendor": "Apple",
      "seen_count": 5
    }
  ]
}
```

## Server Setup

You need a server to receive the BLE data. See the [examples/server](examples/server) directory for:

- **Python Flask server** - Simple HTTP endpoint
- **Node.js server** - Alternative implementation
- **Docker setup** - Easy deployment

Example Python server:

```python
from flask import Flask, request
app = Flask(__name__)

@app.route('/api/esp32/ble', methods=['POST'])
def receive_ble_data():
    data = request.get_json()
    print(f"Received from {data['device_id']}: {len(data['devices'])} devices")
    # Process data here
    return {'status': 'ok'}, 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
```

## Troubleshooting

### Few devices detected

1. Enable Bluetooth on nearby devices
2. Reduce `MIN_RSSI` to `-100` in config.h
3. Increase `SCAN_DURATION_SEC`

### No device names

1. Ensure `BLE_ACTIVE_SCAN = true`
2. Many devices use Random addresses and hide names
3. Apple devices often don't broadcast names

### Upload failures

1. Check server is running and endpoint exists
2. Verify server IP address in config.h
3. Check firewall settings
4. Review serial monitor for detailed error messages

### Memory issues

1. Reduce `MAX_DEVICES_BUFFER` to 50
2. NimBLE uses less memory than standard ESP32 BLE
3. Monitor serial output for heap warnings

## Active vs Passive Scanning

### Active Scan (Default)

```
[Scanner] ──ADV_IND──> [Device]
[Scanner] <──ADV_IND── [Device]
[Scanner] ──SCAN_REQ─> [Device]
[Scanner] <──SCAN_RSP── [Device]  ← Additional data!
```

**Pros:** More information (names, services)  
**Cons:** Device knows it's being scanned

### Passive Scan

```
[Scanner] <──ADV_IND── [Device]
```

**Pros:** Completely passive, lower power  
**Cons:** Less information

Change in `config.h`:
```cpp
#define BLE_ACTIVE_SCAN false  // Passive scan
```

## Privacy & Legal Notice

⚠️ **Important:** BLE scanning may be subject to privacy regulations in your jurisdiction. 

- This tool is for **research and educational purposes**
- Always obtain proper authorization before deployment
- Respect local laws regarding wireless monitoring
- Do not use for unauthorized tracking or surveillance
- Consider privacy implications of data collection and storage

## Contributing

Contributions welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- M5Stack for excellent hardware
- NimBLE-Arduino project for efficient BLE stack
- ArduinoJson for JSON handling

## Author

[Maciej Wierzbowski] - [maciek.wierzbowski@gmail.com]

## Support

- **Issues:** [GitHub Issues](https://github.com/maciej-wierzbowski/m5stack-ble-scanner/issues)
- **Discussions:** [GitHub Discussions](https://github.com/maciej-wierzbowski/m5stack-ble-scanner/discussions)

---

**Star ⭐ this project if you find it useful!**
