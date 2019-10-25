#!/bin/sh

mkdir -p build &&
pio run &&
mv .pio/build/user1/firmware-0x01000.bin build/user1.bin &&
mv .pio/build/user2/firmware-0x81000.bin build/user2.bin &&
scripts/create_upg.py

