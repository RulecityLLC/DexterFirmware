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
