#!/usr/bin/env python3
# encoding: utf-8
"""
create_upg.py

Created by nano on 2019-01-28.
Contributions by kueblc
Copyright (c) 2019 __MyCompanyName__. All rights reserved.
"""

version = b"9.0.0"
section_count = 2
header_len = 0x33

"""
55 AA 55 AA | header_start
39 2E 30 2E 30 00 00 00 00 00 00 | version_string (example 1.0.0)
00 00 00 02 | section_count
00 00 00 33 | user1 start
00 03 E1 04 | user1 len
01 4D AC 94 | user1 sum
00 03 E1 37 | user2 start
00 03 E1 04 | user2 len
01 4D BE 3A | user2 sum
00 00 08 E7 | checksum
AA 55 AA 55 | header_end
"""

USERBIN_1 = ".pioenvs/user1/firmware-0x01000.bin"
USERBIN_2 = ".pioenvs/user2/firmware-0x81000.bin"
OUTBIN = "esp8266-ota-flash-convert_upg.bin"

def loadbin(filename):
    f = open(filename,"rb")
    contents = f.read()
    f.close()
    return contents

packInt = lambda x : x.to_bytes(4, byteorder='big')

def main():
    user1 = loadbin(USERBIN_1)
    user1_len = len(user1)
    user1_sum = sum(user1)
    print("user1 len:%x, sum:%x" % (user1_len,user1_sum))

    user2 = loadbin(USERBIN_2)
    user2_len = len(user2)
    user2_sum = sum(user2)
    print("user2 len:%x, sum:%x" % (user2_len,user2_sum))

    upg = b"\x55\xAA\x55\xAA" +\
        version.ljust(11, b"\0") +\
        packInt(section_count) +\
        packInt(header_len) +\
        packInt(user1_len) +\
        packInt(user1_sum) +\
        packInt(header_len + user1_len) +\
        packInt(user2_len) +\
        packInt(user2_sum)

    upg_sum = sum(upg)
    print("upgchk:%x" % (upg_sum))
    upg = upg + packInt(upg_sum) + b"\xAA\x55\xAA\x55"

    upgfile = open(OUTBIN,"wb")
    upgfile.write(upg)
    upgfile.write(user1)
    upgfile.write(user2)
    upgfile.close()

if __name__ == '__main__':
    main()

