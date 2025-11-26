# ESP32-C3 based SHUNT

## Flash

```bash
  esptool --after hard_reset --before default_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x0 shunt-firmware.bin
```

## Build

1. Create `data`

```bash
  mkdir data
```

2. Create merged bin

```bash
  pio run -t mergebin
```

3. Flash merged bin

```bash
  esptool --after hard_reset --before default_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x0 .pio/build/esp32-c3-devkitm-1/firmware-merged.bin
```

## Dev Build

1. Create `data` with `mkdir data` directory
2. Build FS `pio run -e esp32-c3-devkitm-1_debug --target buildfs`
3. Upload FS `pio run -e esp32-c3-devkitm-1_debug --target uploadfs`
4. Build and upload `pio run -e esp32-c3-devkitm-1_debug --target upload`
