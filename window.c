#include "window.h"

#include <string.h>

void create_window(Window* win, int height, int width, int pos_y, int pos_x)
{
	win->win = newwin(height, width, pos_y, pos_x);
	win->cur_x = 2;
	win->cur_y = 1;
	win->width = width;
	win->height = height;
}

void print_to_window(Window* win, const char* str, bool endline)
{
	mvwprintw(win->win, win->cur_y, win->cur_x, "%s", str);
	int len = strlen(str);
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
