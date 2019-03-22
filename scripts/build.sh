#!/bin/sh

ARDUINO_PATH="/esp/arduino"
#FLASH_PROPERTY="custom_eesz"
FLASH_PROPERTY="custom_FlashSize"

cd ..

mkdir -p build

build_user () {
	echo "==============="
	echo "Compiling user$1"
	echo "==============="
	$ARDUINO_PATH --pref build.path=build/user$1 --pref $FLASH_PROPERTY=generic_1M0_$1 --verify esp8266-ota-flash-convert/esp8266-ota-flash-convert.ino &&
	esptool.py elf2image --version 2 build/user$1/esp8266-ota-flash-convert.ino.elf &&
	mv build/user$1/esp8266-ota-flash-convert.ino-0x*1000.bin .
}

build_user 1
build_user 2

echo "================="
echo "Packaging Upgrade"
echo "================="
./create_upg.py

