#!/bin/bash

AVR_TOOL_PATH="/c/Program Files (x86)/Arduino/hardware/tools/avr/bin"
AVR_INCLUDE_PATH="/c/Program Files (x86)/Arduino/hardware/tools/avr/avr/include"

PATH="$PATH:$AVR_TOOL_PATH"

avr-gcc -Wall -g -Os -mmcu=atmega8 -I"$AVR_INCLUDE_PATH" -o main.bin main.cpp

avr-objcopy -j .text -j .data -O ihex main.bin main.hex

# avr-objdump -d main.bin