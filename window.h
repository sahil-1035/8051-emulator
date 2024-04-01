#ifndef WINDOW_H
#define WINDOW_H

#include <ncurses.h>

#define PRINT_IN_WIN(window, endline, args...) \
{ \
	char tmp_str[30]; \
	sprintf(tmp_str, args); \
	print_to_window(window, tmp_str, endline); \
}

typedef struct Window
{
	WINDOW* win;
	int width, height;
	int cur_y, cur_x;
} Window;

void manage_input(void);

void create_window(Window* win, int height, int width, int pos_x, int pos_y);
void print_to_window(Window* win, const char* str, bool endline);

void printend(const char* str);
#endif
