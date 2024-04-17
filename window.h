#ifndef WINDOW_H
#define WINDOW_H

#include <ncurses.h>


typedef struct Window
{
	WINDOW* win;
	int width, height;
	int cur_y, cur_x;
	char window_title[30];
} Window;


void manage_input(void);

void set_window_title(Window* win, const char* win_title);
void move_window_cursor(Window* win, int x, int y);
void set_window_cursor(Window* win, int x, int y);

void create_window(Window* win, const char* win_title, int height, int width, int pos_x, int pos_y);
void print_to_window(Window* win, bool endline, const char* format, ...);

void refresh_window(Window* win);

void printend(const char* str);
#endif
