# 8051-emulator
This is an 8051 emulator I build from scratch, all the instructions of the 8051 microcontroller and its functionalities have been rewritten in C.

This program uses the Ncurses library to display the ROM, RAM and other important registers.

Steps to build:
```
  mkdir build
  cmake -B build
  make -C build
```
### What doesn't work
+ Timers
+ Interupts
+ Serial Communication
+ I/O Ports
