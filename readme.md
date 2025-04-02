**ESP32-C3 based SHUNT**

**Features:**

- esp32-c3
- Step down converter 6-32V input
- BLE or Wifi -> PCB antenna
- INA226 sensor
- WS2812b status led.
- USB-C for programming


Currently works with a 200A/75MV shunt.

**First software**

- BLE broadcast
- read SOC, AMPS, VOLT
- Store battery size in AH
- Set SOC
- Read via NRF connect or Python scripts (see REPO)
- Store SOC in SPIFFS file system

**Bugs and wish list:**

- History data
- Improve BLE services
- Wifi HTTP/MQTT version
- case
- Serial data output
- Power consumption reduction

**Known issues:**

- WS28b12 connected to SPI Flash pin 24, this causes a problem and the ESP will not boot.
- DeepSleep Flash problem with ESP32-C3 USB-C port
- own consumption is not counted
- below 100mA accuracy decreases

**Hints:**

If the COM port is not found, rotate the USB-C cable. Not all pins are used.

## Dev Build

1. Create `data` with `mkdir data` directory
2. Build FS `pio run -e esp32-c3-devkitm-1_debug --target buildfs`
3. Upload FS `pio run -e esp32-c3-devkitm-1_debug --target uploadfs`
4. Build and upload `pio run -e esp32-c3-devkitm-1_debug --target upload`
