#include <stdio.h>
#include <ncurses.h>

#include "interface.h"

#include "emulator.h"

WINDOW* ROM_win;
WINDOW* RAM_win;
WINDOW* MISC_win;

bool quit;


void interface_main()
{
	init_curses();

	emu_load_ROM("p2");
	change_bank(0);
	emu_clear_ram();
	pc = 0x03ab;
	while(!quit)
	{
		if(pc == ROM_SIZE - 1)
		{
			fprintf(stderr, "end of ROM reached pc=%0X\n", pc);
			break;
		}
		if(rom[pc] == 0x6f && rom[pc + 1] == 0x22)
			break;

		print_curses();
		emu_exec_instr();

		manage_input();
	}
	printend("End of ROM reached. -- Enter any key to exit --");
	getch();
	endwin();
}


void init_curses()
{
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	refresh();

	create_ROM_window();
	create_RAM_window();
	create_MISC_window();
}

void print_curses()
{
	printRAM();
	printROM();
	printMISC();
}

void manage_input()
{
	int inp;// = getch();
	if ( (inp = getch()) == KEY_F(1) )
		quit = true;
}

void create_ROM_window()
{
	int height, width;
	getmaxyx(stdscr, height, width);
	ROM_win = newwin(height - 2, 105, 0, 0);
}
void create_RAM_window()
{
	int height, width;
	getmaxyx(stdscr, height, width);
	RAM_win = newwin( 18, width - 106 - 11, 0, 105);
}
void create_MISC_window()
{
	int height, width;
	getmaxyx(stdscr, height, width);
	MISC_win = newwin( 17, width - 106 - 11, 18, 105);
}

void printMISC()
{
	werase(MISC_win);

	int cur_col = 2, cur_row = 1;
	box(MISC_win, 0, 0);
	mvwprintw(MISC_win, cur_col, cur_row, "Next instr - %s", get_instruction[rom[pc]]);
	wrefresh(MISC_win);
}

void printROM()
{
	const unsigned int ROM_WIDTH = 32;

	box(ROM_win, 0 , 0);
	mvwprintw(ROM_win, 0, 3, " ROM ");

	start_color();
	int cur_col = 2, cur_row = 1;
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_BLACK, COLOR_BLUE);

	unsigned int romptr = 0;
	for (unsigned int i = 0; i < 1 + (ROM_FILE_LEN / ROM_WIDTH); i++)
	{
		// For printing the ROM address at the beginning
		mvwprintw(ROM_win, cur_row, cur_col, "%04X: ", ROM_WIDTH * i);
		cur_col += 6;

		for (unsigned int j = 0; j < ROM_WIDTH; j++)
		{
			if (romptr > ROM_FILE_LEN)
				break;

			romptr = ROM_WIDTH * i + j;
			if (romptr == pc)
				wattron(ROM_win,COLOR_PAIR(1));
			if (romptr > pc && romptr < pc + get_instr_len[rom[pc]])
				wattron(ROM_win,COLOR_PAIR(2));

			mvwprintw(ROM_win, cur_row, cur_col, "%02X", rom[romptr]);
			cur_col += 3;

			wattroff(ROM_win,COLOR_PAIR(1));
			wattroff(ROM_win,COLOR_PAIR(2));
		}

		cur_row++;
		cur_col = 2;
	}
	wrefresh(ROM_win);
}

void printRAM()
{
	const unsigned int RAM_WIDTH = 16;

	box(RAM_win, 0 , 0);
	mvwprintw(RAM_win, 0, 3, " RAM ");

	int cur_col = 2, cur_row = 1;

	unsigned int ramptr = 0;
	for (unsigned int i = 0; i < (RAM_SIZE / RAM_WIDTH); i++)
	{
		mvwprintw(RAM_win, cur_row, cur_col, "%04X: ", RAM_WIDTH * i);
		cur_col += 6;
		for (unsigned int j = 0; j < RAM_WIDTH; j++)
		{
			if (ramptr > RAM_SIZE)
				break;
			ramptr = RAM_WIDTH * i + j;
			mvwprintw(RAM_win, cur_row, cur_col, "%02X", ram[ramptr]);
			cur_col += 3;
		}
		cur_row++;
		cur_col = 2;
	}
	wrefresh(RAM_win);
}

void printend(const char* str)
{
	int height, width;
	getmaxyx(stdscr, height, width);
	mvprintw(height - 1, 0, "%s", str);
}
