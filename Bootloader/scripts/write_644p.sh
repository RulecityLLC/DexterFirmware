#!/bin/bash

# set fuse bits slowly
avrdude -V -B 8000 -p m644p -c dragon_isp -P usb -U efuse:w:efuse644.bin:r -U hfuse:w:hfuse644.bin:r -U lfuse:w:lfuse644.bin:r

# give time for USB to get re-inserted
sleep 2

# program flash and set lock bits faster now that fuse bits are set to use faster clock
avrdude -V -B 1 -p m644p -c dragon_isp -P usb -U lock:w:lock644.bin:r \
	-U flash:w:../bootloader-vldp-avr.hex:i
