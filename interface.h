#ifndef INTERFACE_H
#define INTERFACE_H

#include <ncurses.h>


typedef struct Window
{
	WINDOW* win;
	int y, x;
} Window;

extern Window ROM_win;
extern Window RAM_win;
extern Window MISC_win;


extern bool interface_quit;

void interface_main();

void init_curses();
void print_curses();
void manage_input();

void create_ROM_window();
void create_RAM_window();
void create_MISC_window();

void print_to_window(Window* win, const char* str, bool endline);

void printROM();
void printRAM();
void printMISC();

void printend(const char* str);
#endif
