# DexterFirmware
Firmware to be run on AVR microcontrollers used by Dexter, a hardware laserdisc player replacement.



# To build aux AVR
CC=avr-gcc cmake -DCMAKE_C_FLAGS="-mmcu=atmega328p -Wall -gdwarf-2 -std=gnu99 -DV3 -DF_CPU=16000000UL -O3 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums" -DCMAKE_BUILD_TYPE=Release ..
