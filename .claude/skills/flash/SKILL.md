---
name: flash
description: Flash firmware and filesystem to ESP32-C3 device. Use when uploading, flashing, or programming the device.
disable-model-invocation: true
allowed-tools: Bash Read
argument-hint: "[fs|monitor]"
---

# Flash Firmware to Device

Upload firmware and/or filesystem to the ESP32-C3 device.

## Steps

Based on argument:
- No argument: `pio run -t upload -e esp32c3` (firmware only)
- `fs`: `pio run -t uploadfs -e esp32c3` (LittleFS filesystem with config.json)
- `monitor`: `pio run -t upload -e esp32c3 && pio device monitor -b 115200`
- `all`: upload firmware, then filesystem, then open monitor

## Pre-flight Checks
1. Confirm device is connected via USB
2. Check that `pio device list` shows the ESP32-C3
3. If upload fails with "connection timeout", hold BOOT button while uploading

## After Flash
- Monitor serial at 115200 baud: `pio device monitor -b 115200`
- Expected boot output: `MyAthan Firmware v1.0.0`
- Verify config loads: look for `[Config] Loaded. Device: myathan-XXXXXX`
