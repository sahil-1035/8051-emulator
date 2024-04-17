#include "window.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void create_window(Window* win, int height, int width, int pos_y, int pos_x)
{
	win->win = newwin(height, width, pos_y, pos_x);
	win->cur_x = 2;
	win->cur_y = 1;
	win->width = width;
	win->height = height;
}

void print_to_window(Window* win, bool endline, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char outstr[30];
	vsnprintf(outstr, sizeof(outstr), format, args);
	va_end(args);

	mvwprintw(win->win, win->cur_y, win->cur_x, "%s", outstr);
	int len = strlen(outstr);
	if ( endline )
	{
		win->cur_y++;
		win->cur_x = 2;
	}
	else
		win->cur_x += len;
}

void printend(const char* str)
{
	int height, width;
	getmaxyx(stdscr, height, width);
	mvprintw(height - 1, 0, "%s", str);
}

void refresh_window(Window* win)
{
	wrefresh(win->win);
}
