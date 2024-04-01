#ifndef INTERFACE_H
#define INTERFACE_H

#include <ncurses.h>

#include "window.h"

extern Window ROM_win;
extern Window RAM_win;
extern Window MISC_win;

extern bool interface_quit;

void interface_main(void);

void init_curses(void);
void print_curses(void);

void printROM(void);
void printRAM(void);
void printMISC(void);
#endif
