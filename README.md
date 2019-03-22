# esp8266-ota-flash-convert
Intermediate upgrade firmware for ESP8266 based Tuya devices to easily backup and replace stock firmware

Used for OTA flashing by https://github.com/ct-Open-Source/tuya-convert

It was based on https://github.com/khcnz/Espressif2Arduino

## Setup
### Method 1 - PlatformIO (recommended)
1. Install `platformio`

### Method 2 - Arduino
1.  Install Arduino 1.8.8
2.  Go to Preferences and add to "Additional Boards Manager URLs"
    http://arduino.esp8266.com/stable/package_esp8266com_index.json
3.  Go to "Board Manager" and search "esp"
4.  Install **esp8266 by ESP8266 Community** version **2.3**
5.  Install linker scripts from this repository
```console
cp files/boards.txt $ARDUINO_PATH/packages/esp8266/hardware/esp8266/2.3.0
cp files/*.ld $ARDUINO_PATH/packages/esp8266/hardware/esp8266/2.3.0/tools/sdk/ld
```

## Building
### Method 1 - PlatformIO (recommended)
2.  Build and package
```console
pio run && scripts/create_upg.py
```

### Method 2 - Arduino CLI
6.  Edit `$ARDUINO_PATH` in `scripts/build.sh` to point to the arduino executable
7.  Build and package
```console
scripts/build.sh
```

### Method 3 - Arduino IDE
6.  Build esp8266-ota-flash-convert.ino using Flash Size "1M (Espressif OTA Rom 1)"
7.  Convert to a version 2 binary
```console
esptool.py elf2image --version 2 esp8266-ota-flash-convert.ino.elf
```
8.  Build esp8266-ota-flash-convert.ino using Flash Size "1M (Espressif OTA Rom 2)"
9.  Convert to a version 2 binary
```console
esptool.py elf2image --version 2 esp8266-ota-flash-convert.ino.elf
```
10.  Package UPG
```console
scripts/create_upg.py
```

## Installing
1.  Copy `.bin` files to the `files` directory under `tuya-convert`

