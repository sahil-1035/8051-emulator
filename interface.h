#ifndef INTERFACE_H
#define INTERFACE_H

#include <ncurses.h>

extern WINDOW* ROM_win;
extern WINDOW* RAM_win;
extern WINDOW* MISC_win;

extern bool interface_quit;

void interface_main();

void init_curses();
void print_curses();
void manage_input();

void create_ROM_window();
void create_RAM_window();
void create_MISC_window();

void printROM();
void printRAM();
void printMISC();

void printend(const char* str);
#endif
