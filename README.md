# MyOS
An OS project written in x86 assembly and C++20.

## Features
- A two-stage bootloader
- PS/2 Keyboard Driver
- Floppy Disk Driver
- Basic kmalloc/kfree implementation

## Dependencies
- NASM
- GCC
- make
- QEMU

A cross compiler is also needed which can be done by following https://wiki.osdev.org/GCC_Cross-Compiler.

## Building & Running
First build the cross compiler according to the OSDev wiki site. Then the OS can be compiled by running:
```
make
```

To run the OS do:
```
./start.sh
```

This will launch QEMU booting off the compiled disk image.
