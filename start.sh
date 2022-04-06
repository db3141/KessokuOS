#!/bin/bash

qemu-system-i386 -machine q35 -drive file=./build/bootdisk.img,format=raw,index=0,if=floppy
