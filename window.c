#include "window.h"

#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


void create_window(Window* win, const char* win_title, int height, int width, int pos_y, int pos_x)
{
	win->win = newwin(height, width, pos_y, pos_x);
	win->def_cur_x = 2;
	win->def_cur_y = 1;
	win->cur_x = 2;
	win->cur_y = 1;
	win->width = width;
	win->height = height;
	set_window_title(win, win_title);
}

void print_to_window(Window* win, bool endline, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char outstr[256];
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

char* get_window_input_str(Window* win)
{
	return win->inp_text;
}

void clear_window_input_buffer(Window* win)
{
	strcpy(win->inp_text, "");
}

bool get_window_input(Window *win)
{
	/* char inp = mvwgetch(win->win, win->cur_y, win->cur_x); */
	char inp = getch();
	if ( inp == ERR )
		return false;
	if ( inp != '\n' )
	{
		// To check if the entered character is a backspace
		if ( inp =='\a' )
		{
			// Do not backspace if the string is already empty
			if ( strlen(win->inp_text) != 0 )
				win->inp_text[ strlen(win->inp_text) - 1 ] = '\0';
			return false;
		}
		sprintf(win->inp_text, "%s%c", win->inp_text, inp);
		return false;
	}
	else
		return true;
}

void set_window_title(Window* win, const char* win_title)
{
	strcpy(win->title, win_title);
}

void set_window_cursor(Window* win, int x, int y)
{
	win->cur_x = x;
	win->cur_y = y;
}

void move_window_cursor(Window* win, int x, int y)
{
	win->cur_x += x;
	win->cur_y += y;
}

void clear_window(Window* win)
{
	werase(win->win);
	box(win->win, 0, 0);
	mvwprintw(win->win, 0, 3, "%s", win->title);
	set_window_cursor(win, win->def_cur_x, win->def_cur_y);
}

void refresh_window(Window* win)
{
	wrefresh(win->win);
}
