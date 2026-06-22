| Supported Targets | ESP32-S3 |
| ----------------- | -------- |

# LVGL Example — ST7789 + GT911 + TinyUSB

## Overview

This example demonstrates LVGL (v8.4.0) graphical user interface on an ESP32-S3 with:

- **ST7789** SPI LCD (320×240) — display driver with DMA-backed flushing
- **GT911** capacitive touch (I2C) — multi-touch input with debounce
- **TinyUSB** CDC virtual serial port — debug logging via USB

The UI shows a simple demo page with interactive buttons to verify display, touch, and USB serial functionality.

## Hardware Required

- ESP32-S3 development board
- ST7789 SPI LCD (320×240)
- GT911 capacitive touch sensor (I2C)

### Pin Connections

#### LCD (ST7789) — SPI

| Signal | GPIO Pin |
|--------|----------|
| MOSI   | 36       |
| CLK    | 35       |
| CS     | 37       |
| DC     | 38       |
| RST    | 47       |
| BL     | 48       |

#### Touch (GT911) — I2C

| Signal | GPIO Pin |
|--------|----------|
| SDA    | 2        |
| SCL    | 1        |
| INT    | 21       |
| RST    | 14       |

## Usage

### Build & Flash

```bash
# From the esp32libraries directory
cd example/LVGL

# Set target (ESP32-S3)
idf.py set-target esp32s3

# Build, flash, and monitor
idf.py build flash monitor
```

### Expected Output

After flashing, the LCD will:
1. Display a color test pattern (red → green → blue → white → black)
2. Show the LVGL demo page with title, icons, and buttons
3. Respond to touch input on the buttons
4. Output debug logs via USB serial (TinyUSB CDC)

## Key Dependencies

| Component | Version | Purpose |
|-----------|---------|---------|
| ESP-IDF   | v5.5.1  | Framework |
| LVGL      | ^8.4.0  | GUI library |
| ST7789    | —       | SPI LCD controller |
| GT911     | —       | Capacitive touch |
| TinyUSB   | —       | USB CDC serial |

## Reference

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [LVGL Documentation](https://docs.lvgl.io/)