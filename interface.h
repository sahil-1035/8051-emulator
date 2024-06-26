#ifndef INTERFACE_H
#define INTERFACE_H

#include <ncurses.h>

#include "window.h"

extern Window *ROM_win, *RAM_win, *MISC_win, *CMD_win, *HELP_win, *PORT_win, *TIMER_win;

extern bool interface_quit;

void interface_main(const char* bin_path);

void init_curses(void);
void print_curses(void);

void printROM(void);
void printRAM(void);
void printMISC(void);
void printCMD(void);
void printHELP(void);
void printPORTS(void);
void printTIMER(void);
#endif
