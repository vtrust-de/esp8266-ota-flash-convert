#!/bin/sh

ARDUINO_PATH="/esp"
SDK_VERSION="2.3.0"
#SDK_VERSION="2.5.0"

cp ../files/boards.txt $ARDUINO_PATH/portable/packages/esp8266/hardware/esp8266/$SDK_VERSION
cp ../files/*.ld $ARDUINO_PATH/portable/packages/esp8266/hardware/esp8266/$SDK_VERSION/tools/sdk/ld

