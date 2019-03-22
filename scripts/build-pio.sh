#!/bin/sh

mkdir -p build &&
pio run &&
mv .pioenvs/user1/firmware-0x01000.bin build/user1.bin &&
mv .pioenvs/user2/firmware-0x81000.bin build/user2.bin &&
scripts/create_upg.py

