#!/bin/bash

avrdude -V -B 8000 -p m328p -c dragon_isp -P usb -U efuse:w:efuse328.bin:r -U hfuse:w:hfuse328.bin:r -U lfuse:w:lfuse328.bin:r

sleep 2

avrdude -V -B 1 -p m328p -c dragon_isp -P usb -U lock:w:lock328.bin:r -U flash:w:../bootloader-vbi-inject.hex:i
