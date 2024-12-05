# DexterFirmware
Firmware to be run on AVR microcontrollers used by Dexter, a hardware laserdisc player replacement.

# To build bootloader for main AVR
CC=avr-gcc cmake -DCMAKE_C_FLAGS="-mmcu=atmega644p -Wall -gdwarf-2 -std=gnu99 -DREV3 -DF_CPU=18432000UL -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums" -DCMAKE_C_FLAGS_RELEASE="-Os -DNDEBUG" -DCMAKE_EXE_LINKER_FLAGS="-Wl,--section-start=.text=0xF800" -DCMAKE_BUILD_TYPE=Release ..

# To build aux AVR
CC=avr-gcc cmake -DCMAKE_C_FLAGS="-mmcu=atmega328p -Wall -gdwarf-2 -std=gnu99 -DV3 -DF_CPU=16000000UL -O3 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums" -DCMAKE_BUILD_TYPE=Release ..

# To build bootloader for aux AVR
CC=avr-gcc cmake -DCMAKE_C_FLAGS="-mmcu=atmega328p -Wall -gdwarf-2 -std=gnu99 -DREV3 -DPLATFORM_VBI_INJECT -DF_CPU=16000000UL -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums" -DCMAKE_C_FLAGS_RELEASE="-Os -DNDEBUG" -DCMAKE_EXE_LINKER_FLAGS="-Wl,--section-start=.text=0x7800" -DCMAKE_BUILD_TYPE=Release -DBUILD_AUX=1 ..
