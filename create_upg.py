#!/usr/bin/env python
# encoding: utf-8
"""
create_upg.py

Created by nano on 2019-01-28.
Copyright (c) 2019 __MyCompanyName__. All rights reserved.
"""

import sys
import os
from binascii import unhexlify

version = "9.0.0"
section_count = 2
header_len = 0x33

"""
55 AA 55 AA | header_start
39 2E 30 2E 30 00 00 00 00 00 00 | version_string (example 1.0.0)
00 00 00 02 | section_count
00 00 00 33 | user1 start
00 03 E1 04 | user1 len
01 4D AC 94 | user1 chk
00 03 E1 37 | user2 start
00 03 E1 04 | user2 len
01 4D BE 3A | user2 sum
00 00 08 E7 | checksum
AA 55 AA 55 | header_end
"""

def main():
    upg = unhexlify("55AA55AA") 
    upg = upg + version + (11 - len(version)) * chr(0)
    upg = upg + unhexlify("%08X" % section_count)
    upg = upg + unhexlify("%08X" % header_len)

    user1_file = open("esp8266-ota-flash-convert.ino-0x01000.bin","rb")
    user1 = user1_file.read()
    user1_file.close()
    user1_chk = 0
    for x in user1:
        user1_chk = user1_chk + ord(x)
    print("user1 len:%x, chk:%x" % (len(user1),user1_chk))

    user2_file = open("esp8266-ota-flash-convert.ino-0x81000.bin","rb")
    user2 = user2_file.read()
    user2_file.close()
    user2_chk = 0
    for x in user2:
        user2_chk = user2_chk + ord(x)
    print("user2 len:%x, chk:%x" % (len(user2),user2_chk))


    upg = unhexlify("55AA55AA") 
    upg = upg + version + (11 - len(version)) * chr(0)
    upg = upg + unhexlify("%08X" % section_count)
    upg = upg + unhexlify("%08X" % header_len)
    upg = upg + unhexlify("%08X" % len(user1))
    upg = upg + unhexlify("%08X" % user1_chk)
    upg = upg + unhexlify("%08X" % (len(user1) + header_len))
    upg = upg + unhexlify("%08X" % len(user2))
    upg = upg + unhexlify("%08X" % user2_chk)

    upg_chk = 0
    for x in upg:
        upg_chk = upg_chk + ord(x)
    print("upgchk:%x" % (upg_chk))
    upg = upg + unhexlify("%08X" % upg_chk)
    upg = upg + unhexlify("AA55AA55") 

    upgfile = open("esp8266-ota-flash-convert_upg.bin","wb")
    upgfile.write(upg)
    upgfile.write(user1)
    upgfile.write(user2)
    upgfile.close()

    pass


if __name__ == '__main__':
    main()

