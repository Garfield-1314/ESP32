# ESP32 Comprehensive Development Project

English | [中文版](./README_zh.md)

This project contains multiple development examples based on ESP32-S3, covering LVGL GUI integration, touch drivers, TinyUSB virtual serial port output, and the combined application of DVP camera and SPI LCD.

## Directory Structure

- [esp-idf/](esp-idf/): ESP-IDF framework submodule (v5.5.1).
- [example/](example/): Example projects directory.
    - [example/LVGL/](example/LVGL/): Core graphics project, including:
        - LVGL (v8.4.0) integration.
        - **ST7789** LCD driver (320x240, SPI).
        - **GT911** capacitive touch driver (I2C).
        - **TinyUSB** CDC virtual serial port.
        - Multi-page UI design (Main page, App page).
    - [example/dvp_spi_lcd/](example/dvp_spi_lcd/): Hardware integration example, demonstrating capturing DVP camera data and displaying it in real-time on an SPI LCD (ST7789).

## Environment Setup

Ensure your development environment has the necessary dependencies installed. **ESP-IDF v5.5.1** is recommended.

### 1. Initialize and Clone Submodules
```bash
# Initialize and update submodules
git submodule init 
git submodule update --progress

# Switch to specific ESP-IDF version
cd esp-idf
git checkout v5.5.1
git submodule update --init --recursive --progress

# Install toolchain 
./install.sh 
```

### 2. Export Environment Variables
Export the IDF path before starting work in a new terminal window:
```bash
cd ..
source ./esp-idf/export.sh
```

## Project Usage

### LVGL Example
Demonstrates running UI on ESP32-S3 with USB virtual serial communication.
```bash
cd example/LVGL
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

### DVP Camera & LCD Example
Demonstrates refreshing DVP camera images to an SPI LCD.
```bash
cd example/dvp_spi_lcd
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

## Hardware Configuration

### LCD (ST7789)
- **Interface**: SPI
- **Resolution**: 320x240
- **Pins**:
    - MOSI: 38
    - CLK: 21
    - CS: 40
    - DC: 41
    - RST: 46
    - BL: 48

### Touch (GT911)
- **Interface**: I2C
- **Pins**:
    - SDA: 2
    - SCL: 1
    - INT: 45
    - RST: 37

## Disclaimer
Parts of this project are based on examples from Espressif Systems.
