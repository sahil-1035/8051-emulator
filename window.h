#ifndef WINDOW_H
#define WINDOW_H

#include <ncurses.h>


typedef struct Window
{
	WINDOW* win;
	int width, height;
	int cur_y, cur_x;
	char title[30];
	char inp_text[256];
} Window;

void manage_input(void);

void set_window_title(Window* win, const char* win_title);
void move_window_cursor(Window* win, int x, int y);
void set_window_cursor(Window* win, int x, int y);

void clear_window_input_buffer(Window* win);
char* get_window_input_str(Window* win);
bool get_window_input(Window *win);

void create_window(Window* win, const char* win_title, int height, int width, int pos_x, int pos_y);
void print_to_window(Window* win, bool endline, const char* format, ...);

void clear_window(Window* win);
void refresh_window(Window* win);

void printend(const char* str);
#endif
