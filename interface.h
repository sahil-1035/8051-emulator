#ifndef INTERFACE_H
#define INTERFACE_H

#include <ncurses.h>


typedef struct Window
{
	WINDOW* win;
	int width, height;
	int cur_y, cur_x;
} Window;

extern Window ROM_win;
extern Window RAM_win;
extern Window MISC_win;


extern bool interface_quit;

void interface_main(void);

void init_curses(void);
void print_curses(void);
void manage_input(void);

void create_window(Window* win, int height, int width, int pos_x, int pos_y);

void print_to_window(Window* win, const char* str, bool endline);

void printROM(void);
void printRAM(void);
void printMISC(void);

void printend(const char* str);
#endif
