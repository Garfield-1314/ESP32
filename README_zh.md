# ESP32 综合开发工程

[English](./README.md) | 中文

本项目包含基于 ESP32-S3 的多个开发示例，主要涵盖了 LVGL 图形界面集成、触摸驱动、TinyUSB 虚拟串口输出以及 DVP 摄像头与 SPI LCD 的联合应用。

## 目录结构

- [esp-idf/](esp-idf/): Git 管理的 ESP-IDF 框架子模块 (v5.5.1)。
- [example/](example/): 示例工程目录。
    - [example/LVGL/](example/LVGL/): 核心图形项目，包含：
        - LVGL (v8.4.0) 图形库集成。
        - **ST7789** LCD 驱动 (320x240, SPI)。
        - **GT911** 电容触摸驱动 (I2C)。
        - **TinyUSB** CDC 虚拟串口实现。
        - 多页面 UI 设计 (主页、应用页)。
    - [example/dvp_spi_lcd/](example/dvp_spi_lcd/): 硬件整合示例，演示如何获取 DVP 摄像头数据并在 SPI 接口的 LCD (ST7789) 上实时显示。

## 环境搭建

确保您的开发环境已安装必要的依赖。本工程推荐使用 **ESP-IDF v5.5.1**。

### 1. 初始化并克隆子模块
```bash
# 初始化并更新子模块
git submodule init 
git submodule update --progress

# 切换到指定的 ESP-IDF 版本
cd esp-idf
git checkout v5.5.1
git submodule update --init --recursive --progress

# 安装工具链 
./install.sh 
```

### 2. 导出环境变量
每次在新的终端窗口开始工作前，需要导出 IDF 路径：
```bash
cd ..
source ./esp-idf/export.sh
```

## 项目运行

### LVGL 示例
该项目演示了如何在 ESP32-S3 上运行 UI，并支持通过 USB 虚拟串口进行通信。
```bash
cd example/LVGL
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

### DVP 摄像头显示示例
该示例演示了将 DVP 摄像头图像刷新到 SPI LCD。
```bash
cd example/dvp_spi_lcd
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

## 硬件配置

### LCD (ST7789)
- **接口**: SPI
- **分辨率**: 320x240
- **引脚**:
    - MOSI: 38
    - CLK: 21
    - CS: 40
    - DC: 41
    - RST: 46
    - BL: 48

### 触摸 (GT911)
- **接口**: I2C
- **引脚**:
    - SDA: 2
    - SCL: 1
    - INT: 45
    - RST: 37

## 声明
本项目部分代码基于 Espressif Systems 的示例进行开发。
