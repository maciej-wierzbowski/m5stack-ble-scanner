# M5Stack Atom S3R BLE Scanner

## Wersja dla M5Stack Atom S3R z wyświetlaczem LCD

Firmware do skanowania urządzeń BLE (Bluetooth Low Energy) z wyświetlaniem na ekranie LCD.

## Funkcje

- 📡 **Skanowanie BLE** - Przechwytywanie pakietów reklamowych
- 📺 **Wyświetlacz LCD** - Status i lista urządzeń
- 🔍 **Active Scan** - Wysyła SCAN_REQ aby otrzymać SCAN_RSP
- 🏷️ **Detekcja producentów** - Apple, Google, Microsoft
- 📍 **Detekcja beaconów** - iBeacon, Eddystone
- 🌐 **HTTP Upload** - Wysyłanie do serwera

## Zbierane informacje

| Pole | Opis |
|------|------|
| **MAC Address** | Adres urządzenia |
| **Address Type** | Public lub Random |
| **Device Name** | Nazwa urządzenia (jeśli dostępna) |
| **RSSI** | Siła sygnału |
| **Advertisement Type** | ADV_IND, ADV_SCAN_IND, SCAN_RSP, etc. |
| **TX Power** | Moc nadawania |
| **Manufacturer ID** | ID producenta (np. 0x004C = Apple) |
| **Manufacturer Data** | Dane producenta (hex) |
| **Service UUIDs** | UUID serwisów |
| **Appearance** | Typ urządzenia |

## Typy pakietów reklamowych

| Typ | Nazwa | Opis |
|-----|-------|------|
| 0 | **ADV_IND** | Connectable undirected - można się połączyć |
| 1 | **ADV_DIRECT_IND** | Connectable directed - skierowane do konkretnego urządzenia |
| 2 | **ADV_SCAN_IND** | Scannable undirected - można wysłać SCAN_REQ |
| 3 | **ADV_NONCONN_IND** | Non-connectable - tylko rozgłaszanie |
| 4 | **SCAN_RSP** | Scan Response - odpowiedź na SCAN_REQ |

## Układ ekranu

```
┌─────────────────────────────┐
│ * BLE              D:15     │  <- Status + liczba urządzeń
├─────────────────────────────┤
│ Scan:25s Adv:342            │  <- Czas / liczba pakietów
├─────────────────────────────┤
│ iPhone        App  5   ███  │  <- Lista urządzeń
│ Mi Band       Pub  3   ██   │     (nazwa, typ, count, RSSI)
│ AirPods       App  2   ███  │
│ 4A5B6C        Ran  1   █    │  <- Random MAC (fioletowy)
│ Tile          Pub  1   ██   │
│ ...                      ▓  │  <- Scroll indicator
└─────────────────────────────┘
```

### Kolory urządzeń

| Kolor | Znaczenie |
|-------|-----------|
| 🔵 Cyjan | Public address |
| 🟣 Fioletowy | Random address |

### Typy (skróty)

| Skrót | Znaczenie |
|-------|-----------|
| App | Apple |
| Goo | Google |
| MS | Microsoft |
| Bea | Beacon |
| Pub | Public address |
| Ran | Random address |

## Wymagania

### Sprzęt
- **M5Stack Atom S3R** (ESP32-S3 z LCD 128x128)
- Kabel USB-C

### Biblioteki Arduino IDE
1. **M5Unified** (by M5Stack)
2. **ArduinoJson** (by Benoit Blanchon, v7+)
3. **NimBLE-Arduino** (by h2zero) - **ZALECANA**

## Instalacja

### Krok 1: Zainstaluj biblioteki

**Tools** → **Manage Libraries** → zainstaluj:

1. **M5Unified** - obsługa M5Stack
2. **ArduinoJson** - JSON
3. **NimBLE-Arduino** - BLE (mniejsza i szybsza niż ESP32 BLE)

### Krok 2: Ustawienia płytki

| Ustawienie | Wartość |
|------------|---------|
| Board | **M5Stack-ATOMS3** |
| USB CDC On Boot | **Enabled** |
| USB Mode | **Hardware CDC and JTAG** |

### Krok 3: Konfiguracja

Edytuj `config.h`:

```cpp
// WiFi do uploadu
const char* WIFI_SSID = "TwojaSiec";
const char* WIFI_PASSWORD = "TwojeHaslo";

// Serwer
const char* SERVER_HOST = "192.168.1.100";
const int SERVER_PORT = 8080;

// ID urządzenia
const char* DEVICE_ID = "M5ATOM-BLE-01";
```

### Krok 4: Wgraj

1. Podłącz Atom S3R
2. Wybierz port
3. Kliknij **Upload**

## Obsługa

### Przycisk

| Akcja | Funkcja |
|-------|---------|
| **Krótkie naciśnięcie** | Przewiń listę urządzeń |
| **Przytrzymanie 2s** | Wymuś natychmiastowy upload |

### Cykl pracy

```
┌─────────────────────────────────────┐
│         SKANOWANIE BLE (30s)        │
│  - Zbieranie ADV_IND, SCAN_RSP...   │
│  - Wyświetlanie na LCD              │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│         UPLOAD (WiFi)               │
│  - Wyłączenie BLE (oszczędność RAM) │
│  - HTTP POST do serwera             │
└──────────────┬──────────────────────┘
               │
               ▼
          [POWTÓRZ]
```

## Parametry konfiguracyjne

### Skanowanie BLE

| Parametr | Domyślnie | Opis |
|----------|-----------|------|
| `SCAN_DURATION_SEC` | 30 | Czas skanowania (sekundy) |
| `BLE_SCAN_INTERVAL` | 80 | Interwał skanowania (×0.625ms = 50ms) |
| `BLE_SCAN_WINDOW` | 80 | Okno skanowania (×0.625ms = 50ms) |
| `BLE_ACTIVE_SCAN` | true | Aktywne skanowanie (SCAN_RSP) |
| `MIN_RSSI` | -95 | Minimalny RSSI |

### Bufory

| Parametr | Domyślnie | Opis |
|----------|-----------|------|
| `MAX_DEVICES_BUFFER` | 100 | Max urządzeń w buforze |
| `MAX_DISPLAY_DEVICES` | 50 | Max urządzeń na liście |

## Active vs Passive Scan

### Active Scan (domyślnie)

```
[Scanner] ──ADV_IND──> [Device]
[Scanner] <──ADV_IND── [Device]
[Scanner] ──SCAN_REQ─> [Device]
[Scanner] <──SCAN_RSP── [Device]  <- Dodatkowe dane!
```

**Zalety:**
- Więcej informacji (nazwa, serwisy)
- SCAN_RSP często zawiera pełną nazwę

**Wady:**
- Urządzenie "wie" że jest skanowane
- Większe zużycie energii

### Passive Scan

```
[Scanner] <──ADV_IND── [Device]
```

**Zalety:**
- Całkowicie pasywne
- Mniejsze zużycie energii

**Wady:**
- Mniej informacji

Zmień w `config.h`:
```cpp
#define BLE_ACTIVE_SCAN false  // Passive scan
```

## Wykrywani producenci

| Manufacturer ID | Producent |
|----------------|-----------|
| 0x004C | Apple |
| 0x00E0 | Google |
| 0x0006 | Microsoft |
| 0x0075 | Samsung |
| 0x0157 | Xiaomi |

## Wykrywane beacony

### iBeacon (Apple)
- Manufacturer ID: 0x004C
- Data prefix: 0x0215
- Format: UUID (16 bytes) + Major (2) + Minor (2) + TX Power (1)

### Eddystone (Google)
- Service UUID: 0xFEAA
- Typy: UID, URL, TLM, EID

## Format danych JSON

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

## Rozwiązywanie problemów

### "Mało urządzeń"

1. Włącz Bluetooth w pobliskich urządzeniach
2. Zmniejsz `MIN_RSSI` do -100
3. Zwiększ `SCAN_DURATION_SEC`

### "Brak nazw urządzeń"

1. Upewnij się że `BLE_ACTIVE_SCAN` = true
2. Wiele urządzeń używa Random Address i nie podaje nazwy
3. Apple urządzenia często ukrywają nazwę

### "Błąd pamięci"

1. Zmniejsz `MAX_DEVICES_BUFFER` do 50
2. NimBLE jest mniejsza niż ESP32 BLE

### "Upload nie działa"

1. Sprawdź czy serwer ma endpoint `/api/esp32/ble`
2. Sprawdź IP serwera
3. BLE jest wyłączane przed WiFi (oszczędność RAM)

## Porównanie: WiFi Probe vs BLE

| Cecha | WiFi Probe | BLE Scanner |
|-------|------------|-------------|
| Zasięg | ~50-100m | ~10-30m |
| Urządzenia | Wszystkie z WiFi | Tylko BLE |
| Informacje | SSID, MAC | Nazwa, serwisy, dane |
| Prywatność | MAC randomization | Też randomization |
| Beacony | Nie | Tak (iBeacon, Eddystone) |

## Licencja

MIT License
