#!/bin/bash

qemu-system-i386 -machine q35 -drive file=./build/bootdisk.img,format=raw,index=0,if=floppy -gdb tcp::26000 -S -D ./.log.txt 2>/dev/null 1>&2 &
