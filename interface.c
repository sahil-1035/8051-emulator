#include <stdio.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>

#include "interface.h"

#include "definitions.h"
#include "emulator.h"
#include "window.h"

Window ROM_win;
Window RAM_win;
Window MISC_win;
Window POPUP_win;

bool interface_quit;

void interface_main(void)
{
	emu_init("num1.bin");
	init_curses();

	pthread_t emulator_thread;
	pthread_create(&emulator_thread, NULL, (void* (*)(void*))emu_start, NULL);
	
	while ( !emu_quit )
	{
		print_curses();
		pthread_mutex_unlock(&data_mutex);

		manage_input();
		usleep(1000);
		pthread_mutex_lock(&data_mutex);

		/* pthread_mutex_unlock(&data_mutex); */
		/* emu_exec_instr(); */
		/* pthread_mutex_lock(&data_mutex); */
	}
	void* return_val;
	pthread_join(emulator_thread, return_val);

	if ( emu_return_cause == END_OF_ROM )
	{
		printend("End of ROM reached. -- Enter any key to exit --");
		getch();
	}
	endwin();
}


void init_curses(void)
{
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	refresh();

	int height, width;
	getmaxyx(stdscr, height, width);
	create_window(&ROM_win, height - 1, 50, 0, 0);
	create_window(&RAM_win, 18, width - 106 - 11, 0, 52);
	create_window(&MISC_win, height - 1 - 18, width - 106 - 11, 18, 52);
	create_window(&POPUP_win, 3, 15, height - 3, width - 15);
}

void print_curses(void)
{
	printRAM();
	printROM();
	printMISC();
	box(POPUP_win.win, 0, 0);
	refresh_window(&POPUP_win);
}

void manage_input(void)
{
	int inp = getch();
	if ( inp == ERR )
		return;
	else if ( inp == 'q' )
		interface_quit = true;
}


void printMISC(void)
{
	werase(MISC_win.win);
	box(MISC_win.win, 0, 0);
	MISC_win.cur_x = 2;
	MISC_win.cur_y = 1;

	print_to_window(&MISC_win, 0, "PC = %04XH;  ", pc);
	print_to_window(&MISC_win, 1, "// %s", instructions[rom[pc]].string);
	print_to_window(&MISC_win, 0, "A = %02XH;  ", a);
	print_to_window(&MISC_win, 0, "B = %02XH;  ", b);
	print_to_window(&MISC_win, 0, "SP = %02XH;  ", ram[0x81]);
	print_to_window(&MISC_win, 0, "DPTR = %04XH;", dptr);
	refresh_window(&MISC_win);
}

void printROM(void)
{
	box(ROM_win.win, 0 , 0);
	mvwprintw(ROM_win.win, 0, 3, " ROM ");

	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_BLACK, COLOR_BLUE);

	ROM_win.cur_x = 2;
	ROM_win.cur_y = 1;

	int instr_pos = 0;
	bool curr_instr = false;
	for (unsigned int i = 0; i < ROM_FILE_LEN; i++)
	{
		instr_pos = i;
		// For printing the ROM address at the beginning
		print_to_window(&ROM_win, 0, "%04X: ", i);

		if (i == pc)
			curr_instr = true;

		if (curr_instr)
			wattron(ROM_win.win,COLOR_PAIR(1));

		print_to_window(&ROM_win, 0, "%02X ", rom[i]);

		wattroff(ROM_win.win,COLOR_PAIR(1));
		wattroff(ROM_win.win,COLOR_PAIR(2));

		if (curr_instr)
			wattron(ROM_win.win,COLOR_PAIR(2));

		for (unsigned int j = 0; j < instructions[rom[instr_pos]].no_of_bytes - 1; j++)
			print_to_window(&ROM_win, 0, "%02X ", rom[++i]);

		wattroff(ROM_win.win,COLOR_PAIR(1));
		wattroff(ROM_win.win,COLOR_PAIR(2));

		ROM_win.cur_x = 20;
		print_to_window(&ROM_win, 1, "; %s", instructions[rom[instr_pos]].string);
	}
	refresh_window(&ROM_win);
}

void printRAM(void)
{
	const unsigned int RAM_WIDTH = 16;

	box(RAM_win.win, 0 , 0);
	mvwprintw(RAM_win.win, 0, 3, " RAM ");

	RAM_win.cur_x = 2;
	RAM_win.cur_y = 1;

	unsigned int ramptr = 0;
	for (unsigned int i = 0; i < (RAM_SIZE / RAM_WIDTH); i++)
	{
		// For printing the RAM address at the beginning
		print_to_window(&RAM_win, 0, "%04X: ", RAM_WIDTH * i);
		for (unsigned int j = 0; j < RAM_WIDTH; j++)
		{
			if (ramptr > RAM_SIZE)
				break;
			ramptr = RAM_WIDTH * i + j;
			print_to_window(&RAM_win, 0, "%02X ", ram[ramptr]);
		}
		print_to_window(&RAM_win, 1, "");
	}
	refresh_window(&RAM_win);
}
