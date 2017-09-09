#!/bin/sh

# This script converts the softdevice.hex file into a softdevice.o ELF
# object that we can link with ChibiOS to produce a complete OS image.

arm-none-eabi-objcopy -I ihex -O binary s132_nrf52_5.0.0_softdevice.hex softdevice.bin
arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm softdevice.bin softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --rename-section .data=.softdevice softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --set-section-flags .softdevice=CONTENTS,ALLOC,LOAD,CODE softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --redefine-sym _binary_softdevice_bin_start=softdeviceEntry softdevice.o
