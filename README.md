# esp8266-ota-flash-convert
This is the source to generate 1.bin for https://github.com/ct-Open-Source/tuya-convert
It was based on https://github.com/khcnz/Espressif2Arduino

## Install Arduino 1.8.8 in OS X
1.  Go to Preferences and add to "Additional Boards Manager URLs"
    http://arduino.esp8266.com/stable/package_esp8266com_index.json
2.  Go to "Board Manager"
3.  search esp
4.  install **esp8266 by ESP8266 Community** version **2.3**
5.  Install linker scripts from this repository
```console
cp boards.txt ~/Library/Arduino15/packages/esp8266/hardware/esp8266/2.3.0
cp *.ld ~/Library/Arduino15/packages/esp8266/hardware/esp8266/2.3.0/tools/sdk/ld
```
### Method 1
6.  Close Arduino IDE and create UPG file in the terminal:
```console
./create.sh
```

### Method 2
6.  Build esp8266-ota-flash-convert.ino using "1M (Espressif OTA Rom 1)"
7.  esptool.py elf2image --version 2 esp8266-ota-flash-convert.ino.elf

7.  Build esp8266-ota-flash-convert.ino using "1M (Espressif OTA Rom 2)"
8.  esptool.py elf2image --version 2 esp8266-ota-flash-convert.ino.elf

9.  Compile package with desired version number (make  clean && make USER_SW_VER="9.0.0")
10. package esp8266-ota-flash-convert.ino-0x01000.bin  esp8266-ota-flash-convert.ino-0x81000.bin UPGRADE.bin

