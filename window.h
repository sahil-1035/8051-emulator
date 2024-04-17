#ifndef WINDOW_H
#define WINDOW_H

#include <ncurses.h>


typedef struct Window
{
	WINDOW* win;
	int width, height;
	int cur_y, cur_x;
} Window;

void manage_input(void);

void create_window(Window* win, int height, int width, int pos_x, int pos_y);
void print_to_window(Window* win, bool endline, const char* format, ...);

void refresh_window(Window* win);

void printend(const char* str);
#endif
