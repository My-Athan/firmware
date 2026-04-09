---
name: build
description: Build the ESP32-C3 firmware using PlatformIO. Use when compiling, building, or checking for errors.
disable-model-invocation: true
allowed-tools: Bash Read Glob
argument-hint: "[clean]"
---

# Build Firmware

Compile the MyAthan firmware for ESP32-C3 using PlatformIO.

## Steps

1. If argument is "clean", run `pio run -t clean -e esp32c3` first
2. Run `pio run -e esp32c3`
3. Report the build result:
   - If success: report binary size from `.pio/build/esp32c3/firmware.bin` (must be <=1.5MB for OTA)
   - If failure: analyze the error, suggest a fix, and show the relevant source code

## Binary Size Tracking
- OTA partition limit: 1.5MB (1,572,864 bytes)
- Report: `ls -la .pio/build/esp32c3/firmware.bin`
- Warn if binary exceeds 1.2MB (80% of OTA limit)

## Common Issues
- Missing `#include` — check if a new header was added without including it
- UART pin conflicts — GPIO6/7 are DFPlayer UART1, don't use for other purposes
- LittleFS size — config.json max is 8192 bytes, check CONFIG_MAX_SIZE
- ArduinoJson memory — JsonDocument default is 4096, may need increase for large configs
