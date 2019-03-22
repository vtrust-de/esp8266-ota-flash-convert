#!/bin/sh


cp ./preferences_OTA1.txt ~/Library/Arduino15/preferences.txt
/Applications/Arduino.app/Contents/MacOS/Arduino --pref build.path=OTA1 --verify esp8266-ota-flash-convert/esp8266-ota-flash-convert.ino
cd OTA1
esptool.py elf2image --version 2 esp8266-ota-flash-convert.ino.elf
cp esp8266-ota-flash-convert.ino-0x01000.bin ../
cd ..
rm -r OTA1

cp ./preferences_OTA2.txt ~/Library/Arduino15/preferences.txt
/Applications/Arduino.app/Contents/MacOS/Arduino --pref build.path=OTA2 --verify esp8266-ota-flash-convert/esp8266-ota-flash-convert.ino
cd OTA2
esptool.py elf2image --version 2 esp8266-ota-flash-convert.ino.elf
cp esp8266-ota-flash-convert.ino-0x81000.bin ../
cd ..
rm -r OTA2

python create_upg.py

