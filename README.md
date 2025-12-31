# ESP32 Project Setup

## Environment Setup

Follow these steps to initialize the ESP-IDF submodule and install dependencies for ESP32-S3.

```bash
# Initialize and update submodules
git submodule init
git submodule update

# Switch to the specific ESP-IDF version
cd esp-idf
git checkout v5.5.1
git submodule update --init --recursive

# Install tools for ESP32-S3
./install.sh esp32s3
```
