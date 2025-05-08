# DexterFirmware
Firmware to be run on AVR microcontrollers used by Dexter, a hardware laserdisc player replacement.

# To build
See https://github.com/RulecityLLC/DexterBuilder

# Installation

## To install vldp-avr.bin and vbi-inject.bin files
To install files ending in .bin, put them inside of the '/dexter/firmware/' directory of a Dexter USB drive.  Dexter will automatically reprogram itself when these firmware files change.

## To install .hex files

**NOTE:** All Dexters ship with these .hex files already installed/programmed and they never need to be updated.  This means that the only time you would need to install/program the .hex files on a Dexter is if you are building a Dexter from scratch.

The .hex files represent the two microcontroller bootloaders.

Each of the Dexter microcontrollers contains a bootloader which is a small program that sits at the top of the flash memory and allows the rest of the microcontroller's program memory to be reprogrammed via serial lines (currently using the Raspberry Pi that sits on top of the Dexter custom PCB).  This is convenient because it allows anyone to reprogram their Dexter without having a hardware programmer that plugs into the custom Dexter PCB.

However, both microcontrollers do need to have the bootloader programmed to them one time.

To install .hex files, you need

- a hardware AVR programmer, such as the AVR Dragon ( https://www.microchip.com/en-us/development-tool/atavrdragon ).
- a software utility to use with your hardware programmer, such as AVRDude ( https://github.com/avrdudes/avrdude ).
The output folder will include scripts to install/program these .hex files.

Once one has built the bootloaders, the output folder will contain the bootloader .hex files as well as a 'scripts' subdirectory.  Inside this scripts subdirectory, you will find at least two scripts:
- write644p.sh which is used to program the ATMega644p microcontroller (located at the center of the custom Dexter PCB)
- write328p.sh which is used to program the ATMega328p microcontroller (located at the left of the custom Dexter PCB)

These scripts are designed to work with an AVR Dragon and AVRDude.  You can modify them to work with other solutions.

# Explanation of what gets built and how to use it
## vldp-avr.bin
This is the firmware for the main microcontroller on the Dexter PCB.
## vbi-inject.bin
This is the firmware for the auxiliary microcontroller on the Dexter PCB.  
## bootloader-vldp-avr.hex
This is the bootloader for the main microcontroller on the Dexter PCB.
## bootloader-vbi-inject.hex
This is the bootloader for the auxiliary microcontroller on the Dexter PCB.

# Advanced

## To use C compiler to generate assembly language source code

Use "make VERBOSE=1" to get the command line that the C compiler uses to build the C code that you want to convert to assembly language.
For example, you may see something like this:

```
cd /dexter-firmware/MainAVR/build-avr-release/src && /usr/bin/avr-gcc  -isystem /usr/local/include/ldp_in-1.0 -isystem /usr/local/include/ldp_abst-1.0 -mmcu=atmega644p -Wall -gdwarf-2 -std=gnu99 -DREV3 -DF_CPU=18432000UL -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -O3 -DNDEBUG -MD -MT src/CMakeFiles/avr_main.dir/ld700-main.c.o -MF CMakeFiles/avr_main.dir/ld700-main.c.o.d -o CMakeFiles/avr_main.dir/ld700-main.c.o -c /dexter-firmware/MainAVR/src/ld700-main.c
```

Add a -S somewhere on the command line and change the output file that ends in .o to .s .
For example,
```
cd /dexter-firmware/MainAVR/build-avr-release/src && /usr/bin/avr-gcc  -isystem /usr/local/include/ldp_in-1.0 -isystem /usr/local/include/ldp_abst-1.0 -mmcu=atmega644p -Wall -gdwarf-2 -std=gnu99 -DREV3 -DF_CPU=18432000UL -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -O3 -DNDEBUG -MD -MT src/CMakeFiles/avr_main.dir/ld700-main.c.o -MF CMakeFiles/avr_main.dir/ld700-main.c.o.d -S -o CMakeFiles/avr_main.dir/ld700-main.c.s -c /dexter-firmware/MainAVR/src/ld700-main.c
```

Now you can use your favorite text editor to examine the generated assembly language.

```
nano -w CMakeFiles/avr_main.dir/ld700-main.c.s
```
## Building/debugging firmware

I use a command line like this from the root folder of this repo:
```
docker run --rm -v ${PWD}:/dexter-firmware -v /c/projects/LaserdiscPlayerCommandInterpreters:/ldp-in -v /c/projects/LaserDiscPlayerAbstract:/ldp-abst -ti mpownby/dexter-builder /bin/bash
```
