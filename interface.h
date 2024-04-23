#ifndef INTERFACE_H
#define INTERFACE_H

#include <ncurses.h>

#include "window.h"

extern Window *ROM_win, *RAM_win, *MISC_win, *POPUP_win;

extern bool interface_quit;

void interface_main(void);

void init_curses(void);
void print_curses(void);

void printROM(void);
void printRAM(void);
void printMISC(void);
void printPOPUP(void);
void printHELP(void);
#endif
