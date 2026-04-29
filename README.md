# Production Board HUB12 - ESP32 Firmware

**Version 2.1** | Rousis Systems LTD

ESP32-based production board firmware with web UI control, HUB12 LED matrix display, and real-time clock synchronization.

---

## 📋 Overview

This firmware manages a production board display system with:
- **Web UI Dashboard** (ESPUI) for remote control
- **HUB12 LED Matrix** (4×2 panels, 128×32 pixels)
- **DS1302 External RTC** for accurate timekeeping
- **Auto Brightness Control** via ambient light sensor
- **Persistent Settings** via EEPROM
- **Wi-Fi STA/AP** with captive portal

---

## ✨ Features

### Display Modes
- **LOT Mode** (Function=0): Real-time clock display with date, time, and day of year
- **Production Mode** (Function=1): Product labels and counter totals with thousand separators

### Web Interface Tabs
1. **Main Display** - Product labels, counters, brightness, sensor control
2. **User Settings** - Username and password management
3. **WiFi Settings** - SSID and Wi-Fi password configuration

### Smart Features
- Photo sensor-based auto brightness (60 samples averaging)
- Time sync from browser via "Synchronize Clock with your time" button
- DS1302 RTC backup
- Status LED feedback
- Graceful hotspot fallback when Wi-Fi unavailable

---

## 🔧 Hardware Requirements

| Component | Pin(s) | Notes |
|-----------|--------|-------|
| ESP32 | — | Main MCU |
| DS1302 RTC | CLK=GPIO4, DAT=GPIO32, RST=GPIO2 | External clock |
| HUB12 LED | SPI | 4×2 panel configuration |
| Photo Sensor | GPIO36 (ADC) | Ambient light input |
| Status LED | GPIO15 | Blink feedback |
| Boot Button | GPIO0 | Hold during upload |
| Hotspot Pin | GPIO36 | Dual-purpose with sensor |

---

## 📦 Libraries & Dependencies

Add via Arduino Library Manager or PlatformIO:

---

## ⚡ Installation & Setup

### 1. Clone/Download
### 2. Configure Arduino IDE / Visual Studio Code
- Board: **ESP32 Dev Module** (or variant)
- Upload Speed: **921600** (adjust if needed)
- Flash Size: **4MB**
- Partition Scheme: **Default 4MB with spiffs**

### 3. Install Libraries
### 4. Upload
### 5. First Boot
- Serial output shows IP address
- Open browser to `http://<IP_ADDRESS>` (default: `http://192.168.4.1` in AP mode)
- Login with default credentials:
  - **Username**: `espboard`
  - **Password**: `12345678`

---

## 🎛️ Web UI Usage

### Main Display Tab
- **Function** (Dropdown): Select LOT or Production mode
- **Label1A, Label1B** (Text): Product names (saved to EEPROM)
- **Total** (Number): Counter total (formatted with thousand separators)
- **Product 1-3** (Numbers): Individual product counters
- **Brightness** (Slider): Manual brightness 0-255
- **Auto Sensor Brightness** (Toggle): Enable/disable auto brightness
- **Clear All** (Button): Placeholder (not implemented)
- **Synchronize Clock** (Button): Push browser time to RTC

### User Settings Tab
- **Username & Password**: Update login credentials (stored in EEPROM)
- **Save** (Button): Persist to EEPROM

### WiFi Settings Tab
- **SSID & Password**: Configure network credentials
- **Save** (Button): Connect on next boot
- **Reset Device** (Button): Restart ESP32

---

## 💾 EEPROM Memory Map

| Address | Size | Purpose |
|---------|------|---------|
| 0-31 | 32 bytes | Wi-Fi SSID |
| 32-95 | 64 bytes | Wi-Fi Password |
| 128-159 | 32 bytes | Username |
| 160-191 | 32 bytes | User Password |
| 196 | 1 byte | Photo sensor on/off |
| 197 | 1 byte | Default login flag |
| 198 | 1 byte | Default Wi-Fi flag |
| 199 | 1 byte | Brightness value |
| 200 | 1 byte | Total prices flag |
| 201 | 1 byte | Address placeholder |
| 202 | 1 byte | **Selected Function (0=LOT, 1=Production)** |
| 203-212 | 10 bytes | Product Label 1A |
| 213-222 | 10 bytes | Product Label 1B |
| 223+ | — | Reserved |

---

## 🔌 Wi-Fi Configuration

### Station Mode (STA)
- Connects to stored SSID on boot
- Shows IP on LED matrix for 3 seconds
- If connection fails, switches to AP mode

### Access Point Mode (AP)
- **SSID**: `Rousis_Hotspot`
- **IP**: `192.168.4.1`
- **DNS**: Captive portal (auto-redirect to login page)
- Hold **GPIO36** to boot into AP mode

---

## 🕐 Time Synchronization

### Method 1: Browser Time Picker
1. Open Web UI → **Main Display**
2. Locate hidden time picker (or use button)
3. Click **Synchronize Clock with your time**
4. Browser time is sent to ESP32 and synced to DS1302

### Method 2: NTP (Auto on Boot)
- ESP32 queries `pool.ntp.org` on startup
- Timezone: `EET-2EEST,M3.5.0/3,M10.5.0/4` (Greece)
- External RTC updated via `SetExtRTC()`

### Timezone Adjustment
Edit in sketch:
---

## 🔍 Serial Monitor Output

On boot, expect:
---

## ⚙️ Configuration

### LED Display
- **Panels**: 4 across, 2 down (128×32 pixels)
- **Brightness Range**: 0-255
- **Fonts**: System5x7, scoreboard_12, Big_font

### Photo Sensor
- **Sampling**: 60 samples, rolling average
- **Threshold**: Values <200 reset to 1
- **Range**: Maps 4095 ADC → 0-255 brightness

### Display Update Interval
---

## 🐛 Troubleshooting

### No Web UI Connection
- Check ESP32 is online: `ping <IP>`
- Verify EEPROM credentials are correct
- Try factory reset: Hold GPIO0, press reset

### Time Not Syncing
- Ensure NTP access (port 123 UDP)
- Check timezone setting matches your region
- Use button to manually sync from browser

### LED Display Not Showing
- Verify HUB12 wiring (CLK, DAT, select pins)
- Check DMD32_B library is installed
- Adjust brightness slider to >50

### EEPROM Errors
- Verify EEPROM.begin(256) in setup()
- Check for corruption: reset via serial command

---

## 📝 Notes

- **Default Hotspot Password**: Change in `#define password "jea899hphcbk"`
- **Hostname**: Default `espui` (mDNS discovery)
- **Daylight Saving**: Hardcoded April-October (edit `isDaylightSavingTime()`)
- **HTTPS**: Not implemented; use on trusted networks only

---

## 📄 License

Rousis Systems LTD - Contact for licensing details.

---

## 🤝 Support

For issues or feature requests, refer to:
- ESPUI GitHub: https://github.com/ayushsharma82/ESPUI
- DMD32_B Docs: Check library repository
- ESP32 Docs: https://docs.espressif.com/projects/esp-idf/

---

**Last Updated**: April 29, 2026  
**Firmware Version**: 2.1  
**Author**: ROUSIS_FACTORY

